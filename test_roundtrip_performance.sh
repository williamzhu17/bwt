#!/bin/bash

# BWT Roundtrip Performance Test Script
# Tests both C++ and Python implementations

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CXX_BWT="src/bwt"
CXX_INVERSE_BWT="src/inverse_bwt"
PY_BWT="src/bwt.py"
PY_INVERSE_BWT="src/inverse_bwt.py"

# Check command line arguments
if [ $# -lt 1 ]; then
    echo -e "${RED}Usage: $0 <input_file> [log_file]${NC}"
    echo "  input_file: Path to the input file to test"
    echo "  log_file:   Optional path to log file (default: roundtrip_performance.log)"
    exit 1
fi

INPUT_FILE="$1"
LOG_FILE="${2:-roundtrip_performance.log}"

# Generate temp file names based on input file basename
INPUT_BASENAME=$(basename "$INPUT_FILE")
TEMP_CXX_BWT="temp_${INPUT_BASENAME}_cpp_bwt.txt"
TEMP_CXX_RESTORED="temp_${INPUT_BASENAME}_cpp_restored.txt"
TEMP_PY_BWT="temp_${INPUT_BASENAME}_py_bwt.txt"
TEMP_PY_RESTORED="temp_${INPUT_BASENAME}_py_restored.txt"

# Block sizes to test
BLOCK_SIZES=(64 128 256 512 1024 2048)

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo -e "${RED}Error: Input file $INPUT_FILE not found!${NC}"
    exit 1
fi

# Check if executables exist
if [ ! -f "$CXX_BWT" ]; then
    echo -e "${RED}Error: $CXX_BWT not found! Please compile it first.${NC}"
    exit 1
fi

if [ ! -f "$CXX_INVERSE_BWT" ]; then
    echo -e "${RED}Error: $CXX_INVERSE_BWT not found! Please compile it first.${NC}"
    exit 1
fi

if [ ! -f "$PY_BWT" ]; then
    echo -e "${RED}Error: $PY_BWT not found!${NC}"
    exit 1
fi

if [ ! -f "$PY_INVERSE_BWT" ]; then
    echo -e "${RED}Error: $PY_INVERSE_BWT not found!${NC}"
    exit 1
fi

# Get input file size
INPUT_SIZE=$(stat -f%z "$INPUT_FILE" 2>/dev/null || stat -c%s "$INPUT_FILE" 2>/dev/null)
echo "Input file: $INPUT_FILE"
echo "Input file size: $INPUT_SIZE bytes"
echo ""

# Initialize log file
echo "===========================================" > "$LOG_FILE"
echo "BWT Roundtrip Performance Test Results" >> "$LOG_FILE"
echo "Date: $(date)" >> "$LOG_FILE"
echo "Input file: $INPUT_FILE" >> "$LOG_FILE"
echo "Input file size: $INPUT_SIZE bytes" >> "$LOG_FILE"
echo "===========================================" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Function to run and measure performance
run_roundtrip_test() {
    local impl=$1  # "cpp" or "py"
    local block_size=$2
    local test_num=$3
    local total_tests=$4
    
    echo -e "${YELLOW}[$test_num/$total_tests] Testing $impl implementation with block size: $block_size bytes${NC}"
    
    # Clean up previous temp files
    if [ "$impl" == "cpp" ]; then
        rm -f "$TEMP_CXX_BWT" "$TEMP_CXX_RESTORED"
        BWT_CMD="$CXX_BWT"
        INV_BWT_CMD="$CXX_INVERSE_BWT"
        TEMP_BWT="$TEMP_CXX_BWT"
        TEMP_RESTORED="$TEMP_CXX_RESTORED"
    else
        rm -f "$TEMP_PY_BWT" "$TEMP_PY_RESTORED"
        BWT_CMD="python3 $PY_BWT"
        INV_BWT_CMD="python3 $PY_INVERSE_BWT"
        TEMP_BWT="$TEMP_PY_BWT"
        TEMP_RESTORED="$TEMP_PY_RESTORED"
    fi
    
    # Forward BWT
    echo "  Running forward BWT..."
    forward_start=$(date +%s.%N)
    $BWT_CMD "$INPUT_FILE" "$TEMP_BWT" "$block_size" > /dev/null 2>&1
    forward_exit_code=$?
    forward_end=$(date +%s.%N)
    forward_duration=$(echo "$forward_end - $forward_start" | bc)
    
    if [ $forward_exit_code -ne 0 ]; then
        echo -e "${RED}  Forward BWT failed with exit code $forward_exit_code${NC}"
        return 1
    fi
    
    # Get BWT output file size
    if [ -f "$TEMP_BWT" ]; then
        bwt_size=$(stat -f%z "$TEMP_BWT" 2>/dev/null || stat -c%s "$TEMP_BWT" 2>/dev/null)
    else
        bwt_size=0
        echo -e "${RED}  BWT output file not created${NC}"
        return 1
    fi
    
    # Inverse BWT
    echo "  Running inverse BWT..."
    inverse_start=$(date +%s.%N)
    $INV_BWT_CMD "$TEMP_BWT" "$TEMP_RESTORED" "$block_size" > /dev/null 2>&1
    inverse_exit_code=$?
    inverse_end=$(date +%s.%N)
    inverse_duration=$(echo "$inverse_end - $inverse_start" | bc)
    
    if [ $inverse_exit_code -ne 0 ]; then
        echo -e "${RED}  Inverse BWT failed with exit code $inverse_exit_code${NC}"
        return 1
    fi
    
    # Verify roundtrip
    if [ -f "$TEMP_RESTORED" ]; then
        restored_size=$(stat -f%z "$TEMP_RESTORED" 2>/dev/null || stat -c%s "$TEMP_RESTORED" 2>/dev/null)
        if diff -q "$INPUT_FILE" "$TEMP_RESTORED" > /dev/null 2>&1; then
            roundtrip_status="PASS"
            echo -e "${GREEN}  Roundtrip verification: PASSED${NC}"
        else
            roundtrip_status="FAIL"
            echo -e "${RED}  Roundtrip verification: FAILED${NC}"
        fi
    else
        roundtrip_status="ERROR"
        restored_size=0
        echo -e "${RED}  Restored file not created${NC}"
    fi
    
    # Calculate compression ratio
    if [ $bwt_size -gt 0 ] && [ $INPUT_SIZE -gt 0 ]; then
        compression_ratio=$(echo "scale=4; $bwt_size / $INPUT_SIZE" | bc)
    else
        compression_ratio="N/A"
    fi
    
    total_duration=$(echo "$forward_duration + $inverse_duration" | bc)
    
    # Log results
    echo "----------------------------------------" >> "$LOG_FILE"
    echo "Implementation: $impl" >> "$LOG_FILE"
    echo "Block Size: $block_size bytes" >> "$LOG_FILE"
    echo "Forward BWT:" >> "$LOG_FILE"
    echo "  Wall time: ${forward_duration}s" >> "$LOG_FILE"
    echo "  Output size: $bwt_size bytes" >> "$LOG_FILE"
    echo "Inverse BWT:" >> "$LOG_FILE"
    echo "  Wall time: ${inverse_duration}s" >> "$LOG_FILE"
    echo "  Output size: $restored_size bytes" >> "$LOG_FILE"
    echo "Total roundtrip time: ${total_duration}s" >> "$LOG_FILE"
    echo "Roundtrip: $roundtrip_status" >> "$LOG_FILE"
    echo "Compression ratio: $compression_ratio" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # Print summary
    echo "  Forward: ${forward_duration}s | Inverse: ${inverse_duration}s | Total: ${total_duration}s | Status: $roundtrip_status"
    echo ""
    
    # Clean up
    rm -f "$TEMP_BWT" "$TEMP_RESTORED"
}

# Run tests
total_tests=$((${#BLOCK_SIZES[@]} * 2))  # 2 implementations
test_num=0

for block_size in "${BLOCK_SIZES[@]}"; do
    # Test C++ implementation
    test_num=$((test_num + 1))
    run_roundtrip_test "cpp" $block_size $test_num $total_tests
    
    # Test Python implementation
    test_num=$((test_num + 1))
    run_roundtrip_test "py" $block_size $test_num $total_tests
done

echo -e "${GREEN}All tests completed! Results logged to $LOG_FILE${NC}"
echo ""
echo "Summary written to: $LOG_FILE"

