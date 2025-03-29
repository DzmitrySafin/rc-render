# module to be used by rc-render.py

import platform

isWindows = platform.system().upper() == "WINDOWS"
isMacos = platform.system().upper() == "DARWIN"
isLinux = platform.system().upper() == "LINUX"

# constants
outputName = "rc-render"
libExt = "dll" if isWindows else "dylib" if isMacos else ""

# configurable
gimpFolder = ""
includeFolder = "include-win" if isWindows else "include-mac" if isMacos else ""

# set automatically
libFolder = "lib"
gimpResFolder = ""
gimpLibFolder = ""
