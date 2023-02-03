const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const exe = b.addExecutable("demwm", null);
    exe.addCSourceFiles(&.{ "demwm.c", "util.c", "drw.c" }, &.{ "-Wall", "-DVERSION=\"1.0\"" });
    exe.linkLibC();
    exe.addIncludePath("/usr/X11R6/include");
    exe.addLibraryPath("/usr/X11R6/lib");
    exe.linkSystemLibrary("imlib2");
    exe.linkSystemLibrary("x11-xcb");
    exe.linkSystemLibrary("xcb");
    exe.linkSystemLibrary("xcb-res");
    exe.linkSystemLibrary("fontconfig");
    exe.linkSystemLibrary("Xft");
    exe.linkSystemLibrary("Xrender");
    exe.linkSystemLibrary("X11");
    exe.linkSystemLibrary("Xinerama");
    exe.install();
}
