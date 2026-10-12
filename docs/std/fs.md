# std.fs

File system operations.

## Functions

```nx
import std.fs

// Read entire file as string
let content = fs.read_to_string("file.txt")?

// Read entire file as bytes
let bytes = fs.read("file.bin")?

// Write string to file
fs.write("output.txt", "Hello, World!")?

// Write bytes to file
fs.write_bytes("output.bin", &[1, 2, 3, 4])?

// Append to file
fs.append("log.txt", "New line\n")?

// Check existence
let exists = fs.exists("file.txt")
let is_file = fs.is_file("file.txt")
let is_dir = fs.is_dir("dir/")

// Get metadata
let meta = fs.metadata("file.txt")?
let size = meta.size()
let modified = meta.modified()

// Create directory
fs.create_dir("new_dir")?
fs.create_dir_all("path/to/deep/dir")?

// Remove
fs.remove("file.txt")?
fs.remove_dir("empty_dir")?
fs.remove_dir_all("dir_with_contents")?

// Rename/move
fs.rename("old.txt", "new.txt")?

// Copy
fs.copy("src.txt", "dst.txt")?

// Read directory
for entry in fs.read_dir(".") {
    println("{}", entry.name())
    println("  is_dir: {}", entry.is_dir())
    println("  size: {}", entry.metadata()?.size())
}

// Path operations
let path = fs.join("dir", "subdir", "file.txt")
let parent = fs.parent("path/to/file.txt")
let filename = fs.file_name("path/to/file.txt")
let extension = fs.extension("file.txt")

// Canonicalize (resolve symlinks, absolute)
let abs = fs.canonicalize("relative/path")?

// Temporary files
let (tmp_file, tmp_path) = fs.temp_file()?
let tmp_dir = fs.temp_dir()?

// Current directory
let cwd = fs.current_dir()?
fs.set_current_dir("/new/path")?
```

## Types

```nx
struct DirEntry {
    name: str,
    path: str,
    is_dir: bool,
    is_file: bool,
    is_symlink: bool,
}

struct Metadata {
    size: u64,
    modified: SystemTime,
    accessed: SystemTime,
    created: SystemTime,
    is_readonly: bool,
}
```

## Example

```nx
import std.fs
import std.io

fn main() {
    // Create config directory
    fs.create_dir_all("config")?
    
    // Write config
    fs.write("config/settings.json", r#"{"theme": "dark"}"#)?
    
    // Read back
    let content = fs.read_to_string("config/settings.json")?
    println("Config: {content}")
    
    // List files
    for entry in fs.read_dir("config") {
        println("{}", entry.name())
    }
}
```