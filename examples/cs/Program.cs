using System;
using System.Collections.Generic;
using System.Linq;

/// <summary>
/// Uses up/down commands
/// </summary>
public static partial class Program_a1 {
    private static void Main() {
        Console.ReadLine(); // Elevated
        Console.ReadLine(); // Info
        Console.ReadLine(); // Capacity
        Console.ReadLine(); // Commands

        string[] input = Console.ReadLine()!.Split();
        Group[] groups = new Group[int.Parse(input[1])];
        Elevator[] elevators = new Elevator[int.Parse(input[2])];
        for (int i = 0; i < groups.Length; ++i)
            groups[i] = new Group(Console.ReadLine());
        for (int i = 0; i < elevators.Length; ++i)
            elevators[i] = new Elevator(Console.ReadLine(), groups);

        Console.ReadLine(); // Done
        Console.WriteLine("ready");
        Run(groups, elevators);
    }

    private static void Run(Group[] groups, Elevator[] elevators) {
        int lastTime = 0;

        while (true) {
            string line = Console.ReadLine()!;
            if (line == "stop") break;

            string[] input = line.Split();
            int time = int.Parse(input[1]);
            foreach (Elevator elevator in elevators)
                elevator.Update(time - lastTime);
            lastTime = time;

            int eventCount = int.Parse(input[2]);
            for (int i = 0; i < eventCount; ++i) {
                string[] eventLine = Console.ReadLine()!.Split();
                switch (eventLine[0]) {
                    case "timer":
                        break;
                    case "closed":
                        Elevator elevator = elevators[int.Parse(eventLine[1])];
                        elevator.isFull = eventLine[7] != "-";
                        elevator.ClearTarget();
                        if (eventLine[5] != "-")
                            elevator.FindNewTarget(eventLine[5].Split(',').Select(int.Parse));
                        groups[int.Parse(eventLine[2])].SetRequest(eventLine[3], eventLine[7]);
                        break;
                    case "request":
                        groups[int.Parse(eventLine[2])].AddRequest(eventLine[1], eventLine[3]);
                        break;
                }
            }
            Console.ReadLine(); // Done

            foreach (Group group in groups)
                group.AssignRequests();

            foreach (Elevator elevator in elevators)
                if (elevator.Command != null)
                    Console.WriteLine(elevator.Command);
            Console.WriteLine("done");
        }
    }

    public class Elevator {
        private readonly int id, speed, capacity, openTime, closeTime;
        private int height;
        private int? target;
        private bool isMovingUp;
        public string Command { get; private set; }
        public Request? handlingRequest;
        public bool isFull;
        private int targetCount, minTarget, maxTarget;

        public Elevator(string input, Group[] groups) {
            int[] arr = input.Split().Skip(1).Select(int.Parse).ToArray();
            (id, speed, capacity, openTime, closeTime) = (arr[0], arr[2], arr[3], arr[4], arr[5]);
            groups[arr[1]].AddElevator(this);
        }

        public void Update(int deltaTime) {
            Command = null;
            if (!target.HasValue) return;
            if (Math.Abs(height - target.Value) <= speed * deltaTime)
                height = target.Value;
            else
                height += speed * deltaTime * Math.Sign(target.Value - height);
        }

        public void ClearTarget() {
            target = null;
            minTarget = height;
            maxTarget = height;
            targetCount = 0;
        }

        public void SetTarget(int? to) {
            if (target == to) return;
            target = to;
            if (to != height)
                isMovingUp = to > height;
            Command = (isMovingUp ? to < maxTarget : to > minTarget) ? $"move {id} {to} {(isMovingUp ? "up" : "down")}" : $"move {id} {to}";
        }

        public void FindNewTarget(IEnumerable<int> targets) {
            int? closest = null;
            int smallestDiff = int.MaxValue;
            bool bestMatchesDirection = false;

            foreach (int newTarget in targets) {
                ++targetCount;
                minTarget = Math.Min(minTarget, newTarget);
                maxTarget = Math.Max(maxTarget, newTarget);

                bool matchesDirection = isMovingUp == newTarget > height;
                int diff = Math.Abs(newTarget - height);
                if (bestMatchesDirection
                        ? !matchesDirection || diff >= smallestDiff
                        : !matchesDirection && diff >= smallestDiff)
                    continue;
                bestMatchesDirection = matchesDirection;
                smallestDiff = diff;
                closest = newTarget;
            }
            if (closest.HasValue)
                SetTarget(closest.Value);
        }

        private int GetDistance(int to) => Math.Abs(height - to) + (openTime + closeTime) * speed;

        private int TargetDistance => targetCount * (openTime + closeTime) * speed + maxTarget - minTarget +
                                      (isMovingUp ? maxTarget - height : height - minTarget);

        public bool IsBetterThan(Elevator other, int to, bool isUp, bool isDown) {
            if (target.HasValue && target.Value != to) {
                if (isFull || targetCount >= capacity) return false;
                if (to < Math.Min(target.Value, height)) return false;
                if (to > Math.Max(target.Value, height)) return false;
                if (isMovingUp ? !isUp : !isDown) return false;
            }
            if (other == null) return true;

            int d1 = GetDistance(to) * other.speed;
            int d2 = other.GetDistance(to) * speed;
            if (d1 != d2) return d1 < d2;

            if (other.target == to) return false;
            if (target == to) return true;
            if (!other.target.HasValue) return false;
            if (!target.HasValue) return true;
            return TargetDistance * other.speed < other.TargetDistance * speed;
        }
    }

}

// basic-1,2,3,4
// koppele-1,2,3
// ruben-1,2
// meta-1,2,3,4
// reverse-1,2,3
