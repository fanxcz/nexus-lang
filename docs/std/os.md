# `std.os`

The current `stdlib/os.nx` module wraps two command-line argument helpers:

- `args_count() -> i64` returns the number of command-line arguments.
- `args_get(index: i64) -> string` returns an argument by zero-based index; a negative or out-of-range index returns an empty string.

The executable path is **not** included. If the program is run as `./tool alpha "beta gamma"`, argument 0 is `alpha`, argument 1 is `beta gamma`, and `args_count()` is 2.

## Example

```nx
import std.os

fn main() {
    let count = args_count()
    print("Arguments: " + str_i64(count))

    let mut i: i64 = 0
    while i < count {
        print(str_i64(i) + ": " + args_get(i))
        i = i + 1
    }
}
```

The module also exposes a per-user application configuration directory:

- `config_dir(app_name: string) -> string` creates and returns the app's configuration directory. It returns an empty string on failure.
- `config_error() -> string` returns the reason from the most recent `config_dir` call on the current thread; it is empty after success.

The platform base is `APPDATA` (falling back to `LOCALAPPDATA`) on Windows, `~/Library/Application Support` on macOS, and an absolute `XDG_CONFIG_HOME` or `~/.config` on Linux. The application name must be a single path component of at most 128 bytes; path separators, control characters and platform-invalid filename characters are rejected. The directory is created if needed. The runtime test covers successful creation, invalid names, path traversal and a base path that is a regular file.

The wrappers do not currently expose environment enumeration, process spawning, current-directory changes or platform-specific OS handles. Use the built-in `env(name)` to read one environment variable.

## Application configuration directory

Use `config_dir` to create a per-user directory for application settings or small
configuration files. It returns an empty string on failure; `config_error` returns the
failure description for the current thread.

```nx
import std.os

fn main() {
    let directory = config_dir("My NEXUS App")
    if directory == "" {
        print("Could not prepare settings directory: " + config_error())
        return
    }
    print("Settings directory: " + directory)
}
```

The application name is one path component (maximum 128 bytes). It cannot contain
path separators, control characters, or reserved filename characters. This prevents
the app-name argument from selecting a parent or unrelated directory. The API creates
the directory if necessary; it does not create a settings file for the application.

## Capturing command output

The built-in `capture_output(command: string) -> string` runs a shell command and returns everything it writes to stdout as a string (empty string on failure). It complements `system()`, which only returns the exit code:

Both built-ins execute a command through the host command interpreter (`std::system` / `popen` on POSIX and the corresponding Windows CRT process helpers). They are **not** argument-array process APIs and do not escape or quote interpolated values for you. Never concatenate arbitrary file paths, process names, URLs, user input or untrusted project data into a shell command. Prefer a fixed command; validate every dynamic argument against a narrow allow-list and use platform-specific argument escaping only when unavoidable. Commands execute with the permissions of the current user and are not sandboxed.

For a safer application boundary, keep external command execution out of untrusted input paths. The `examples/task_manager.nx` sample restricts process/program tokens to ASCII letters, digits, dot, dash and underscore, and requires a non-zero decimal PID before composing its fixed Windows commands. This validation is specific to that sample, not a general shell-escaping API.

```nx
fn main() {
    let listing = capture_output("tasklist /FO CSV /NH")
    let lines = string.split_count(listing, "\n")
    print("process entries: " + lines)
}
```

`capture_output` requires the LLVM backend (`--backend llvm`) and is not part of the built-in native subset.
