#!/usr/bin/env bash
#===============================================================================
# src2txt.sh — Export project source into a single text file for AI ingestion
#===============================================================================
# Usage:
#    ./src2txt.sh [--project NAME] [--output FILE] [--help]
#
# Description:
#    Combines README.md and all relevant source files into one text file.
#    Creates a directory tree listing and includes each file’s content.
#    Automatically backs up old output files with timestamps.
#
# Arguments:
#    --project NAME    Set project name (default: Project-Source)
#    --output  FILE    Set output filename (default: ${PROJECTNAME}.txt)
#    --exclude FILE LIST Set output foldername "Folder1" "Folder2"
#    --help            Show this help message
#
#===============================================================================

set -euo pipefail
IFS=$'\n\t'

declare -g PROJECTNAME="Project-Source"
# New declaration for the folder holding individual text file copies
declare -g SRC_TEXT_FOLDER="../src_text"
declare -g SRC_BACKUP_FOLDER="../backup" # New relative path for backup

# Add the source text folder to the default exclusions
declare -ga EXCLUDES=(".git" "build" "$SRC_BACKUP_FOLDER" "$SRC_TEXT_FOLDER")
declare SHOW_OUTPUT; SHOW_OUTPUT="false";
#
declare OUTPUT_FILE=""
declare SCRIPT_DIR; SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Removed BACKUP_DIR declaration as it is now defined by SRC_BACKUP_FOLDER
#------------------------------------------------------------------------------
# Function: show_help
#------------------------------------------------------------------------------
show_help()
{
    grep '^#' "$0" | sed 's/^#//'
    exit 0
}
#------------------------------------------------------------------------------
# Parse arguments
#------------------------------------------------------------------------------
while [[ $# -gt 0 ]]
do
    case "$1" in
        --project)
            PROJECTNAME="$2"
            shift 2
            ;;
        --output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        --exclude)
            shift
            while [[ $# -gt 0 && "$1" != --* ]]; do
                EXCLUDES+=("$1")
                shift
            done
            ;;
        --help|-h)
            show_help
            ;;
        *)
            echo "Unknown argument: $1" >&2
            echo "Use --help for usage info." >&2
            exit 1
            ;;
    esac
done

OUTPUT_FILE="${OUTPUT_FILE:-${SCRIPT_DIR}/${PROJECTNAME}.txt}"

#------------------------------------------------------------------------------
# Build the find exclusion array: ! -path "./dir" ! -path "./dir/*"
# This array holds the literal arguments for the find command.
# This must happen after argument parsing to include all user exclusions.
#------------------------------------------------------------------------------
declare -ga EXCLUDE_FIND_ARGS=()
# Add the output file name to exclusions to prevent infinite recursion
EXCLUDES+=("$(basename "$OUTPUT_FILE")")

for exc in "${EXCLUDES[@]}"; do
    # Exclude the directory itself
    EXCLUDE_FIND_ARGS+=("!" "-path" "./$exc")
    # Exclude everything under it
    EXCLUDE_FIND_ARGS+=("!" "-path" "./$exc/*")
done

#------------------------------------------------------------------------------
# Function: system_info
#------------------------------------------------------------------------------
system_info()
{
    local os=""
    local distro=""
    local version=""

    case "$(uname -s)" in
        Linux)
            os="Linux"
            if grep -qEi "(Microsoft|WSL)" /proc/version 2>/dev/null
            then
                os="WSL"
            fi
            distro="$(awk -F= '/^NAME/{print $2}' /etc/os-release | tr -d '"')"
            version="$(awk -F= '/^VERSION_ID/{print $2}' /etc/os-release | tr -d '"')"
            ;;
        Darwin)
            os="macOS"
            distro="macOS"
            version="$(sw_vers -productVersion)"
            ;;
        MINGW*|MSYS*|CYGWIN*)
            os="Windows"
            distro="MinGW"
            version="$(cmd /c ver 2>/dev/null | tr -d '\r')"
            ;;
        *)
            os="Unknown"
            distro="Unknown"
            version="?"
            ;;
    esac

    echo "# OS: $os on ${distro}: $version"
}

#------------------------------------------------------------------------------
# Function: backup_old_output
#------------------------------------------------------------------------------
backup_old_output()
{
    # Use the new relative path for backup folder
    mkdir -p "$SRC_BACKUP_FOLDER"
    if [[ -f "$OUTPUT_FILE" ]]
    then
        local timestamp
        timestamp="$(date +"%Y-%m-%d_%H-%M-%S")"

        local backup_file
        # Use the new variable for the path
        backup_file="${SRC_BACKUP_FOLDER}/$(basename "${OUTPUT_FILE%.txt}")-${timestamp}.txt"

        cp "$OUTPUT_FILE" "$backup_file"
        echo "# Backup created: $backup_file"
    fi
}

#------------------------------------------------------------------------------
# Function: print_tree
# Description:
#    Prints a Markdown-friendly directory tree with folders first, excluding
#    specified directories using the global EXCLUDE_FIND_ARGS.
#------------------------------------------------------------------------------
print_tree()
{
    echo "# Directory Structure"
    echo "#-------------------------------------------------------------------------------"

    # Internal recursive function
    _print_tree_level()
    {
        local dir="$1"
        local prefix="$2"
        local entries=()

        # Folders first (using safe array expansion)
        local find_d_command=(find "$dir" -mindepth 1 -maxdepth 1 -type d "${EXCLUDE_FIND_ARGS[@]}" )
        while IFS= read -r entry; do
            entries+=("$entry")
        done < <("${find_d_command[@]}" | LC_ALL=C sort)

        # Then files (using safe array expansion)
        local find_f_command=(find "$dir" -mindepth 1 -maxdepth 1 -type f "${EXCLUDE_FIND_ARGS[@]}" )
        while IFS= read -r entry; do
            entries+=("$entry")
        done < <("${find_f_command[@]}" | LC_ALL=C sort)

        # The subsequent loop now iterates over the already filtered 'entries' array.
        # The manual filtering block using EXCLUDES is removed as find now handles exclusion.

        local last_index=$(( ${#entries[@]} - 1 ))

        for i in "${!entries[@]}"
        do
            local path="${entries[$i]}"
            local base; base=$(basename "$path")
            local connector="├──"
            if (( i == last_index )); then
                connector="└──"
            fi

            echo "${prefix}${connector} ${base}"

            # Recurse into subdirectories
            if [[ -d "$path" ]]; then
                local next_prefix="$prefix"
                if (( i == last_index )); then
                    next_prefix+="    "
                else
                    next_prefix+="│   "
                fi
                _print_tree_level "$path" "$next_prefix"
            fi
        done
    }

    _print_tree_level "." ""
    echo
}

#------------------------------------------------------------------------------
# Function: collect_files
# Function to collect files for inclusion in the MERGED file content
# This function dynamically includes the EXCLUDE_FIND_ARGS array (ShellCheck safe).
#------------------------------------------------------------------------------
collect_files()
{
    # Construct the base command array
    local FIND_COMMAND=(
        find . -type f
    )

    # Append the dynamically built exclusion arguments (ShellCheck safe array expansion)
    FIND_COMMAND+=("${EXCLUDE_FIND_ARGS[@]}")

    # Append the inclusion criteria
    FIND_COMMAND+=(
        \(
        -name "README.md" -o
        -name "*.cpp" -o
        -name "*.h" -o
        -name "*.hpp" -o
        -name "*.ui" -o
        -name "*.qrc" -o
        -name "CMakeLists.txt"
        \)
    )

    # Execute the command from the array and pipe the result
    "${FIND_COMMAND[@]}" | LC_ALL=C sort
}
#------------------------------------------------------------------------------
# Function: write_output
#------------------------------------------------------------------------------
write_output()
{
    # Create the folder for individual file copies
    mkdir -p "$SRC_TEXT_FOLDER"

    {
        echo "#==============================================================================="
        echo "# Project: $PROJECTNAME"
        echo "# Generated on: $(date +"%Y-%m-%d %H:%M:%S")"
        system_info
        echo "# This file is used to show full source code, cmake, read me, and file locates."
        echo "#==============================================================================="
        echo
        mapfile -t files < <(collect_files)
        for f in "${files[@]}"
        do
            [[ -f "$f" ]] || continue

            # Copy the file to the new folder with the .txt suffix
            local target_file; target_file="${SRC_TEXT_FOLDER}/$(basename "$f").txt"
            cp "$f" "$target_file"

            # Append content to the main output file
            echo "#-------------------------------------------------------------------------------"
            echo "# File: ${f#./}"
            echo "#-------------------------------------------------------------------------------"
            cat "$f"
            echo
        done
        # Note: print_tree does not need "$@" as it operates on the current directory
        print_tree
        echo "#*** End of file $PROJECTNAME ***"
    } > "$OUTPUT_FILE"
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
backup_old_output
write_output

echo "Output written to: $OUTPUT_FILE"
echo "Individual file copies written to: $SRC_TEXT_FOLDER" # Added info message
if [ "$SHOW_OUTPUT" = "true" ]; then
    # Clear the screen (if you had this before)
    clear
    # Display the file with paging (fixed for SC2002: no cat | more)
    more "$OUTPUT_FILE"
fi
# End of script src2txt.sh #
