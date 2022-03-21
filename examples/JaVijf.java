import java.util.Scanner;

public class JaVijf {

public static void main(String[] args) {
    Scanner scanner = new Scanner(System.in);

    String startupLine = scanner.nextLine();

    if (!startupLine.equals("game 0 vijf")) {
        System.err.println("Wrong game startup: _" + startupLine + "_");
        System.exit(1);
        return;
    }

    System.out.println("ready");

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

        System.out.println("play " + handParts[1].charAt(0));
        System.out.flush();
    }

}

}
