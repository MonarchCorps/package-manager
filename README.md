# xpkg

A minimal package manager written in C from scratch. Install, list, and remove packages using `.tar.gz` archives and a flat-file database. Zero runtime dependencies.

Built as part of the systems programming foundation for [EduOS](https://github.com/MonarchCorps) — a privacy-first, offline AI OS for African schools.

---

## Features

- Install packages from `.tar.gz` archives
- Parse `manifest.json` inside the archive for package metadata
- Copy files to the install location with correct directory structure
- Record installed packages in a flat-file database
- List all installed packages
- Remove packages and clean up all installed files

---

## Package Format

Every `.tar.gz` package must contain a `manifest.json` at its root:

```json
{
  "name": "hello",
  "version": "1.0.0",
  "files": ["bin/hello", "share/hello/README.md"]
}
```

---

## Usage

```bash
xpkg install hello-1.0.0.tar.gz   # Install a package
xpkg list                          # List all installed packages
xpkg remove hello                  # Remove a package by name
xpkg --help                        # Show usage
```

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Or with gcc directly:

```bash
gcc main.c manager.c -o xpkg
```

---

## How It Works

**Install:**
1. Extracts the `.tar.gz` to a temp directory using the system `tar` command
2. Finds and parses `manifest.json` from the extracted directory
3. Copies each listed file to the install location (`/usr/local/xpkg/<name>/`)
4. Records the package name, version, and file list in `xpkg.db`

**List:**
Reads `xpkg.db` line by line and prints each installed package name and version.

**Remove:**
Looks up the package in `xpkg.db`, deletes every file it installed, removes the package directory, and rewrites the database without that entry.

---

## Database Format

`xpkg.db` is a plain text flat file. Each line is one installed package:

```
hello|1.0.0|bin/hello,share/hello/README.md
```

Fields are pipe-separated. Files are comma-separated within the third field.

---

## Project Context

xpkg is Project 15 in a structured C systems programming curriculum. Projects in this curriculum build toward EduOS — a privacy-first, AI-native Linux OS designed for African schools with offline-first architecture, low-RAM hardware targets, and no cloud dependency.

Previous projects in the curriculum: memory allocator, file explorer, process monitor, HTTP server, terminal chat app, mini Unix shell.

---

## Author

**David Okocha** — Full Stack Engineer at Migranium, building EduOS.

X: [@davidokocha086](https://x.com/davidokocha086)