using System;
using System.Collections.Generic;
using System.Linq;

public static partial class Program_a1 {

public class Group {
    private readonly List<Elevator> elevators = new List<Elevator>();
    private readonly List<Request> requests = new List<Request>();
    private readonly int[] floors;

    public Group(string input) {
        floors = input.Split()[3].Split(',').Select(int.Parse).ToArray();
    }

    public void AddElevator(Elevator elevator) => elevators.Add(elevator);

    public void SetRequest(string height, string direction) {
        Request request = new Request(height, direction);
        int index = requests.IndexOf(request);
        if (direction != "-") {
            if (index != -1)
                requests[index] = request;
            else
                requests.Add(request);
        } else if (index != -1)
            requests.RemoveAt(index);
    }

    public void AddRequest(string height, string direction) {
        Request request = new Request(height, direction);
        int index = requests.IndexOf(request);
        if (index == -1)
            requests.Add(request);
        else
            requests[index] |= request;
    }

    public void AssignRequests() {
        foreach (Elevator elevator in elevators)
            elevator.handlingRequest = null;
        foreach (Request request in requests)
            request.FindHandler(elevators);
    }
}

public readonly struct Request : IEquatable<Request> {
    private readonly int height;
    private readonly bool isUp, isDown;

    public Request(string height, string direction) {
        this.height = int.Parse(height);
        isUp = direction.Contains("up");
        isDown = direction.Contains("down");
    }

    public static Request operator |(Request a, Request b) => new Request(a, b);
    private Request(Request a, Request b) {
        height = a.height;
        isUp = a.isUp || b.isUp;
        isDown = a.isDown || b.isDown;
    }

    public bool Equals(Request other) => height == other.height;

    public void FindHandler(List<Elevator> elevators) {
        Elevator handler = null;
        foreach (Elevator elevator in elevators)
            if (elevator.IsBetterThan(handler, height, isUp, isDown))
                handler = elevator;
        if (handler == null) return;

        Request? previous = handler.handlingRequest;
        handler.handlingRequest = this;
        handler.SetTarget(height);
        previous?.FindHandler(elevators);
    }
}
}
