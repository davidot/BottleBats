const std = @import("std");
const mem = @import("std").mem;

const stdin = std.io.getStdIn().reader();
const stdout = std.io.getStdOut().writer();
const stderr = std.io.getStdErr().writer();

pub fn main() anyerror!void {

    var input_buffer: [256]u8 = undefined;

    const startup_command = (try stdin.readUntilDelimiterOrEof(input_buffer[0..], '\n')) orelse return;

    if (!mem.eql(u8, startup_command, "game 0 vijf")) {
        try stderr.print("invalid start command!\n", .{});
        try stdout.print("invalid start command!\n", .{});
        return;
    }
    try stdout.print("ready\n", .{});

    while (true) {
       const command = (try stdin.readUntilDelimiterOrEof(input_buffer[0..], '\n')) orelse {
           try stderr.print("End of input without died?\n", .{});
           return;
       };

       try stderr.print("Got input _{s}_\n", .{command});

       if (mem.eql(u8, command, "died"))
           break;

    }

}
