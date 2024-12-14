#!/usr/bin/env python3

import os
import re
import argparse

# ANSI escape codes for colored output
CYAN = "\033[36m"
ORANGE = "\033[38;5;214m"
RESET = "\033[0m"
YELLOW = "\033[33m"

def clear_console():
    # Check the operating system and execute the appropriate command
    os.system('cls' if os.name == 'nt' else 'clear')

def search_string_in_files(directory, search_string, file_extensions=None, whole_word=False):
    """
    Recursively search for a string in all files within a directory and its subdirectories.

    Args:
        directory (str): The path of the directory to search.
        search_string (str): The string to search for.
        file_extensions (list, optional): List of file extensions to include (e.g., ['.txt', '.py']).
        whole_word (bool): If True, match the search string as a whole word only.
    """
    print(f"{YELLOW}Searching in: {directory}, for {search_string}{RESET}")

    # Compile a regex pattern for whole word or substring matching
    if whole_word:
        pattern = re.compile(rf'\b({re.escape(search_string)})\b', re.IGNORECASE)
    else:
        pattern = re.compile(rf'({re.escape(search_string)})', re.IGNORECASE)

    for root, _, files in os.walk(directory):  # Recursively traverse directories
        print(f"{CYAN}{root}{RESET}")
        for file in files:
            # Filter files by extensions if provided
            if file_extensions and not any(file.endswith(ext) for ext in file_extensions):
                continue

            file_path = os.path.join(root, file)
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    for line_num, line in enumerate(f, start=1):
                        match = pattern.search(line)
                        if match:  # If a match is found
                            highlighted_line = line[:match.start()] + \
                                               ORANGE + match.group(1) + RESET + \
                                               line[match.end():]
                            print(f"Found in {file_path} (Line {line_num}): {highlighted_line.strip()}")
            except Exception as e:
                print(f"Could not read {file_path}: {e}")

def main():
    parser = argparse.ArgumentParser(description="Recursively search for a string in .hpp and .cpp files.")
    parser.add_argument(
        "search_string", 
        type=str, 
        help="The string to search for."
    )
    parser.add_argument(
        "directory", 
        type=str, 
        nargs="?", 
        default=".",  # Default to the current directory
        help="The directory to search in (defaults to the current directory)."
    )
    parser.add_argument(
        "-w", "--whole-word", 
        action="store_true", 
        help="Match the search string as a whole word only."
    )

    args = parser.parse_args()

    # Only search .hpp and .cpp files
    file_extensions = ['.hpp', '.cpp']
    search_string_in_files(args.directory, args.search_string, file_extensions, args.whole_word)

if __name__ == "__main__":
    clear_console()
    main()
