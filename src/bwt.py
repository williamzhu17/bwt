#!/usr/bin/env python3
import sys
import argparse

def forward(input: str):
    """
    Generates the forward transform of BWT
    """

    # create a string using data_block
    bwt_str = ""
    # STEP-1: add a delimiter
    input += '$'

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
    
    try:
        with open(args.input_file, 'rb') as infile, open(args.output_file, 'wb') as outfile:
            while True:
                # Read a chunk
                chunk = infile.read(args.block_size)
                
                if len(chunk) == 0:
                    break
                
                # Convert bytes to string for processing
                chunk_str = chunk.decode('latin-1')
                
                # Apply forward transform to this chunk and write
                result = forward(chunk_str)
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
