import sys

def fix_unistd(content):
    lines = content.split('\n')
    result = []
    for i, line in enumerate(lines):
        if '#include <unistd.h>' in line:
            result.append('#ifdef _WIN32')
            result.append('#include <io.h>')
            result.append('#define isatty _isatty')
            result.append('#define fileno _fileno')
            result.append('#else')
            result.append('#include <unistd.h>')
            result.append('#endif')
        else:
            result.append(line)
    return '\n'.join(result)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: fix_unistd.py <file>")
        sys.exit(1)

    with open(sys.argv[1], 'r') as f:
        content = f.read()

    fixed = fix_unistd(content)

    with open(sys.argv[1], 'w') as f:
        f.write(fixed)

    print(f"Fixed {sys.argv[1]}")
