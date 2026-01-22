# 3snake Analysis and Fixes

## Issues Identified

### 1. Output Redirection Issue (FIXED)
**Problem**: The `output()` macro in helpers.h was writing to stderr, but the program redirects stdout to the output file. This prevented traced credentials from appearing in the output file.

**Solution Applied**: Changed `output()` macro to write to stdout and added `fflush()` to ensure immediate output.

### 2. Race Condition / Late Process Detection
**Problem**: The netlink event listener detects process events (PROC_EVENT_EXEC, PROC_EVENT_UID) but there's a race condition:
- Event is fired
- Tracer process is forked
- By the time ptrace(PTRACE_ATTACH) is called, the sudo/su process may have:
  - Already exited
  - Changed state significantly
  - Become unavailable for tracing

This is particularly problematic for quick commands like `sudo -l` or `su -c` that complete very quickly.

**Why**: The process validation chain (trace_process → get_proc_name → validate_process_name → validate_process_path) takes time during which the target process may exit.

### 3. Debug Output Added
Added extensive debug output throughout the code to trace execution:
- plisten.c: Logs all netlink events
- tracers.c: Logs process validation steps
- sudo_tracer.c: Logs ptrace attachment and syscall tracing

## How to Rebuild and Test

```bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean
make
```

## How to Test with Debug Output

Run with debug output visible:
```bash
# In terminal (not daemonized) - debug output goes to stderr
sudo ./3snake

# In another terminal, try a sudo command:
sudo su -
# or
sudo passwd root
```

Watch for debug messages starting with `[DEBUG]` to see where the execution flow stops.

## Likely Root Cause

Based on the code analysis, the most likely reason for no output is that the processes complete too quickly before the tracer can attach. The program needs to:

1. Wait for the process event notification
2. Fork a child tracer process
3. Get the process name from /proc/[pid]/cmdline
4. Validate the process name and path
5. Call ptrace(PTRACE_ATTACH)
6. Begin tracing syscalls

All of this happens while the target process (sudo/su) is running and reading from stdin for the password.

## Recommendations for Testing

1. Try with longer-running commands that give more time for the tracer to attach:
   ```bash
   # Use sleep to keep sudo running longer
   sudo sleep 10
   ```

2. Or try with interactive commands:
   ```bash
   sudo su
   # Then enter password
   ```

3. Check if the program runs without errors:
   ```bash
   sudo ./3snake
   # No output? Check stderr for "[DEBUG]" messages
   ```

## Next Steps if Still No Output

If the above fixes don't produce output, the issue could be:

1. **Netlink events not firing** - Check if the kernel supports process connector netlink socket
2. **Permission issues** - Ensure running with proper root privileges
3. **Process state mismatch** - The /proc/[pid] entries may be gone before tracing begins
4. **Architecture-specific issues** - Check if __amd64__ is defined correctly for your system
