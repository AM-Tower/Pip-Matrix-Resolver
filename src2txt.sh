#!/usr/bin/env bash

SRC="src"
DEST="src_text"
MERGED="source_files.txt"

mkdir -p "$DEST"
: > "$MERGED"

# Add header with date/time and AI context
{
    echo "=============================================="
    echo "PROJECT SNAPSHOT FOR AI CONTEXT"
    echo "Generated on: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "This file contains:"
    echo " - Complete source code from src/"
    echo " - Key project files (README.md, CMakeLists.txt, .qrc)"
    echo " - Directory structure in tree format"
    echo "Purpose: Provide full context for AI analysis or review."
    echo "=============================================="
    echo
} >> "$MERGED"

# Add README.md
if [[ -f "README.md" ]]; then
    echo "==============================================" >> "$MERGED"
    echo "FILE: README.md" >> "$MERGED"
    echo "==============================================" >> "$MERGED"
    cat "README.md" >> "$MERGED"
    echo >> "$MERGED"
fi

# Add CMakeLists.txt
if [[ -f "CMakeLists.txt" ]]; then
    echo "==============================================" >> "$MERGED"
    echo "FILE: CMakeLists.txt" >> "$MERGED"
    echo "==============================================" >> "$MERGED"
    cat "CMakeLists.txt" >> "$MERGED"
    echo >> "$MERGED"
fi

# Add PipMatrixResolverQt.qrc
if [[ -f "PipMatrixResolverQt.qrc" ]]; then
    cp "PipMatrixResolverQt.qrc" "$DEST/PipMatrixResolverQt.qrc.txt"
    echo "==============================================" >> "$MERGED"
    echo "FILE: PipMatrixResolverQt.qrc" >> "$MERGED"
    echo "==============================================" >> "$MERGED"
    cat "PipMatrixResolverQt.qrc" >> "$MERGED"
    echo >> "$MERGED"
fi

# Loop through src files
for file in "$SRC"/*; do
    filename=$(basename "$file")
    cp "$file" "$DEST/$filename.txt"
    echo "==============================================" >> "$MERGED"
    echo "FILE: $filename" >> "$MERGED"
    echo "==============================================" >> "$MERGED"
    cat "$file" >> "$MERGED"
    echo >> "$MERGED"
done

# Add directory tree listing
echo "==============================================" >> "$MERGED"
echo "DIRECTORY LISTING" >> "$MERGED"
echo "==============================================" >> "$MERGED"
{
    echo "."
    find . -maxdepth 1 -type f ! -path "./build*" | sort | sed 's|^\./|├── |'
    find . -type d ! -path "./build*" | sort | while read -r dir; do
        if [[ "$dir" != "." ]]; then
            echo "├── ${dir#./}"
            find "$dir" -maxdepth 1 -type f ! -path "./build*" | sort | sed 's|^\./|│   ├── |'
        fi
    done
} >> "$MERGED"

echo "[OK] All files copied to '$DEST' with .txt extension."
echo "[OK] Combined file created: '$MERGED'"