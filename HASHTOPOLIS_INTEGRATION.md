# Hashtopolis Integration Guide

## Overview

This cracker is fully compatible with Hashtopolis as a generic cracker. It wraps MDXfind to provide hash identification and cracking with proper progress reporting.

## Hashtopolis Agent Command Format

The Hashtopolis agent calls the cracker with the following format:

### Benchmark
```bash
cracker crack -a '<hashlist_path>' -w <wordlist> --hash-type "ALL" -i 10 -s 0 -l <keyspace> --timeout=<seconds>
```

### Chunk Execution
```bash
cracker crack -s <skip> -l <length> -a '<hashlist_path>' -w <wordlist> --hash-type "ALL" -i 10
```

### Keyspace Measurement
```bash
cracker keyspace -w <wordlist>
```

## Supported Arguments

| Argument | Alias | Description |
|----------|-------|-------------|
| `-a <file>` | `--attacked-hashlist` | File containing hashes to crack |
| `-w <file>` | `--wordlist` | Wordlist for dictionary attack |
| `-t <types>` | `--type`, `--hash-type` | Hash algorithm types (e.g., "ALL", "MD5,SHA1") |
| `-s <num>` | `--skip` | Skip first N passwords |
| `-l <num>` | `--length` | Process N passwords |
| `-i <num>` | `--iterations` | Iteration count for hash algorithms (default: 10) |
| `--timeout <sec>` | | Stop after N seconds (for benchmarking) |

## Output Format

### STATUS Lines
The cracker outputs progress updates to stdout in the format:
```
STATUS <progress> <speed>
```

Where:
- **progress**: Integer 0-10000 representing 0.00% to 100.00% completion
- **speed**: Hash rate in H/s (hashes per second)

Example:
```
STATUS 2500 13500000   # 25% complete, 13.5 Mh/s
```

### Cracked Hashes
Cracked hashes are output to stdout in the format:
```
hash:plaintext:algorithm
```

Example:
```
5f4dcc3b5aa765d61d8327deb882cf99:password:MD5x01
```

## Benchmark Behavior

### How Benchmarking Works

1. Hashtopolis agent runs:
   ```bash
   cracker crack -a '<hashlist>' -w <wordlist> --hash-type "ALL" -i 10 -s 0 -l <keyspace> --timeout=<bench_time>
   ```

2. The cracker processes hashes for `<bench_time>` seconds, then terminates

3. Agent collects all output and finds the **last valid STATUS line**

4. Returns `progress / 10000` as the benchmark result

### Example Benchmark

Command:
```bash
cracker crack -a hashes.txt -w wordlist.txt --hash-type "ALL" -i 10 -s 0 -l 1000000 --timeout=30
```

Output:
```
STATUS 0 0
STATUS 0 0
STATUS 145 6180000
STATUS 145 6180000
```

Agent sees:
- Last STATUS: `145 6180000`
- Progress: 145 (1.45% of 1M keyspace)
- Speed: 6.18 Mh/s
- Benchmark result: `145 / 10000 = 0.0145`

This tells Hashtopolis that 1.45% of the work was completed in 30 seconds, allowing it to estimate total time needed.

### Benchmark Configuration Recommendations

**MDXfind Progress Reporting Delay:**
- MDXfind outputs progress to stderr approximately every 15 seconds
- For benchmarks shorter than 15 seconds, progress may show as 0%
- **Recommended minimum benchmark time: 20-30 seconds**

**Hashtopolis Configuration:**
```
Cracker Type: Generic
Benchmark Timeout: 30 seconds (recommended)
```

### Why Short Benchmarks May Show 0%

MDXfind doesn't immediately report progress - it takes ~15 seconds to output the first progress line. If the benchmark timeout is < 15 seconds:

```bash
# 10 second benchmark
cracker ... --timeout=10

# Output:
STATUS 0 0
STATUS 0 0
STATUS 0 0  # No progress data yet!
```

The agent sees `progress = 0`, which results in a benchmark value of `0.0`, causing "Generic benchmark failed!" errors.

**Solution**: Configure Hashtopolis to use benchmark timeouts of at least 20-30 seconds.

## Progress Tracking

### MDXfind Progress Format

MDXfind outputs progress to **stderr** in this format:
```
Working on wordlist.txt, w=248, line 360, Found=2, 12.86Mh/s, 2.76Kc/s
```

The cracker parses:
- **line 360**: Current line in wordlist being processed
- **Found=2**: Number of hashes cracked so far
- **12.86Mh/s**: Hash rate in megahashes per second
- **2.76Kc/s**: Candidate rate in kilocandidates per second

### Progress Calculation

Progress is calculated as:
```
progress = (current_line - skip) / total_keyspace * 10000
```

Example with `-s 1000 -l 10000`:
- If MDXfind reports "line 3500"
- Processed: 3500 - 1000 = 2500 passwords
- Progress: (2500 / 10000) * 10000 = 2500 (25.00%)

## Chunk Execution

During normal chunk execution, the agent:

1. Runs the cracker with `-s <skip> -l <length>`
2. Reads stdout in real-time via threading
3. For each STATUS line:
   - Sends progress update to Hashtopolis server
   - Reports: `relativeProgress`, `speed`, `state`
4. For lines containing `:` (but not "STATUS"):
   - Treats as cracked hash
   - Sends to server in batches of up to 1000

### Status Updates

The cracker outputs STATUS lines every 5 seconds during execution. The agent:
- Monitors for STATUS lines on stdout
- Sends updates to Hashtopolis server
- Tracks cracked hashes separately

## Keyspace Measurement

The `keyspace` command simply counts lines in the wordlist:

```bash
cracker keyspace -w wordlist.txt
# Output: 57351281
```

The agent uses this to:
- Calculate chunk sizes
- Estimate total task time
- Plan distribution across agents

## Testing

### Test Benchmark
```bash
# Simulate Hashtopolis benchmark
python3 cracker.py crack \
  -a test_hashes.txt \
  -w wordlist.txt \
  --hash-type "ALL" \
  -i 10 \
  -s 0 \
  -l 1000000 \
  --timeout=30

# Expected output:
# STATUS 0 0
# STATUS 0 0
# STATUS <progress> <speed>  # After ~15 seconds
# STATUS <progress> <speed>
# ...
```

### Test Chunk
```bash
# Simulate Hashtopolis chunk execution
python3 cracker.py crack \
  -a test_hashes.txt \
  -w wordlist.txt \
  --hash-type "MD5" \
  -s 1000 \
  -l 5000

# Expected output:
# hash:plaintext:algorithm  # As hashes are cracked
# STATUS <progress> <speed>  # Every 5 seconds
# STATUS <progress> <speed>
# ...
# STATUS 10000 <speed>  # When complete
```

### Test Keyspace
```bash
# Count wordlist lines
python3 cracker.py keyspace -w wordlist.txt

# Expected output:
# 12345  # Line count
```

## Troubleshooting

### "Generic benchmark failed!"

**Cause**: No valid STATUS lines in output

**Solutions**:
1. Increase benchmark timeout to 30+ seconds
2. Check that hashes and wordlist files exist
3. Verify hash types are valid for MDXfind

### "Generic benchmark gave no output!"

**Cause**: Cracker produced no stdout

**Solutions**:
1. Test cracker manually with same command
2. Check stderr for error messages
3. Verify MDXfind binary exists in `mdx_bin/` directory

### Progress stuck at 0%

**Cause**: MDXfind hasn't reported progress yet

**Solutions**:
1. Wait at least 15 seconds for first progress update
2. For benchmarks, use 30+ second timeout
3. Verify large enough keyspace (> 1000 passwords)

### No cracked hashes reported

**Possible causes**:
1. No matching hashes in wordlist (normal)
2. Wrong hash algorithm selected
3. Malformed hash file

**Verification**:
```bash
# Test with known hash
echo "5f4dcc3b5aa765d61d8327deb882cf99" > test.txt
echo "password" > words.txt
python3 cracker.py crack -a test.txt -w words.txt --hash-type MD5

# Should output:
# 5f4dcc3b5aa765d61d8327deb882cf99:password:MD5x01
```

## Performance Characteristics

### Hash Rates (Approximate)

| Algorithm | Typical Speed (M H/s) |
|-----------|----------------------|
| MD5 | 10-15 |
| SHA1 | 8-12 |
| SHA256 | 5-8 |
| bcrypt | 0.001-0.01 |

*Note: Actual speeds depend on CPU, number of cores, and hash complexity*

### Keyspace Processing

- Small wordlists (< 10K): Complete in seconds
- Medium wordlists (100K-1M): Minutes to hours
- Large wordlists (10M+): Hours to days

MDXfind is CPU-bound and will utilize all available cores.

## Integration Checklist

- [ ] Cracker binary placed in Hashtopolis crackers directory
- [ ] `mdx_bin/mdxfind` executable is present and has execute permissions
- [ ] Benchmark timeout configured to 30+ seconds
- [ ] Test hashes and wordlist prepared
- [ ] Benchmark completes successfully without errors
- [ ] Chunk execution reports progress correctly
- [ ] Cracked hashes are received by Hashtopolis server

## Example Hashtopolis Configuration

```
Cracker Name: Generic-Cracker-MDXfind
Cracker Type: Generic
Binary Name: cracker
Binary Path: /path/to/generic-cracker/cracker
Version: 2.1
Benchmark Timeout: 30
```

## Notes

- The cracker requires Python 3.6+ on the agent system
- No special libraries needed (uses system Python)
- MDXfind handles the actual hash cracking
- The wrapper provides Hashtopolis-compatible interface
- All hash algorithms supported by MDXfind are available
