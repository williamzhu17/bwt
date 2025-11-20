#!/usr/bin/env python3
import sys
import argparse

def inverse(transformed: str):
    """
    Generates the inverse of the BWT.
    """
    N = len(transformed)

    output_str = ""
    delimiter = '~'

    # decoding loop
    bwm_list = ["" for i in range(N)]
    for _ in range(N):
        bwm_list = [transformed[i] + bwm_list[i] for i in range(N)]
        bwm_list.sort()

    # find the string which ends with delimiter
    output_str = ""
    for _str in bwm_list:
        if _str.endswith(delimiter):
            # Return string without the delimiter
            output_str = _str[:-1]
            break

    return output_str

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
