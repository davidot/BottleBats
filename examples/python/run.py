import random
import sys

startup = sys.stdin.readline()
if startup != 'game 0 vijf\n':
    sys.stderr.write('Unknown start signal:' + startup + '\n')
    print('Unknown start signal!', flush=True)
    if startup == '':
        startup = sys.stdin.readline()
        sys.stderr.write('new startup:' + startup + '\n')
    exit(0)

# sys.stderr.write('Got startup' + startup + '\n')
print('ready', flush=True)

while True:
    command = sys.stdin.readline()
    if command.startswith('died'):
        # sys.stderr.write('Oh no we died (but we might have won :)) :(\n')
        break

    if not command.startswith('turn 7'):
        sys.stderr.write('Unknown command, stopping: ' + command + '\n')
        print('fail')
        break

    base = command.strip()
    nums = [int(p) for p in command.split(' ')[1:3]]
    lines_to_read = nums[0]
    player_index = nums[1]
    # players_alive = nums[2]
    # round_number = nums[3]



    # own_hand = ''
    # while player_index >= 0:
    #     hand = sys.stdin.readline()
    #     player_index -= 1
    #     lines_to_read -= 1
    #
    # for i in range(lines_to_read):
    #     sys.stdin.readline()

    state = [sys.stdin.readline() for i in range(lines_to_read)]
    # sys.stderr.write('Got ' + str(state) + '\n')

    own_hand = state[player_index].strip()
    # if own_hand[0] == '0':
    #     sys.stderr.write('We should not be going right now!')
    #     exit(-1)
    cards = own_hand.split(' ')[1].strip()
    card_to_play_index = random.randrange(0, len(cards))
    card_to_play = cards[card_to_play_index]
    print('play ' + card_to_play, flush=True)
