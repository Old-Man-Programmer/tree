const std = @import("std");
const man = @import("man.zig");

// Import C main function (renamed to tree_main)
extern fn tree_main(argc: c_int, argv: [*][*:0]u8) c_int;

pub fn main() !u8 {
    const allocator = std.heap.page_allocator;
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    // Check if user wants man page
    if (args.len == 2 and std.mem.eql(u8, args[1], "man")) {
        try man.printManPage();
        return 0;
    }

    // Otherwise call into C tree implementation
    // Need to convert [:0]const u8 slice to [*:0]u8 for C compatibility
    var c_args = try allocator.alloc([*:0]u8, args.len);
    defer allocator.free(c_args);

    for (args, 0..) |arg, i| {
        c_args[i] = @constCast(arg.ptr);
    }

    const c_argc: c_int = @intCast(args.len);
    const result = tree_main(c_argc, c_args.ptr);

    return if (result >= 0) @intCast(result) else 1;
}
