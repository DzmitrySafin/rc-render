#!/usr/bin/env python
# show help message: python rc-render.py -h
# on Windows: add "c:\msys64\mingw64\bin" to Path environment variable
# can be compiled with version 2.10.34 revision 2 or higher

import argparse
import os
import glob
import subprocess
import shutil
import re
import platform
import plistlib

import rc_settings as ss

if not (ss.isWindows or ss.isMacos):
    print(f"Platform {platform.system()} is not supported")
    exit()

def printInfo(*values: object):
    if args.verbose:
        print(*values)

### 1 : parse arguments

parser = argparse.ArgumentParser(
    prog = "python rc_render.py",
    description = "Build and/or install \"rc-render\" plug-in for GIMP",
    epilog =
"""
So, the simplest command to build and install plug-in: python rc_render.py -b -i
... or even just build and do not install it:          python rc_render.py -b
But we might want to keep generated set of symlinks:   python rc_render.py -k -b -i
... and later on we can use existing symlinks:         python rc_render.py -l -b -i
Just install the plug-in we built before:              python rc_render.py -i
""",
    formatter_class=argparse.RawTextHelpFormatter
)
parser.add_argument("--gimp", help = f"GIMP installation location (skip to search automatically)", metavar='<path>', type=str)
parser.add_argument("--include", help = f"headers files location (default \"{ss.includeFolder}\")", metavar='<path>', type=str)
parser.add_argument("-k", "--lib-keep", help = f"do not delete created libraries symlinks after build", action='store_true')
parser.add_argument("-l", "--lib-nocheck", help = f"skip checking libraries, use existing symlinks", action='store_true')
parser.add_argument("-b", "--build", help = f"build plug-in", action='store_true')
parser.add_argument("-i", "--install", help = f"install built plug-in", action='store_true')
parser.add_argument("-v", "--verbose", help = f"use verbose output", action='store_true')
args = parser.parse_args()
printInfo(f"Arguments: {vars(args)}")

### 2 : get GIMP location

def getGimpFolder(argValue):
    gimpFolder = ""
    if (not argValue):
        if ss.isMacos:
            gimpFolder = "/Applications/GIMP.app/Contents"
        if ss.isWindows:
            gimpFolder = "C:\\Program Files\\GIMP 3"
            if not os.path.exists(gimpFolder):
                gimpFolder = "C:\\Program Files\\GIMP 2"
        if (ss.isLinux):
            pass
    else:
        gimpFolder = re.sub(r"[/\\]", lambda _: os.sep, argValue.strip())
        gimpFolder = gimpFolder.removesuffix(os.sep).rstrip()
        if ss.isMacos:
            if not gimpFolder.startswith(os.sep):
                gimpFolder = os.sep + gimpFolder
            if gimpFolder.upper().endswith("GIMP"):
                gimpFolder = gimpFolder + ".app"
            if gimpFolder.upper().endswith("app"):
                gimpFolder = gimpFolder + "/Contents"
        if ss.isWindows:
            if gimpFolder.upper().endswith("GIMP"):
                gimpFolder = gimpFolder + " 3"
                if not os.path.exists(gimpFolder + os.sep):
                    gimpFolder = gimpFolder[:-1] + "2"
        if (ss.isLinux):
            pass
    return gimpFolder

ss.gimpFolder = getGimpFolder(args.gimp)
printInfo(f"Looking for GIMP here: {ss.gimpFolder}")
if not os.path.exists(ss.gimpFolder):
    print(f"Could not find GIMP installation folder.")
    exit()
gimpResFolder = ss.gimpFolder if ss.isWindows else os.path.join(ss.gimpFolder, "Resources") if ss.isMacos else ""
gimpLibFolder = os.path.join(gimpResFolder, "bin" if ss.isWindows else "lib")

### 3 : make sure we start from the "build" folder

userFolder = os.getcwd()
printInfo("Current directory:", userFolder)
scriptFolder = os.path.dirname(os.path.realpath(__file__))
os.chdir(scriptFolder)
printInfo("Current directory:", scriptFolder)

### 4 : get Headers location

def getHeadersFolder(argValue):
    includeFolder = ss.includeFolder
    if (argValue != None):
        includeFolder = argValue
    includeFolder = os.path.realpath(includeFolder)
    return includeFolder

ss.includeFolder = getHeadersFolder(args.include)
printInfo(f"Headers location: {ss.includeFolder}")
if args.build and not os.path.exists(ss.includeFolder):
    print(f"Could not find Headers folder.")
    exit()

### 5 : get libraries folder

def getLibrariesFolder(argValue, build):
    libFolder = os.path.realpath(argValue)
    libN = 0
    if args.lib_nocheck: # search existing 'lib' folder with symlinks
        while not os.path.exists(libFolder) and libN < 10:
            libN += 1
            libFolder = re.sub(r"(-\d+)$", f"-{libN}", libFolder)
        if os.path.exists(libFolder):
            printInfo("Use existing folder with symlinks:", libFolder)
        else:
            libFolder = None
    else: # create 'lib' folder for symlinks
        while os.path.exists(libFolder):
            libN += 1
            libFolder = re.sub(r"(-\d+)$", "", libFolder) + f"-{libN}"
        if build:
            os.mkdir(libFolder)
        printInfo("Created new folder for symlinks:", libFolder)
    return libFolder

ss.libFolder = getLibrariesFolder(ss.libFolder, args.build)
if args.build and not ss.libFolder:
    print(f"Could not set Libraries folder.")
    exit()

### 6 : lists of files required for compilation

includes = [
    "",
    "gimp-2.0",
    "gegl-0.4",
    "gio-unix-2.0" if ss.isMacos else "gio-win32-2.0" if ss.isWindows else "",
    "json-glib-1.0",
    "babl-0.1",
    "gtk-2.0",
    "pango-1.0",
    "harfbuzz",
    "fribidi",
    "cairo",
    "atk-1.0",
    "pixman-1",
    "freetype2",
    "gdk-pixbuf-2.0",
    "libpng16",
    "glib-2.0"
]

libs = [
    "gimpui-2.0",
    "gimpwidgets-2.0",
    "gimpmodule-2.0",
    "gimp-2.0",
    "gimpmath-2.0",
    "gimpconfig-2.0",
    "gimpcolor-2.0",
    "gimpbase-2.0",
    "gegl-0.4",
    "gegl-npd-0.4",
    "gmodule-2.0",
    "json-glib-1.0",
    "babl-0.1",
    "gtk-quartz-2.0" if ss.isMacos else "gtk-win32-2.0" if ss.isWindows else "",
    "gdk-quartz-2.0" if ss.isMacos else "gdk-win32-2.0" if ss.isWindows else "",
    "pangocairo-1.0",
    "pango-1.0",
    "harfbuzz",
    "atk-1.0",
    "cairo",
    "gdk_pixbuf-2.0",
    "gio-2.0",
    "gobject-2.0",
    "glib-2.0",
    "intl"
]

sources = [
    "rc-render-main",
    "rc-render-ui",
    "rc-render-json",
    "rc-render-combinations",
    "rc-render-bg"
]

### 7 : create symlinks

if args.build and not args.lib_nocheck: # create symlinks to libraries of the installed application
    dylibFiles = glob.glob(os.path.join(gimpLibFolder, f"lib*.{ss.libExt}"))
    for libName in libs:
        if len(libName) == 0:
            continue
        found = False
        for fullName in dylibFiles:
            if os.path.isfile(fullName) and os.path.basename(fullName).find(libName) == 3: # search file with prefix 'lib' + given name
                found = True
                os.symlink(fullName, os.path.join(ss.libFolder, f"lib{libName}.{ss.libExt}"))
                break
        if not found:
            print(f"Could not find library {libName} in {gimpLibFolder}")
            exit()

### 8 : compile resources (Windows only)

if args.build and ss.isWindows:
    completed = subprocess.run(f"windres ../src/rc-render-res.rc ./rc-render-res.o", stderr=subprocess.STDOUT, shell=True)
    if completed.returncode != 0:
        print("Failed to compile resource file with code ", completed.returncode)
        exit()

### 9 : set "src" as current folder

os.chdir(os.path.join(scriptFolder, "..", "src"))
printInfo("Current directory:", os.getcwd())

### 10 : compilation

cmd = [
    "gcc" if ss.isWindows else "/usr/bin/clang" if ss.isMacos else "",
    " ".join(f"-I\"{os.path.join(ss.includeFolder, name).replace(f'{chr(92)}', '/')}\"" for name in includes),
    "-mwindows" if ss.isWindows else "",
    f"-L\"{ss.libFolder.replace(f'{chr(92)}', '/')}\"",
    "-framework Cocoa" if ss.isMacos else "",
    " ".join(f"-l{name}" for name in libs if len(name) > 0),
    f"-rpath '{gimpResFolder}'" if ss.isMacos else "",
    " ".join(f"{name}.c" for name in sources),
    "../build/rc-render-res.o" if ss.isWindows else "",
    f"-o ../build/{ss.outputName}"
]
clang = " ".join(name for name in cmd if len(name) > 0)
printInfo("Run gcc/clang command:", clang)

if args.build:
    completed = subprocess.run(clang, stderr=subprocess.STDOUT, shell=True)
    if completed.returncode != 0:
        print("Compilation process failed with code ", completed.returncode)
        exit()

### 11 : delete symlinks

if args.build and not args.lib_keep and not args.lib_nocheck:
    printInfo("Delete folder with symlinks:", ss.libFolder)
    shutil.rmtree(ss.libFolder)
else:
    if args.build:
        printInfo("Keep folder with symlinks:", ss.libFolder)

### 12 : set current folder back to "build"

os.chdir(os.path.join(scriptFolder, "..", "build"))
printInfo("Current directory:", os.getcwd())

### 13 : installation

def getGimpVersion():
    version = ""
    if ss.isMacos:
        plistFilename = os.path.join(ss.gimpFolder, "Info.plist")
        with open(plistFilename, "br") as plFile:
            pl = plistlib.load(plFile)
            if "CFBundleVersion" in pl:
                version = pl["CFBundleVersion"]
    if ss.isWindows:
        cmd_path = os.path.join(ss.gimpFolder, "bin", "gimptool-2.0")
        binVersion = subprocess.check_output(f"\"{cmd_path}\" --version", stderr=subprocess.STDOUT, shell=True)
        version = binVersion.decode()
    if ss.isLinux:
        pass
    match = re.search(r"\d\.\d+.\d+", version)
    return None if match is None else match.group(0)

def getGimpRcPath(version):
    filename = ""
    ver = ".".join(version.split(".")[:2]) # 2.10
    if ss.isMacos:
        filename = os.path.join(os.environ['HOME'], "Library", "GIMP", ver, "gimprc")
        if not os.path.exists(filename):
            printInfo("File", filename, "does not exist. Checking another one...")
            filename = os.path.join(os.environ['HOME'], "Library", "Application Support", "GIMP", ver, "gimprc")
    if ss.isWindows:
        filename = os.path.join(os.environ['APPDATA'], "GIMP", ver, "gimprc")
    if ss.isLinux:
        pass
    return filename

def parseGimprc(filename: str):
    with open(filename) as rcFile:
        return re.findall(f"\\(plug-in-path\\s+\"(.+)\"\\)", rcFile.read())

def getPluginsPath(version):
    folder = ""
    ver = ".".join(version.split(".")[:2]) # 2.10
    if ss.isMacos:
        folder = os.path.join(os.environ['HOME'], "Library", "GIMP", ver)
        if not os.path.exists(folder):
            printInfo("Folder", folder, "does not exist. Checking another one...")
            folder = os.path.join(os.environ['HOME'], "Library", "Application Support", "GIMP", ver)
    if ss.isWindows:
        folder = os.path.join(os.environ['APPDATA'], "GIMP", ver)
    if ss.isLinux:
        pass
    return folder

if args.install:
    pathList = []

    gimpVersion = getGimpVersion()
    if not gimpVersion:
        print("Could not detect installed GIMP version.")
        exit()
    else:
        printInfo("Installed GIMP version detected:", gimpVersion)

    # most obvious way to get plug-ins folder
    # to make it 100% accurate we would have to parse gimprc file
    pluginFolder = os.path.join(getPluginsPath(gimpVersion), "plug-ins")
    printInfo("Plug-in will be installed into", pluginFolder)

    if not os.path.exists(pluginFolder):
        os.mkdir(pluginFolder)
    pluginFolder = os.path.join(pluginFolder, ss.outputName)
    if not os.path.exists(pluginFolder):
        os.mkdir(pluginFolder)

    pluginFilename = ss.outputName + (".exe" if ss.isWindows else "")
    pluginFullpath = os.path.join(pluginFolder, pluginFilename)
    shutil.copy(os.path.realpath(pluginFilename), pluginFullpath)
    printInfo("Plug-in file has been installed:", pluginFullpath)

os.chdir(userFolder)
print("Work complete!")
