# 3snake - Quick Reference

## TL;DR - The Problem and Solution

**Problem**: No output when tracing sudo/su/ssh despite the program running.

**Root Cause**: Output macro wrote to `stderr` instead of `stdout`, which is where output file redirection occurs.

**Solution**: Changed `fprintf(stderr, ...)` to `fprintf(stdout, ...)` in output macro and added `fflush()`.

---

## Quick Start

```bash
# 1. Build
cd /home/dj/PurpleTeam/3snake/3snake
make clean && make

# 2. Terminal 1 - Start tracer with debug output visible
sudo ./3snake

# 3. Terminal 2 - Execute a command that requires password
sudo -l

# 4. Should see output like:
# [DEBUG] PROC_EVENT_EXEC detected, pid=xxxxx
# [root] 1234567890 xxxxx sudo -l     password_here
```

---

## What Was Changed

| File | Change | Line(s) |
|------|--------|---------|
| src/helpers.h | `fprintf(stderr` → `fprintf(stdout` + `fflush()` | 11-14 |
| src/plisten.c | Added debug output | ~80-140 |
| src/tracers.c | Added debug output | ~175-282 |
| src/sudo_tracer.c | Added debug output | Throughout |

---

## Debug Output Prefixes

| Prefix | Meaning |
|--------|---------|
| `[DEBUG]` | Program flow and validation steps |
| `[SUDO]` | Sudo tracer specific events |
| `[root]` | Captured credential output |
| `[-] ERROR:` | Fatal error (from original code) |

---

## Common Test Commands

| Goal | Command |
|------|---------|
| Quick test | `sudo -l` |
| Interactive login | `sudo su` |
| Change password | `sudo passwd root` |
| SSH test | `ssh user@host` (if ENABLE_SSH_CLIENT=1) |

---

## If No Output Appears

1. Check debug messages:
   ```bash
   sudo ./3snake 2>&1 | grep DEBUG
   ```

2. Look for these patterns:
   - `PROC_EVENT_EXEC detected` → Process was detected ✓
   - `Matched SUDO` → Name validation passed ✓
   - `Path matched` → Path validation passed ✓
   - `Process attached successfully` → Ptrace attached ✓

3. If "Process not stopped after attach!":
   - Target process exited too quickly
   - Try longer-running commands: `sudo sleep 10` → type password

4. If no PROC_EVENT messages:
   - Kernel may not support netlink process events
   - Try running longer-lived process to see if events fire later

---

## Files to Check

- **Main changes**: 
  - `src/helpers.h` - Output macro fix
  - `src/plisten.c` - Process detection logging
  - `src/tracers.c` - Validation logging
  - `src/sudo_tracer.c` - Syscall tracing logging

- **Configuration**:
  - `src/config.h` - Enable/disable tracers, configure paths

- **Documentation**:
  - `FIXES_APPLIED.md` - Detailed changes
  - `TESTING_GUIDE.md` - Complete testing procedures
  - `FIX_SUMMARY.md` - Executive summary

---

## Verify Binary Was Built

```bash
ls -lah /home/dj/PurpleTeam/3snake/3snake/3snake
file /home/dj/PurpleTeam/3snake/3snake/3snake
```

Should show an executable file, not older than the source changes.

---

## Key Insight

The program was likely working and capturing credentials, but:
- They were written to the wrong stream (stderr vs stdout)
- There was no way to see if it was working (no debug output)

Now:
- Output goes to the correct stream (stdout) 
- Debug messages show exactly what's happening
- Can verify functionality and troubleshoot issues

---

## Original Symptoms ❌
- No output file contents
- No terminal output
- Program appeared to run but do nothing
- No way to debug

## Expected After Fix ✅
- Captured credentials in output file or terminal
- Debug messages showing execution flow
- Can see exactly where in the process detection chain things succeed/fail
- Can test different scenarios and understand behavior
