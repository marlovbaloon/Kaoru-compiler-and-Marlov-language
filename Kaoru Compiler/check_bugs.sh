#!/bin/bash

# Configuration
INPUT_FILE="${1:-disasm.txt}"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: File '$INPUT_FILE' not found!"
    echo "Usage: ./check_bugs.sh [path_to_disasm.txt]"
    exit 1
fi

echo "=================================================="
echo " Kaoru Compiler Assembly Bug Analyzer"
echo " Target File: $INPUT_FILE"
echo "=================================================="
echo ""

# 1. Check for unaligned Stack Frame Allocations (Windows x64 ABI requires 16-byte alignment)
echo "[1] Checking Unaligned Stack Allocations (sub rsp, N where N % 16 != 0)..."
grep -E "sub\s+\$0x[0-9a-f]+,%rsp" "$INPUT_FILE" | while read -r line; do
    hex_val=$(echo "$line" | grep -oP '0x[0-9a-f]+')
    if [ ! -z "$hex_val" ]; then
        dec_val=$((hex_val))
        if [ $((dec_val % 16)) -ne 0 ]; then
            echo "  ⚠️  WARNING (Stack Unaligned): $line (Size: $dec_val bytes)"
        fi
    fi
done

echo ""
# 2. Check for potential Sign Extension bugs on 1-byte/char types
echo "[2] Checking Byte Loads (movzbl vs movsbl - potential sign extension issues)..."
grep -n -E "movzbl|movsbl|movb" "$INPUT_FILE" | head -n 15

echo ""
# 3. Check for Dangerous Division Instructions (Missing cdq/cqo before idiv)
echo "[3] Checking Division Logic (idiv without preceding cqo/cdq sign-extend)..."
grep -n -B 2 -E "\bidiv\b" "$INPUT_FILE"

echo ""
# 4. Check Win64 Shadow Space Allocations (Missing 32-byte shadow space before call)
echo "[4] Checking Win64 Calling Convention (Functions making 'call' without 32B shadow space)..."
grep -n -B 5 -E "\bcall\b" "$INPUT_FILE" | grep -E "sub\s+\$0x20,%rsp|sub\s+\$0x[0-9a-f]+,%rsp" | head -n 10

echo ""
# 5. Check for Null Pointer Dereferences / Zero Displacement Operations
echo "[5] Checking Null Displacement Operations (e.g. mov (%rax), ...)..."
grep -n -E "mov\s+\(%r[a-z0-9]+\)," "$INPUT_FILE" | head -n 10

echo ""
echo "=================================================="
echo " Scan Complete. Review flagged lines in $INPUT_FILE"
echo "=================================================="
