# Building

## Linux
To build for Linux you'll need `make` and `gcc` (or `clang`) installed. The rest is easy:
 > make -j9

Several build options are availble. Check the top of the `Makefile` for all the options. Multiple options can be set together. Here's a few that you may use:
### Debug builds
  > make DEBUG=1 -j9

Debug builds optionally support improved engine assertions which provide full stack traces. These stack traces require [`libunwind`](https://www.nongnu.org/libunwind/).

### Profile builds
  > make PROFILE=1 -j9
### Sanitizer builds
For debuggging purposes several sanitizer exist for POSIX platforms. Here's the table:
| Sanitizer                    | Make Variable |
|------------------------------|---------------|
| Address Sanitizer            | `ASAN`        |
| Thread Sanitizer             | `TSAN`        |
| Undefined Behavior Sanitizer | `UBSAN`       |
| Engine Sanitizer             | `ESAN`        |
Sanitizers can be used together. Here's an example:
  > make ASAN=1 TSAN=1 UBSAN=1 -j9

## FreeBSD
To build for FreeBSD you'll need `gmake` and `gcc` (or `clang`) installed. The rest is easy:
  > gmake -j9

All the build options outlined in [Linux](#linux) are available under FreeBSD too, including the Sanitizers.

Optionally, for improved stack traces in debug builds, you will need to have [devel/binutils](https://www.freshports.org/devel/binutils) installled for `addr2line`.

## Emscripten
To build for Emscripten you'll need the [Emscripten toolchain](https://emscripten.org/docs/getting_started/downloads.html) installed and set in your `PATH`. Building is the same as Linux, just set the compiler to `emcc`. Here's an example:
 > make CC=emcc -j9

With exception to Sanitizers, other than Engine Sanitizer, all the build options provided by [Linux](#linux) are supported by Emscripten builds.

## Windows
To build for Windows launch the `rex.sln` Visual Studio solution file in Visual Studio 2019 or greater.

Release builds for Windows **do not** require redistributables of the Universal C Runtime or the MSVCRT at all. The compiled binaries are free-standing and will work on any version of Windows since Windows Vista. However, optionally for improved enggine stack traces in debug builds, you'll need to have the Windows SDK installed for the DbgHelp library.