/*
 * Pinpoint: A small-ish presentation tool
 *
 * Copyright (C) 2011 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option0 any later version.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Written by: Øyvind Kolås <pippin@linux.intel.com>
 *             Damien Lespiau <damien.lespiau@intel.com>
 *             Emmanuele Bassi <ebassi@linux.intel.com>
 *             Chris Lord <chris@linux.intel.com>
 */

#ifndef __PINPOINT_H__
#define __PINPOINT_H__

#include <clutter/clutter.h>

ClutterActor *pinpoint_new_from_file (const gchar *file);
ClutterActor *pinpoint_new_from_text (const gchar *text);

#endif

