
startup = sys.stdin.readline()
if startup != 'game 0 vijf\n':
    sys.stderr.write('Unknown start signal:' + startup + '\n')
    print('Unknown start signal!', flush=True)

print('réady', flush=True)

while True:
    command = sys.stdin.readline()

    sys.stderr.write('&gt; ' + command)

    if command.startswith('died'):
            # sys.stderr.write('Oh no we died (but we might have won :)) :(\n')
            break

    if not command.startswith('turn 7'):
        sys.stderr.write('Unknown command, stopping: ' + command + '\n')
        break

    base = command.strip()
    nums = [int(p) for p in command.split(' ')[1:3]]
    lines_to_read = nums[0]
    player_index = nums[1]

    state = [sys.stdin.readline() for i in range(lines_to_read)]

    own_hand = state[player_index].strip()

    cards = own_hand.split(' ')[1].strip()

    card_to_play_index = random.randrange(0, len(cards))
    card_to_play = cards[card_to_play_index]
    print('play ' + card_to_play, flush=True)
