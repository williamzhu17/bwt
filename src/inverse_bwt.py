#!/usr/bin/env python3
import sys
import argparse

def inverse(transformed: str, delimiter: str = '$') -> str:
    """
    Reconstruct the original string from its BWT transform using the LF-mapping
    described in Ben Langmead's thesis (S := cS; r := LF(r)).
    """
    n = len(transformed)
    if n == 0:
        return ""

    # Count characters and record Occ(c, i) = number of occurrences of `c`
    # strictly before position i in the last column.
    occ_before = {}
    occ_table = [0] * n
    delimiter_row = -1

    for i, ch in enumerate(transformed):
        if ch == delimiter:
            delimiter_row = i

        occ_table[i] = occ_before.get(ch, 0)
        occ_before[ch] = occ_table[i] + 1

    # C(c): index of the first occurrence of 'c' in the sorted first column.
    first_occurrence = {}
    total = 0
    for ch in sorted(occ_before.keys()):
        first_occurrence[ch] = total
        total += occ_before[ch]

    # Follow Langmead's pseudocode: rebuild string by iteratively applying LF.
    result = []
    row = delimiter_row
    while True:
        row = first_occurrence[transformed[row]] + occ_table[row]
        ch = transformed[row]
        if ch == delimiter:
            break
        result.append(ch)

    result.reverse()
    return "".join(result)

def main():
    parser = argparse.ArgumentParser(description='BWT Inverse Transform')
    parser.add_argument('input_file', help='Input file path')
    parser.add_argument('output_file', help='Output file path')
    parser.add_argument('block_size', nargs='?', type=int, default=128,
                       help='Block size in bytes (default: 128)')
    
    args = parser.parse_args()
    
    if args.block_size <= 0:
        print("Error: Block size must be greater than 0", file=sys.stderr)
        return 1
    
    # Note: Forward BWT outputs chunks of size (input_size + 1) due to delimiter
    # So we need to read chunks of size (block_size + 1) to match
    bwt_chunk_size = args.block_size + 1
    
    try:
        with open(args.input_file, 'rb') as infile, open(args.output_file, 'wb') as outfile:
            while True:
                # Read a chunk (size block_size + 1 to account for delimiter)
                chunk = infile.read(bwt_chunk_size)
                
                if len(chunk) == 0:
                    break
                
                # Convert bytes to string for processing
                chunk_str = chunk.decode('latin-1')
                
                # Apply inverse transform to this chunk and write
                result = inverse(chunk_str)
                outfile.write(result.encode('latin-1'))
    
    except FileNotFoundError as e:
        print(f"Error: Could not open input file {args.input_file}: {e}", file=sys.stderr)
        return 1
    except IOError as e:
        print(f"Error: Could not open output file {args.output_file}: {e}", file=sys.stderr)
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
