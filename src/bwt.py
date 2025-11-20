#!/usr/bin/env python3
import sys
import argparse

def find_unique_char(file):
    """
    Find a byte value (0-255) that does not appear in the file.
    Returns the first unused byte value, or None if all 256 values are used.
    
    Args:
        file: Either a file path (str)
    
    Returns:
        int: A byte value (0-255) that doesn't appear in the file, or None if all are used
    """
    # Track which byte values appear in the file
    used_bytes = set()
    
    # File path provided - open it
    with open(file, 'rb') as f:
        while True:
            chunk = f.read(8192)  # Read in 8KB chunks
            if not chunk:
                break
            used_bytes.update(chunk)
    
    # Find the first unused byte value (0-255)
    for byte_val in range(256):
        if byte_val not in used_bytes:
            return byte_val
    
    # All 256 byte values are used (extremely rare)
    return None


def forward(input: str, delimiter: str):
    """
    Generates the forward transform of BWT
    """

    # create a string using data_block
    bwt_str = ""
    # STEP-1: add a delimiter
    input += delimiter

    # STEP-2: get all cyclic rotations, and sort
    N = len(input)
    cyclic_rotations = []
    cur_str = input
    for _ in range(N):
        cur_str = cur_str[-1] + cur_str[:-1]
        cyclic_rotations.append(cur_str)
    cyclic_rotations.sort()

    # STEP-3: pick the last column and make a single string
    bwt_str = "".join([rot_str[-1] for rot_str in cyclic_rotations])

    return bwt_str

def main():
    parser = argparse.ArgumentParser(description='BWT Forward Transform')
    parser.add_argument('input_file', help='Input file path')
    parser.add_argument('output_file', help='Output file path')
    parser.add_argument('block_size', nargs='?', type=int, default=128,
                       help='Block size in bytes (default: 128)')
    
    args = parser.parse_args()
    
    if args.block_size <= 0:
        print("Error: Block size must be greater than 0", file=sys.stderr)
        return 1
    
    # find unique delimiter
    delimiter_byte = find_unique_char(args.input_file)
    
    if delimiter_byte is None:
        print("Error: Cannot find a unique delimiter (all 256 byte values appear in file)", file=sys.stderr)
        return 1
    
    # Convert byte value to single-character string
    delimiter = chr(delimiter_byte)
    
    try:
        with open(args.input_file, 'rb') as infile, open(args.output_file, 'wb') as outfile:
            # Write delimiter as first byte of output file
            outfile.write(bytes([delimiter_byte]))
            
            while True:
                # Read a chunk
                chunk = infile.read(args.block_size)
                
                if len(chunk) == 0:
                    break
                
                # Convert bytes to string for processing
                chunk_str = chunk.decode('latin-1')
                
                # Apply forward transform to this chunk and write
                result = forward(chunk_str, delimiter)
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
