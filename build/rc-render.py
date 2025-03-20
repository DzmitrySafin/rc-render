#!/usr/bin/env python3
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

isWindows = platform.system().upper() == "WINDOWS"
isMacos = platform.system().upper() == "DARWIN"
isLinux = platform.system().upper() == "LINUX"
os.chdir(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "src"))

outputName = "rc-render"
gimpFolder = "c:\\Program Files\\GIMP 2\\" if isWindows else "/Applications/GIMP.app" if isMacos else ""
gimpResFolder = ""
gimpLibFolder = ""
includeFolder = "../build/include-win" if isWindows else "../build/include-mac" if isMacos else ""
libFolder = "../build/lib"
libExt = "dll" if isWindows else "dylib" if isMacos else ""

### 1 : Configuration

if not (isWindows or isMacos):
    print(f"Platform {platform.system().upper()} is not supported")
    exit()

def printInfo(*values: object):
    if args.verbose:
        print(*values)

parser = argparse.ArgumentParser()
parser.add_argument("--gimp", help = f"GIMP application bundle location (default {gimpFolder})", metavar='<path>', type=str)
parser.add_argument("--include", help = f"headers files location (default {includeFolder})", metavar='<path>', type=str)
parser.add_argument("--symlink", help = f"do not delete created symlinks after build", action='store_true')
parser.add_argument("-v", "--verbose", help = f"use verbose output", action='store_true')
parser.add_argument("-s", "--skip", help = f"skip checking libraries, use existing symlinks", action='store_true')
parser.add_argument("--install", help = f"install built plug-in", action='store_true')
args = parser.parse_args()
printInfo(f"Command line arguments: {vars(args)}")

if (args.gimp != None):
    gimpFolder = args.gimp
if (args.include != None):
    includeFolder = args.include

gimpFolder = os.path.realpath(os.path.join(gimpFolder, "Contents") if isMacos else gimpFolder)
gimpResFolder = gimpFolder if isWindows else os.path.join(gimpFolder, "Resources") if isMacos else ""
gimpLibFolder = os.path.join(gimpResFolder, "bin" if isWindows else "lib")
includeFolder = os.path.realpath(includeFolder)
libFolder = os.path.realpath(libFolder)

printInfo(f"GIMP installation folder: {gimpFolder}")
printInfo(f"GIMP libraries folder: {gimpResFolder}")
printInfo(f"Include files folder: {includeFolder}")
printInfo(f"Libraries symlinks folder: {gimpLibFolder}")
printInfo(f"Current location: {os.getcwd()}")

if not os.path.exists(gimpFolder):
    print(f"Folder {gimpFolder} does not exist.")
    exit()
if not os.path.exists(includeFolder):
    print(f"Folder {includeFolder} does not exist.")
    exit()

if args.skip: # search existing 'lib' folder with symlinks
    printInfo("Looking for existing symlinks folder...")
    for s in "TTTTTTTTTT":
        if os.path.exists(libFolder):
            break
        libFolder = libFolder + s
    if os.path.exists(libFolder):
        printInfo("Use existing folder with symlinks:", libFolder)
    else:
        print("Could not find folder with symlinks:", libFolder)
        exit()
else: # create 'lib' folder for symlinks
    while os.path.exists(libFolder):
        libFolder = libFolder + "T"
    printInfo("Create temporary folder for symlinks:", libFolder)
    os.mkdir(libFolder)

### 2 : Compilation

includes = [
    "",
    "gimp-2.0",
    "gegl-0.4",
    "gio-unix-2.0" if isMacos else "gio-win32-2.0" if isWindows else "",
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
    "gtk-quartz-2.0" if isMacos else "gtk-win32-2.0" if isWindows else "",
    "gdk-quartz-2.0" if isMacos else "gdk-win32-2.0" if isWindows else "",
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
    #
    #"imm32",
    #"shell32",
    #"ole32",
    ##"Wl",
    #"uuid",
    ##"pangowin32-1.0",
    #"gdi32",
    #"msimg32",
    #
    #"iconv"
]

sources = [
    "rc-render-main",
    "rc-render-ui",
    "rc-render-json",
    "rc-render-combinations",
    "rc-render-bg"
]

if not args.skip: # create symlinks to libraries in application bundle
    dylibFiles = glob.glob(os.path.join(gimpLibFolder, f"lib*.{libExt}"))
    for libName in libs:
        if len(libName) == 0:
            continue;
        found = False
        for fullName in dylibFiles:
            if os.path.isfile(fullName) and os.path.basename(fullName).find(libName) == 3:
                found = True
                os.symlink(fullName, os.path.join(libFolder, f"lib{libName}.{libExt}"))
                break
        if not found:
            print(f"Could not find library {libName} in {gimpLibFolder}")
            exit()

if isWindows:
    completed = subprocess.run(f"windres rc-render-res.rc ../build/rc-render-res.o", stderr=subprocess.STDOUT, shell=True)
    if completed.returncode != 0:
        print("Failed to compile resource file with code ", completed.returncode)
        exit()

cmd = [
    "gcc" if isWindows else "/usr/bin/clang" if isMacos else "",
    " ".join(f"-I\"{os.path.join(includeFolder, name).replace(f'{chr(92)}', '/')}\"" for name in includes),
    "-mwindows" if isWindows else "",
    f"-L\"{libFolder.replace(f'{chr(92)}', '/')}\"",
    "-framework Cocoa" if isMacos else "",
    " ".join(f"-l{name}" for name in libs if len(name) > 0),
    f"-rpath '{gimpResFolder}'" if isMacos else "",
    " ".join(f"{name}.c" for name in sources),
    "../build/rc-render-res.o" if isWindows else "",
    f"-o ../build/{outputName}"
]
clang = " ".join(name for name in cmd if len(name) > 0)
printInfo("Run clang command:", clang)

completed = subprocess.run(clang, stderr=subprocess.STDOUT, shell=True)
if completed.returncode != 0:
    print("Compilation process failed with code ", completed.returncode)
    exit()

if not args.symlink and not args.skip:
    printInfo("Delete folder with symlinks:", libFolder)
    shutil.rmtree(libFolder)
else:
    printInfo("Keep folder with symlinks:", libFolder)

### 3 : Installation

def parseGimprc(filename: str):
    with open(filename) as rcFile:
        return re.findall(f"\\(plug-in-path\\s+\"(.+)\"\\)", rcFile.read())

if args.install:
    gimpVersion = ""
    gimprcFullname = ""
    pathList = []

    # detect installed GIMP version
    if isMacos:
        plistFilename = os.path.join(gimpFolder, "Info.plist")
        with open(plistFilename, "br") as plFile:
            pl = plistlib.load(plFile)
            if "CFBundleVersion" in pl:
                gimpVersion = pl["CFBundleVersion"]
        printInfo("GIMP version", gimpVersion, "extracted from the file", plistFilename)

    if isWindows:
        #TODO: detect installed version on Windows
        gimpVersion = "2.10.38"

    # search file 'gimprc' which contain GIMP preferences
    if isMacos:
        ver = ".".join(gimpVersion.split(".")[:2]) # 2.10
        gimprcFullname = os.path.join(os.environ['HOME'], "Library", "GIMP", ver, "gimprc")
        if not os.path.exists(gimprcFullname):
            printInfo("File", gimprcFullname, "does not exist. Checking another one...")
            gimprcFullname = os.path.join(os.environ['HOME'], "Library", "Application Support", "GIMP", ver, "gimprc")

    if isWindows:
        ver = ".".join(gimpVersion.split(".")[:2]) # 2.10
        gimprcFullname = os.path.join(os.environ['APPDATA'], "GIMP", ver, "gimprc")

    #TODO: have to detect folder properly (substitute GIMP variables)
    pluginFolder = os.path.join(os.path.dirname(gimprcFullname), "plug-ins")

    # search plug-ins path
    if os.path.exists(gimprcFullname):
        printInfo("File gimprc found:", gimprcFullname)
        pathList = parseGimprc(gimprcFullname)
        printInfo("plug-in-path values:", pathList)
    else:
        printInfo("File", gimprcFullname, "does not exist. Checking application bundle...")

    if not pathList:
        ver = gimpVersion.split(".")[0] + ".0" # 2.0
        gimprcFullname = os.path.join(gimpResFolder, "etc", "gimp", ver, "gimprc")
        if os.path.exists(gimprcFullname):
            printInfo("File gimprc found:", gimprcFullname)
            pathList = parseGimprc(gimprcFullname)
            printInfo("plug-in-path values:", pathList)
        else:
            printInfo("File", gimprcFullname, "does not exist.")

    if not pathList:
        printInfo("Could not find plug-in-path values in the gimprc")
        exit()

    # extract 'plug-in-path' location
    # pluginFolder = None
    # sep = "\\\\/" if isWindows else "/"
    # for line in pathList:
    #     match = re.search("\\$\\{gimp_dir\\}" + f"[{sep}](.+)", line)
    #     if match != None:
    #         pluginFolder = os.path.join(os.path.dirname(gimprcFullname), match.group(1))
    #         break
    if pluginFolder == None:
        print("Could not parse user's plug-in-path")
        exit()
    printInfo("Plug-in will be installed into", pluginFolder)

    # create plug-in sub-folder
    if not os.path.exists(pluginFolder):
        os.mkdir(pluginFolder)
    pluginFolder = os.path.join(pluginFolder, outputName)
    if not os.path.exists(pluginFolder):
        os.mkdir(pluginFolder)

    # copy plug-in binary file
    pluginFilename = outputName + (".exe" if isWindows else "")
    pluginFullname = os.path.join(pluginFolder, pluginFilename)
    shutil.copy(os.path.join(os.path.abspath("../build"), pluginFilename), pluginFullname)
    printInfo("Plug-in file has been installed:", pluginFullname)

print("Done!")
