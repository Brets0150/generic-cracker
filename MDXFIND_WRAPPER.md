# MDXfind Wrapper for Hashtopolis

This is a specialized Hashtopolis agent that wraps the MDXfind tool for hash algorithm identification and cracking.

## Overview

The original generic-cracker has been adapted to act as a wrapper for MDXfind, allowing Hashtopolis to leverage MDXfind's ability to identify and crack hashes using 600+ different hash algorithms.

## Key Changes

### Modified Files

1. **[cracker/runthread.h](cracker/runthread.h)**
   - Added `QProcess` support for subprocess execution
   - Added `QRegularExpression` for output parsing
   - Added new methods:
     - `runMDXfind()` - Executes MDXfind as a subprocess
     - `parseMDXfindOutput()` - Parses MDXfind output format
     - `getMDXfindExecutable()` - Locates MDXfind binary

2. **[cracker/runthread.cpp](cracker/runthread.cpp)**
   - Replaced MD5 cracking logic with MDXfind subprocess execution
   - Implements real-time output parsing
   - Maintains Hashtopolis-compatible status reporting
   - Supports skip/length parameters for distributed cracking

3. **[cracker/main.cpp](cracker/main.cpp)**
   - Updated application name to "mdxfind-wrapper"
   - Updated description to reflect MDXfind functionality

## How It Works

### Execution Flow

1. **Input**: The wrapper receives:
   - Hashlist file (`-a` option)
   - Wordlist file (`-w` option)
   - Skip value (`-s` option) - for distributed workload
   - Length value (`-l` option) - number of passwords to try
   - Timeout (`--timeout` option)
   - Hash type (`--hash-type` option) - defaults to "ALL,!user,salt"
   - Iterations (`-i` option) - defaults to 10

2. **Hash Parsing**:
   - Parses the hashlist to separate hashes and salts
   - Supports multiple formats:
     - `hash` - Single hash without salt
     - `hash:salt` - Hash with salt (2 fields)
     - `hash:salt:plaintext` - Hash with salt and plaintext (3 fields)
   - Creates temporary `.hashes` and `.salts` files
   - Uses heuristics to distinguish between salt and plaintext in 2-field format

3. **MDXfind Execution**:
   - Launches MDXfind as a subprocess with appropriate arguments
   - Passes hash file (`-f`) and salt file (`-s`) to MDXfind
   - Includes hash type (`-h`), iterations (`-i`, `-q`), extended search (`-e`), and rotation (`-g`) options
   - MDXfind automatically detects hash types and attempts cracking with salts

4. **Output Parsing**:
   - Monitors MDXfind's stdout in real-time
   - Parses two output formats:
     - Without salt: `HASHTYPE hash:plaintext`
       - Example: `MD5x01 5f4dcc3b5aa765d61d8327deb882cf99:password`
     - With salt: `HASHTYPE hash:salt:plaintext`
       - Example: `MD5SALT 5192d8813bbef9b620dd91a757834dc2:vl1A*):zhurA123`

5. **Hashtopolis Reporting**:
   - Converts to format: `hash:plaintext:hashtype`
   - Example (no salt): `5f4dcc3b5aa765d61d8327deb882cf99:password:MD5x01`
   - Example (with salt): `5192d8813bbef9b620dd91a757834dc2:zhurA123:MD5SALT`
   - This allows Hashtopolis to know:
     - The cracked hash
     - The plaintext password (salt stripped)
     - **The algorithm that was used** (key differentiator)

6. **Status Updates**:
   - Reports progress every 5 seconds
   - Format: `STATUS <progress> <speed>`
   - Final status: `STATUS 10000 0` (100% complete)

### MDXfind Binary Location

The wrapper searches for the MDXfind executable in the following locations:
- `../mdx_bin/mdxfind.exe` (Windows) or `../mdx_bin/mdxfind` (Linux/Mac)
- `./mdx_bin/mdxfind.exe` (Windows) or `./mdx_bin/mdxfind` (Linux/Mac)

## Usage

### Keyspace Calculation

Calculate the keyspace (number of passwords) in a wordlist:

```bash
./cracker keyspace -w wordlist.txt
```

### Cracking

Run MDXfind against a hashlist with a wordlist:

```bash
./cracker crack -a hashlist.txt -w wordlist.txt -l 1000000 --timeout 3600
```

#### With Custom Hash Types and Iterations

```bash
./cracker crack -a hashlist.txt -w wordlist.txt --hash-type "MD5SALT,SHA1SALT" -i 5
```

Parameters:
- `-a` - Path to hashlist file (required)
- `-w` - Path to wordlist file (required)
- `-s` - Number of passwords to skip (for distributed cracking)
- `-l` - Number of passwords to try (length of keyspace)
- `--timeout` - Maximum time in seconds
- `--hash-type` - Hash types for MDXfind (default: "ALL,!user,salt")
- `-i` / `--iterations` - Number of iterations for hash algorithms (default: 10)

### Output Format

When a hash is cracked, output format is:
```
hash:plaintext:algorithm
```

Example:
```
5f4dcc3b5aa765d61d8327deb882cf99:password:MD5x01
098f6bcd4621d373cade4e832627b4f6:test:MD5x01
```

Status updates:
```
STATUS 2500 1234
STATUS 5000 1198
STATUS 10000 0
```

## Building

### Using qmake

```bash
cd cracker
qmake cracker.pro
make
```

### Using cmake

```bash
mkdir build
cd build
cmake ..
make
```

Note: Requires Qt5 Core libraries.

## MDXfind Capabilities

MDXfind supports 600+ hash algorithms including:
- Standard hashes: MD5, SHA1, SHA256, SHA512, etc.
- Salted hashes: MD5SALT, SHA1SALT, etc.
- Combination hashes: MD5SHA1, SHA1MD5, etc.
- Application-specific: NTLM, MySQL5, PHPBB3, etc.
- And many more...

MDXfind automatically identifies the hash type when given a wordlist, making it ideal for scenarios where the hash algorithm is unknown.

## Integration with Hashtopolis

This wrapper maintains full compatibility with the Hashtopolis API:
- Supports distributed cracking via skip/length parameters
- Reports progress and speed statistics
- Outputs cracked hashes in real-time
- Provides completion status

The key enhancement is that **each cracked hash includes the algorithm information**, allowing Hashtopolis to:
- Track which algorithm was successful
- Provide more detailed reporting
- Enable algorithm-specific analysis

## Future Enhancements

Potential additions:
- Support for MDXfind's `-h` option to specify hash types
- Support for salt files (`-s`)
- Support for userid files (`-u`)
- Support for rules files (`-r`/`-R`)
- Support for iteration counts (`-i`)
- Thread control (`-t`)
- Integration with mdsplit for result processing

## License

Same as original generic-cracker project.
