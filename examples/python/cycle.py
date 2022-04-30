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

next_line = read_line()

floors = []

while next_line != 'done':
    next_line = read_line()

    if next_line.startswith('building '):
        parts = next_line.split(' ')
        if int(parts[1]) > 1:
            write_line('reject only work for single groups')
            exit()
        group = read_line()
        group_parts = group.split(' ')
        if len(group_parts) != 4 or not group.startswith('group '):
            write_line('reject unexpected group line ' + group)
            exit()

        floors = [int(f) for f in group_parts[3].split(',')]


pr('Got floors ' + str(floors))
if len(floors) == 0:
    write_line('reject empty group? ' + str(floors))
    exit()

next_floors = {}
for i in range(len(floors) - 1):
    next_floors[floors[i]] = floors[i + 1]

next_floors[floors[-1]] = floors[0]

pr('next: ' + str(next_floors))

write_line('ready')

while True:
    next_line = read_line()

    if next_line == 'stop':
        break

    if next_line.startswith('events 0'):
        write_line('move 0 ' + str(floors[0]))
        write_line('set-timer 100')

    if next_line == 'done':
        write_line('done')
    else:
        parts = next_line.split(' ')
        if parts[0] == 'closed':
            write_line('move 0 ' + str(next_floors[int(parts[3])]))
