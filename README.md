### Here's a draft README file for your project:

```markdown
# C++ Compilation and Hashing Utility

## Overview
This project is a **C++ compilation manager** that streamlines the compilation process for multiple C++ files. It leverages configuration files, file hashing (SHA-256), and caching mechanisms to detect changes and compile only the modified files, thereby reducing unnecessary recompilation efforts.

### Key Features
- **Change Detection**: Uses SHA-256 hashing to identify modified files and compile only those.
- **Custom Configuration**: Supports per-file and global configuration using JSON files.
- **Automatic Directory Management**: Ensures required directories (e.g., cache, logs) exist.
- **Customizable Compiler Flags**: Provides options for specifying additional compiler flags.
- **Cross-file Dependencies**: Handles compilation requirements per file, minimizing redundant actions.
- **Logs and Diagnostics**: Generates detailed compilation logs for debugging.

---

## Dependencies
### Required Libraries and Tools
1. **C++17 or higher**: The project uses modern C++ features.
2. [**nlohmann/json**](https://github.com/nlohmann/json): For handling JSON configurations.
3. [**OpenSSL**](https://www.openssl.org/): For computing SHA-256 file hashes.
4. **Windows SDK**: Necessary for `SetConsoleOutputCP` and related functions.
5. **Microsoft Visual C++ Compiler (`cl`)**: For compiling the C++ files.

To install dependencies, use your package manager or build the libraries manually.

---

## How to Use

### 1. Prepare the Environment
- Ensure the following files and directories exist:
  - `config.json`: The global configuration file.
  - Input directory containing `.cpp` files.
  - Optional per-file JSON configuration files for additional settings.

### 2. Configure `config.json`
Define global settings in `config.json`. Example structure:
```json
{
  "input_directory": "path/to/input",
  "output_directory": "path/to/output",
  "cache_directory": "path/to/cache",
  "default_compiler_flags": "/O2 /W4",
  "devcmd_path": "path/to/dev.bat"
}
```

### 3. Run the Program
- Compile and execute the utility.
- The program will:
  1. Scan the input directory for `.cpp` files.
  2. Compute hashes and check for changes.
  3. Compile only modified files or those missing their outputs.

### 4. Review Logs
- Compilation logs are saved in the `compile_logs` directory.

---

## Features and Capabilities

### JSON Configuration Options
- **Global Configuration**:
  - Input/Output directories.
  - Default compiler flags.
- **File-specific Configuration**:
  - Custom output file names.
  - Specific compiler flags.

### Automatic Cache Management
- Tracks the last state of each file using a hash stored in the cache directory.

### Error Handling
- Gracefully handles missing files or directories.
- Logs errors and skips problematic files instead of halting execution.

---

## Example
Assume the following structure:
```
project/
├── config.json
├── input/
│   ├── file1.cpp
│   ├── file2.cpp
│   └── file2.json
└── cache/
```

1. Create `config.json` with the appropriate paths.
2. Place `.cpp` files in the input directory.
3. Optionally, define `file2.json` to override global settings for `file2.cpp`.
4. Run the program and view the output files in the specified output directory.

---

## Notes
- This utility is currently designed for Windows platforms due to its reliance on the Microsoft Visual C++ compiler (`cl`) and batch files.
- Ensure the `dev.bat` script initializes your development environment (e.g., setting up paths for `cl`).

---

## Contributing
Feel free to fork this repository and contribute by adding:
- Cross-platform support.
- Enhanced dependency management.
- Integration with modern build tools.

---
