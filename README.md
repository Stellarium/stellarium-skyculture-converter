# Stellarium Sky Culture Converter

This is a command-line tool that can help convert a Stellarium sky culture in the legacy format to the new one described [here](https://github.com/Stellarium/stellarium-skycultures).

## Running

To convert a sky culture you just run the converter from the command line as follows (assuming its location is in `PATH` environment variable):
```
skyculture-converter my-sky-culture converted-sky-culture
```
where `my-sky-culture` is the path to your sky culture, `converted-sky-culture` is the directory where the new sky culture will be located.

## Building

### Linux

To build the converter you'll need the following:

 * Qt6
 * CMake
 * libgettextpo
 * A C++ compiler

On Ubuntu you can install them like so:

```shell
sudo apt install qt6-base-dev libgettextpo-dev cmake g++
```

Then as normal for a CMake-based project (substitute the path to the sources with your own path):
```shell
mkdir sc-converter-build
cd sc-converter-build
cmake /path/to/stellarium-skyculture-converter
make
```
And optionally `sudo make install` (or you can run the converter right from the build directory without installation).

### Windows

To build the converter you'll need the following:

 * Qt6
 * CMake
 * libgettextpo
 * A C++ compiler

The compiler tested is Visual Studio 2022. Qt6 and CMake are downloadable from the Internet.

To get libgettextpo you need to either build it from sources or get ready-made binaries from the releases of [this repo](https://github.com/vslavik/gettext-tools-windows). Then, to build with Visual Studio you'll need to rename the DLL to `libgettextpo.dll` (stripping the `-0` from the base name), generate a .LIB import library for linking, and a header file for compilation, which for some reason aren't available in the release package.

The following will explain how to use the binaries downloaded from the link above.

#### Preparing DLL and LIB

 1. After downloading the binary package and extracting it, say, to `c:\gettext-tools-windows-0.23.1`, rename or copy the file `libgettextpo-0.dll` to `libgettextpo.dll`, e.g. using this PowerShell command:
```PowerShell
cp c:\gettext-tools-windows-0.23.1\bin\libgettextpo-0.dll c:\gettext-tools-windows-0.23.1\bin\libgettextpo.dll
```
 2. Now prepare the LIB file by the following commands in PowerShell (assuming your path of Visual Studio tools like `C:\Program Files\Microsoft Visual Studio\2022\Community\SDK\ScopeCppSDK\vc15\VC\bin` is included in the `PATH` environment variable):
```PowerShell
echo EXPORTS > libgettextpo.def
(dumpbin /EXPORTS c:\gettext-tools-windows-0.23.1\bin\libgettextpo.dll) -match "^.*\b(po_.*)$" -replace "^.*\b(po_.*)$","`$1" >> libgettextpo.def
lib /def:libgettextpo.def /out:c:\gettext-tools-windows-0.23.1\lib\libgettextpo.lib
```

After this you should have the import library `c:\gettext-tools-windows-0.23.1\lib\libgettextpo.lib`.

#### Generating the header file

The template for a header file is available in a source package for gettext available e.g. [here](https://ftp.gnu.org/gnu/gettext/) (choose the same version as that of the binary release you downloaded). After you download it and extract, say, into `c:\gettext-0.23.1` (substitute it with your path), you can then run the following command in PowerShell:

```PowerShell
mkdir c:\gettext-0.23.1\include
(cat c:\gettext-0.23.1\gettext-tools\libgettextpo\gettext-po.in.h) -replace "extern ([^()]*);","extern __declspec (dllimport) `$1;" > c:\gettext-0.23.1\include\gettext-po.h
```

After this you should have the header `c:\gettext-0.23.1\include\gettext-po.h`.

#### CMake configuration

For CMake to find libgettextpo library the CMake command line should have an additional option: `-DCMAKE_PREFIX_PATH=c:\gettext-tools-windows-0.23.1` (again, substitute the path with yours).

If something doesn't work as you expected, or this instruction has mistakes, you can see a working AppVeyor configuration for this project [here](.appveyor.yml).
