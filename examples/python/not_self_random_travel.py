import random
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
    print(out, flush=True)

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

rand = random.Random()


def random_floor():
    return str(floors[rand.randrange(0, len(floors))])

write_line('ready')

in_response = False

last_floor = None

while True:
    next_line = read_line()

    if next_line == 'stop':
        break

    if next_line.startswith('events 0'):
        last_floor = random_floor()
        write_line('move 0 ' + last_floor)
        write_line('set-timer 100')

    if next_line == 'done':
        write_line('done')
        in_response = False
    else:
        parts = next_line.split(' ')
        if not in_response:
            floor = random_floor()
            while floor == last_floor:
                pr('in while!')
                floor = random_floor()
            last_floor = floor
            write_line('move 0 ' + last_floor)
        in_response = True
