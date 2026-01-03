const std = @import("std");

// Note: Windows native is not supported (tree requires POSIX).
// Use Cygwin for Windows builds.

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const bin_name = switch (target.result.os.tag) {
        .windows => "tree.exe",
        .openbsd => "colortree",
        else => "tree",
    };
    const exe = createExecutable(b, target, optimize, bin_name);
    b.installArtifact(exe);

    makeRunStep(b, exe);
}

fn createExecutable(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    bin_name: []const u8,
) *std.Build.Step.Compile {
    const common_sources = [_][]const u8{
        "tree.c",
        "list.c",
        "hash.c",
        "color.c",
        "file.c",
        "filter.c",
        "info.c",
        "unix.c",
        "xml.c",
        "json.c",
        "html.c",
    };

    var sources_buf: [12][]const u8 = undefined;
    var num_sources: usize = 0;

    for (common_sources) |src| {
        sources_buf[num_sources] = src;
        num_sources += 1;
    }

    // Conditionally include strverscmp.c
    // Only include strverscmp.c if not Linux or if Android
    const needs_strverscmp = target.result.os.tag != .linux or target.result.abi == .android;
    if (needs_strverscmp) {
        sources_buf[num_sources] = "strverscmp.c";
        num_sources += 1;
    }

    const exe = b.addExecutable(.{
        .name = bin_name,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    const sources = sources_buf[0..num_sources];
    const cflags = getCFlags(b, optimize, target.result.os.tag);
    exe.addCSourceFiles(.{
        .files = sources,
        .flags = cflags,
    });
    addPreprocessorDefines(exe, target);

    exe.linkLibC();

    // Strip symbols in release mode (equivalent to -s linker flag)
    if (exe.root_module.optimize != .Debug) {
        exe.root_module.strip = true;
    }

    return exe;
}

fn getCFlags(
    b: *std.Build,
    optimize: std.builtin.OptimizeMode,
    os_tag: std.Target.Os.Tag,
) []const []const u8 {
    // Maximum possible flags: 7 base + 1 opt + 1 platform + 1 macos = 10
    var flags_buf: [10][]const u8 = undefined;
    var flags_count: usize = 0;

    // Base flags for all platforms
    const base_flags = [_][]const u8{
        "-std=c11",
        "-Wpedantic",
        "-Wall",
        "-Wextra",
        "-Wstrict-prototypes",
        "-Wshadow",
        "-Wconversion",
    };

    for (base_flags) |flag| {
        flags_buf[flags_count] = flag;
        flags_count += 1;
    }

    // Optimization flags
    switch (optimize) {
        .Debug => {
            flags_buf[flags_count] = "-ggdb";
            flags_count += 1;
        },
        .ReleaseFast, .ReleaseSafe, .ReleaseSmall => {
            // Linux uses -O3, others use -O2
            const opt_flag = if (os_tag == .linux) "-O3" else "-O2";
            flags_buf[flags_count] = opt_flag;
            flags_count += 1;
        },
    }

    // Platform-specific flags
    switch (os_tag) {
        .freebsd, .openbsd, .macos => {
            flags_buf[flags_count] = "-fomit-frame-pointer";
            flags_count += 1;
        },
        else => {},
    }

    if (os_tag == .macos) {
        flags_buf[flags_count] = "-no-cpp-precomp";
        flags_count += 1;
    }

    const flags = flags_buf[0..flags_count];

    // Need to allocate a persistent slice since local array will go out of scope
    const persistent_flags = b.allocator.alloc([]const u8, flags.len) catch @panic("OOM");
    @memcpy(persistent_flags, flags);
    return persistent_flags;
}

fn addPreprocessorDefines(exe: *std.Build.Step.Compile, target: std.Build.ResolvedTarget) void {
    // Universal defines for large file support
    exe.root_module.addCMacro("LARGEFILE_SOURCE", "");
    exe.root_module.addCMacro("_FILE_OFFSET_BITS", "64");

    // Platform-specific defines
    const os_tag = target.result.os.tag;
    switch (os_tag) {
        .linux => {
            exe.root_module.addCMacro("_GNU_SOURCE", "");
        },
        .solaris, .illumos => {
            exe.root_module.addCMacro("_XOPEN_SOURCE", "500");
            exe.root_module.addCMacro("_POSIX_C_SOURCE", "200112");
        },
        else => {},
    }

    // Android-specific
    if (target.result.abi == .android) {
        exe.root_module.addCMacro("_LARGEFILE64_SOURCE", "");
    }
}

fn makeRunStep(b: *std.Build, exe: *std.Build.Step.Compile) void {
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    // Allow passing arguments
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the tree command");
    run_step.dependOn(&run_cmd.step);
}
