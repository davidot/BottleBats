from base import *

next_line = read_line()

floors = []

num_elevators = 1

while next_line != 'done':
    next_line = read_line()

    if next_line.startswith('building '):
        parts = next_line.split(' ')
        if int(parts[1]) > 1:
            write_line('reject only work for single groups')
            exit()
        num_elevators = int(parts[2])
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
        start_floor = floors[0]
        for i in range(num_elevators):
            write_line(f'move {i} ' + str(start_floor))
            start_floor = next_floors[next_floors[start_floor]]
        write_line('set-timer 100')

    if next_line == 'done':
        write_line('done')
    else:
        parts = next_line.split(' ')
        if parts[0] == 'closed':

            write_line(f'move {parts[1]} ' + str(next_floors[int(parts[3])]))
