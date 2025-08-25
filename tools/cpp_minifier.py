#!/usr/bin/env python3
import argparse
import re
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description="Minify C++ source files by removing comments and unnecessary whitespace.")
    parser.add_argument('input', type=Path, help='Input C++ source file')
    parser.add_argument('-o', '--output', type=Path, default='-', help='Output minified C++ source file')
    args = parser.parse_args()

    if not args.input.exists():
        raise FileNotFoundError(f"Input file {args.input} does not exist.")
    
    with open(args.input, 'r', encoding='utf-8') as f:
        code = f.read()

    # Replace include directives with a file content
    def replace_includes(match):
        include_file = match.group(1)
        include_path = args.input.parent / include_file
        if include_path.exists():
            with open(include_path, 'r', encoding='utf-8') as inc_f:
                return inc_f.read()
        else:
            raise FileNotFoundError(f"Included file {include_file} not found.")
    code = re.sub(r'^#include\s+"([^"]+)"', replace_includes, code, flags=re.MULTILINE)
    # Remove STL includes and other directives
    code = re.sub(r'^#.*', '', code, flags=re.MULTILINE)
    # Remove single-line comments
    code = re.sub(r'//.*', '', code)
    # Remove multi-line comments
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
    # Remove linebreaks
    code = code.replace('\n', ' ')
    # Remove spaces before and after certain characters
    code = re.sub(r'\s*([-=,;<>\+\{\}\(\)\\\*\|/:%\[\]])\s*', r'\1', code)
    # Collapse multiple spaces into one
    code = re.sub(r'\s+', ' ', code)
    # Trim leading and trailing spaces
    code = code.strip()
    # Add bits/stdc++.h include at the top
    code = '#include<bits/stdc++.h>\n' + code

    if args.output == Path('-'):
        print(code)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, 'w', encoding='utf-8') as out_f:
            out_f.write(code)
        print(f"Minified code written to {args.output}")


if __name__ == "__main__":
    main()