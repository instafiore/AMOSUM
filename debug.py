import re
import sys

def contains_patterns(line):
    # Create a combined regex pattern using lookaheads within a single regex
    combined_pattern = (
        r"(?=.*in_knapsack\(2,12\))"
        r"(?=.*in_knapsack\(1,17\))"
        r"(?=.*in_knapsack\(3,20\))"
    )
    
    # Compile the combined pattern
    regex = re.compile(combined_pattern)
    
    # Check if the line matches the pattern
    return bool(regex.search(line))

def process_file(filename):
    try:
        with open(filename, 'r') as file:
            for line in file:
                line = line.strip()
                if contains_patterns(line):
                    print(f"Matched: {line}")
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.", file=sys.stderr)
    except Exception as e:
        print(f"An error occurred: {e}", file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 script.py <filename>", file=sys.stderr)
        sys.exit(1)

    filename = sys.argv[1]
    process_file(filename)
