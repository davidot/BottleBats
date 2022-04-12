Scanner scanner = new Scanner(System.in);

String startupLine = scanner.nextLine();

if (!startupLine.equals("game 0 vijf")) {
    System.err.println("Wrong game startup: _" + startupLine + "_");
    System.exit(1);
    return;
}

System.out.println("réady");

while (scanner.hasNextLine()) {
    String line = scanner.nextLine();
    if (line.startsWith("died"))
        break;

    if (!line.startsWith("turn 7")) {
        System.err.println("Weird commmand:" + line);
        System.exit(2);
        return;
    }

    String[] parts = line.split(" ");
    int lines = Integer.parseInt(parts[1]);
    int playerIndex = Integer.parseInt(parts[2]);

    String rawLines[] = new String[lines];

    int i = 0;

    while (lines-- > 0) {
        rawLines[i++] = scanner.nextLine();
    }

    String myHand = rawLines[playerIndex];
    String[] handParts = myHand.split(" ");
    if (handParts.length == 1) {
        System.err.println("Got empty hand to play?");
        System.exit(3);
        return;
    }

    char card = handParts[1].charAt(new Random().nextInt(handParts[1].length()));

    System.out.println("play " + card);
    System.out.flush();
}
