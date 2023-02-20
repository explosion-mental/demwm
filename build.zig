const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const features = [_][]const u8{
        "-DXINERAMA",
        "-DICONS",
        "-DSYSTRAY",
        //"-TAG_PREVIEWS",
        //"-DDEBUG",
    };

    const args = features ++ [_][]const u8{
        "-D_POSIX_C_SOURCE=200809L",
        "-DVERSION=\"1.0\"",
        "-std=c99",
        "-pedantic",
        "-Wall",
        "-Wno-unused-function",
        "-Wno-unused-variable",
        "-march=native",
        "-Ofast",
        "-flto=auto",
        //debug
        //"-Wextra",
        //"-flto",
        //"-fsanitize=address,undefined,leak",
    };

    const libs = [_][]const u8{
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

    const exe = b.addExecutable("demwm", null);

    exe.addCSourceFiles(&.{ "demwm.c", "util.c", "drw.c" }, &args);
    exe.linkLibC();

    // INCS = -I${X11INC} -I${FREETYPEINC}
    exe.addIncludePath("/usr/X11R6/include");
    exe.addLibraryPath("/usr/X11R6/lib");

    // external libs
    for (libs) |lib| exe.linkSystemLibrary(lib);

    exe.install();
}
