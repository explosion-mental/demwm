const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const exe = b.addExecutable("demwm", null);
    const libs: []const u8 = &.{
        "imlib2",
        "x11-xcb",
        "xcb",
        "xcb-res",
        "fontconfig",
        "X11",
        "Xft",
        "Xrender",
        "Xinerama",
    };
    exe.addCSourceFiles(&.{ "demwm.c", "util.c", "drw.c" }, &.{ "-Wall", "-DVERSION=\"1.0\"" });
    exe.linkLibC();

    // INCS = -I${X11INC} -I${FREETYPEINC}
    exe.addIncludePath("/usr/X11R6/include");
    exe.addLibraryPath("/usr/X11R6/lib");

    // external libs
    for (libs) |lib| exe.linkSystemLibrary(lib);

    exe.install();
}
