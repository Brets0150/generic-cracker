#!/bin/bash
# Build script for WSL Ubuntu
# Run this script inside WSL Ubuntu terminal

echo "Building MDXfind Wrapper in WSL Ubuntu..."

# Install qt5-qmake if not already installed
echo "Checking for required packages..."
if ! dpkg -l | grep -q qt5-qmake; then
    echo "Installing qt5-qmake..."
    sudo apt-get update
    sudo apt-get install -y qt5-qmake
fi

# Navigate to cracker directory
cd "$(dirname "$0")/cracker"

# Clean old build files
echo "Cleaning old build files..."
rm -f Makefile .qmake.stash *.o moc_*.cpp cracker

# Generate Makefile with qmake
echo "Generating Makefile with qmake..."
qmake cracker.pro

# Build with make
echo "Building with make..."
make -j$(nproc)

# Check if build succeeded
if [ -f "cracker" ]; then
    echo ""
    echo "✓ Build successful!"
    echo "Executable created at: cracker/cracker"
    echo ""
    echo "Test with:"
    echo "  ./cracker/cracker --help"
    echo ""
else
    echo ""
    echo "✗ Build failed!"
    echo "Please check the error messages above."
    exit 1
fi
