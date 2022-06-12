import sys

verbose = False

def pr(str):
    if verbose:
        sys.stderr.write(str + '\n')

def read_line():
    line = input()
    if line == '':
        pr('> [empty!]')
    else:
        pr('> ' + line)
    return line

def write_line(out):
    pr('< ' + out)
    print(out)
