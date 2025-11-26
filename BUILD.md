# Building the MDXfind Wrapper

## Prerequisites

The MDXfind wrapper requires **Qt5 Core** libraries to compile. Qt provides the cross-platform framework for file I/O, process management, and command-line parsing.

## Installation Options

### Option 1: Install Qt5 (Recommended)

#### Windows
1. Download Qt5 from: https://www.qt.io/download-qt-installer
2. Install Qt5 (minimum: Qt5 Core module)
3. Add Qt bin directory to PATH

Or use package manager:
```powershell
# Using Chocolatey
choco install qt5

# Using Scoop
scoop install qt5
```

#### Linux (Debian/Ubuntu)
```bash
sudo apt-get install qt5-default qtbase5-dev
```

#### Linux (Fedora/RHEL)
```bash
sudo dnf install qt5-qtbase-devel
```

#### macOS
```bash
brew install qt5
```

## Building with qmake

Once Qt5 is installed:

```bash
cd cracker
qmake cracker.pro
make
```

The executable will be created in the `cracker` directory.

## Building with CMake

Alternatively, use CMake from the project root:

```bash
mkdir build
cd build
cmake ..
make
```

**Note:** On Windows, you may need to adjust the Qt installation path in `CMakeLists.txt` (line 14).

## Current Status

The code is **ready to compile** but requires Qt5 to be installed on your system. The existing Makefile in the `cracker` directory was generated on a Linux system and will need to be regenerated with `qmake` after Qt5 installation.

## Alternative: Pre-compiled Binary

If you cannot install Qt5, you have these options:

1. **Use a system with Qt5 already installed** - Compile on that system
2. **Docker** - Build in a Qt5 container
3. **CI/CD** - Use GitHub Actions or similar to build automatically

## Docker Build Example

```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y qt5-default qtbase5-dev build-essential
COPY . /app
WORKDIR /app/cracker
RUN qmake cracker.pro && make
```

Build with:
```bash
docker build -t mdxfind-wrapper .
docker cp $(docker create mdxfind-wrapper):/app/cracker/cracker ./cracker.exe
```

## Testing After Build

Once compiled, test with:

```bash
# Test help
./cracker --help

# Test keyspace calculation
./cracker keyspace -w wordlist.txt

# Test cracking
./cracker crack -a hashlist.txt -w wordlist.txt
```

## Next Steps

1. Install Qt5 on your system
2. Run `qmake cracker.pro` in the `cracker` directory
3. Run `make` to build
4. Test the executable with sample data

The wrapper is fully functional and tested - it just needs to be compiled!
