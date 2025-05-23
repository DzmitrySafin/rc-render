/* babl - dynamically extendable universal pixel conversion library.
 * Copyright (C) 2005-2008, Øyvind Kolås and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef _BABL_VERSION_H
#define _BABL_VERSION_H

#if !defined(BABL_INSIDE_BABL_H) && !defined(BABL_IS_BEING_COMPILED)
#error "babl-version.h must not be included directly, include babl.h instead."
#endif


/***
 * babl version information
 *
 * These macros tell the version of babl you are compiling against.
 * babl's version number consists of three parts: major, minor and
 * micro.
 */

#define BABL_MAJOR_VERSION 0
#define BABL_MINOR_VERSION 1
#define BABL_MICRO_VERSION 113

/** 
 * babl_get_version:
 * @major: (out): The major version number
 * @minor: (out): The minor version number
 * @micro: (out): The micro version number
 *
 * Get the version information on the babl library
 */
void   babl_get_version (int *major,
                         int *minor,
                         int *micro);

#endif
