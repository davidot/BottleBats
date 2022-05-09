using System;
using System.Collections.Generic;
using System.Linq;

class Program {
    private static void Main(string[] args) {
        Console.ReadLine(); // Elevated
        Console.ReadLine(); // Info
        Console.ReadLine(); // Capacity
        Console.ReadLine(); // Commands
        string[] input = Console.ReadLine().Split();
        int groups = int.Parse(input[1]);
        Elevator[] elevators = new Elevator[int.Parse(input[2])];
        input = Console.ReadLine().Split();
        int[] floors = input[3].Split(',').Select(int.Parse).ToArray();
        for (int i = 0; i < elevators.Length; ++i)
            elevators[i] = new Elevator(Console.ReadLine());
        Console.ReadLine(); // Done

        if (groups != 1) {
            Console.WriteLine("reject Too many groups!");
            return;
        }
        Console.WriteLine("ready");

        while (true) {
            string line = Console.ReadLine();
            if (line == "stop") break;
            int[] info = line.Split().Skip(1).Select(int.Parse).ToArray();
            (int time, int eventCount) = (info[0], info[1]);
            List<string> output = new();
            for (int i = 0; i < eventCount; ++i) {
                string[] eventLine = Console.ReadLine().Split();
                switch (eventLine[0]) {
                    case "timer":
                        break;
                    case "closed":
                        Elevator elevator = elevators[int.Parse(eventLine[1])];
                        int currHeight = int.Parse(eventLine[3]);
                        int[] targets = eventLine[5] == "-"
                            ? Array.Empty<int>()
                            : eventLine[5].Split(',').Select(int.Parse).ToArray();
                        bool waitingDown = eventLine[7].Contains("down");
                        bool waitingUp = eventLine[7].Contains("up");
                        if (targets.Length != 0)
                            output.Add($"move {elevator.id} {targets[0]}");
                        break;
                    case "request":
                        int height = int.Parse(eventLine[1]);
                        bool isUp = eventLine[3] == "up";
                        output.Add($"move {0} {height}");
                        break;
                }
            }
            Console.ReadLine(); // Done
            foreach (string outs in output)
                Console.WriteLine(outs);
            Console.WriteLine("done");
        }
    }

    private struct Elevator {
        public readonly int id, group, speed, capacity, openTime, closeTime;

        public Elevator(string input) {
            int[] arr = input.Split().Skip(1).Select(int.Parse).ToArray();
            (id, group, speed, capacity, openTime, closeTime) = (arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
        }
    }
}
