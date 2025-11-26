# Salt Support in MDXfind Wrapper

## Overview

The MDXfind wrapper now fully supports salted hashes, automatically parsing hashlist files to extract salts and passing them to MDXfind for proper hash identification and cracking.

## Supported Hash Formats

The wrapper intelligently handles multiple hashlist formats from Hashtopolis:

### Format 1: Hash Only
```
5f4dcc3b5aa765d61d8327deb882cf99
098f6bcd4621d373cade4e832627b4f6
```

### Format 2: Hash:Salt
```
4b0fca56687b8673e0e08aaaa0e9e63e:c0
fa0a58319f00c191d0a28c0640bd2df0:f1
bb5202b17c508dd32fd9f819ce3ec291:16
```

### Format 3: Hash:Salt:Plaintext
```
5192d8813bbef9b620dd91a757834dc2:vl1A*)qMhN&d4\|8EwtgwHnOpGeK]P:zhurA123
bbd6cab6f7a72061a17a14d279bcb866:q7X79"l0lH8OE)"1btH9P>]}{@>%"`:qweRty99
```

## How Salt Parsing Works

### Intelligent Detection

The wrapper uses heuristics to distinguish between salts and plaintexts in 2-field format:

1. **Hex Pattern Check**: If the second field matches `^[a-fA-F0-9]+$` and is ≤64 chars, it's treated as a salt
2. **Length Check**: If the second field is ≤16 chars, it's likely a salt
3. **Otherwise**: Treated as plaintext (no salt)

### File Generation

For each hashlist, the wrapper creates two temporary files:

- `hashlist.txt.hashes` - Contains only the hash values
- `hashlist.txt.salts` - Contains corresponding salts (one per line, empty line if no salt)

**Example:**

Input (`hashes.txt`):
```
5192d8813bbef9b620dd91a757834dc2:vl1A*)qMhN&d4\|8EwtgwHnOpGeK]P:zhurA123
4b0fca56687b8673e0e08aaaa0e9e63e:c0
5f4dcc3b5aa765d61d8327deb882cf99
```

Generated `hashes.txt.hashes`:
```
5192d8813bbef9b620dd91a757834dc2
4b0fca56687b8673e0e08aaaa0e9e63e
5f4dcc3b5aa765d61d8327deb882cf99
```

Generated `hashes.txt.salts`:
```
vl1A*)qMhN&d4\|8EwtgwHnOpGeK]P
c0

```

## MDXfind Integration

The wrapper passes the extracted files to MDXfind with the appropriate options:

```bash
mdxfind -h "ALL,!user,salt" -i 10 -q 10 -f hashlist.txt.hashes -s hashlist.txt.salts -e -g wordlist.txt
```

### Command-Line Options

- `-h` - Hash types (default: "ALL,!user,salt")
- `-i` - Iterations count (default: 10)
- `-q` - Internal iteration counts (default: 10)
- `-f` - Hash file path
- `-s` - Salt file path (only if salts detected)
- `-e` - Extended search for truncated hashes
- `-g` - Rotate calculated hashes to attempt match

## Output Handling

MDXfind returns different formats depending on whether salts are present:

### Without Salt
```
MD5x01 5f4dcc3b5aa765d61d8327deb882cf99:password
```

Wrapper output to Hashtopolis:
```
5f4dcc3b5aa765d61d8327deb882cf99:password:MD5x01
```

### With Salt
```
MD5SALT 5192d8813bbef9b620dd91a757834dc2:vl1A*)qMhN&d4\|8EwtgwHnOpGeK]P:zhurA123
MD5SALTPASSx01 4b0fca56687b8673e0e08aaaa0e9e63e:c0:hellothere
```

Wrapper output to Hashtopolis:
```
5192d8813bbef9b620dd91a757834dc2:zhurA123:MD5SALT
4b0fca56687b8673e0e08aaaa0e9e63e:hellothere:MD5SALTPASSx01
```

**Note:** The salt is stripped from the output because Hashtopolis already has it in the original hashlist.

## Usage Examples

### Basic Salted Hash Cracking

```bash
./cracker crack -a salted_hashes.txt -w wordlist.txt
```

### With Custom Hash Types

```bash
./cracker crack -a salted_hashes.txt -w wordlist.txt --hash-type "MD5SALT,SHA1SALT"
```

### With Iterations

```bash
./cracker crack -a salted_hashes.txt -w wordlist.txt -i 5
```

### Full Example (matching your bash script)

```bash
./cracker crack \
  -a sample_hashes.txt \
  -w /path/to/wordlist.txt \
  --hash-type "ALL,!user,salt" \
  -i 10 \
  -l 1000000 \
  --timeout 3600
```

## Implementation Details

### Code Changes

1. **[cracker/runthread.h](cracker/runthread.h:38-40)**
   - Added `hashFile`, `saltFile`, and `hasSalts` members
   - Added `parseHashlistWithSalts()` method

2. **[cracker/runthread.cpp:160-216](cracker/runthread.cpp)**
   - Implemented `parseHashlistWithSalts()` to extract hashes and salts
   - Creates temporary `.hashes` and `.salts` files

3. **[cracker/runthread.cpp:220-256](cracker/runthread.cpp)**
   - Updated `runMDXfind()` to pass salt file with `-s` option
   - Added hash type (`-h`) and iterations (`-i`, `-q`) support

4. **[cracker/runthread.cpp:335-379](cracker/runthread.cpp)**
   - Enhanced `parseMDXfindOutput()` to handle both formats
   - Strips salt from output before reporting to Hashtopolis

5. **[cracker/main.cpp:51-59](cracker/main.cpp)**
   - Added `--hash-type` and `-i`/`--iterations` command-line options

## Testing

Tested with [sample_hashes.txt](sample_hashes.txt) containing:
- 3 hashes with long complex salts (format: `hash:salt:plaintext`)
- 4 hashes without salts (format: `hash:plaintext`)
- 2 SHA1 hashes without salts
- 6 hashes with short hex salts (format: `hash:salt:plaintext`)

All formats were successfully parsed and cracked by MDXfind.

## Bash Script Compatibility

The wrapper's behavior matches your manual bash script:

```bash
#!/bin/bash
ts=$(date +%s)
working_dir='./'
mdxfind="/mnt/7tb_data/software/mdxfind/mdxfind"
log="${working_dir}/log_plain_${ts}_.txt"
hash_parm='ALL,!user,salt'
rounds=10
hashes_file="${1}"
dict="${2}"
hash="${working_dir}/hash"
salts="${working_dir}/salts"

echo -n '' > "${hash}"
echo -n '' > "${salts}"
cat "${hashes_file}" | awk -F':' '{print $1}' >> "${hash}"
cat "${hashes_file}" | awk -F':' '{print $2}' >> "${salts}"
"${mdxfind}" -h "${hash_parm}" -i "${rounds}" -q "${rounds}" -f "${hash}" -s "${salts}" -e -g -l "${dict}" >>"${log}"
```

The wrapper automates this entire process and integrates it with Hashtopolis.

## Benefits

1. **Automatic Detection**: No need to manually specify if hashes have salts
2. **Format Flexibility**: Handles multiple input formats seamlessly
3. **Clean Output**: Strips salts before reporting to Hashtopolis
4. **Algorithm Identification**: Reports which hash algorithm was successful
5. **Distributed Cracking**: Works with Hashtopolis skip/length parameters
6. **Real-time Progress**: Status updates every 5 seconds

## Future Enhancements

Potential improvements:
- Support for more complex salt positions (e.g., `salt:hash:plaintext`)
- Custom salt detection rules via configuration
- Support for userid files (`-u` option)
- Support for rules files (`-r`/`-R` options)
- Support for suffix files (`-k` option)
