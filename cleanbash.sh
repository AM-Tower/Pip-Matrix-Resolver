#!/usr/bin/env bash
set -euo pipefail
# 
# Find all .sh files in current directory (non-recursive)
for f in ./*.sh; do
    # Skip if no .sh files exist
    [ -e "$f" ] || continue

    # Skip the current script to prevent it from running on itself
    if [[ "$f" == "$0" ]]; then
        echo "Skipping execution script: $f"
        continue
    fi

    echo "Converting line endings: $f"
    dos2unix "$f"

    echo "Running shellcheck: $f"
    shellcheck "$f"

    echo "Making executable: $f"
    chmod +x "$f"
done
# End of script cleanbash.sh #
