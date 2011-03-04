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

#include "pinpoint.h"
#include "pinpoint-main.h"

PinPointRenderer *pp_clutter_renderer (void);

static void
pinpoint_free_renderer (PinPointRenderer *renderer)
{
  if (renderer->source)
    g_free (renderer->source);
  renderer->finalize (renderer);
  g_free (renderer);
}

static void
pinpoint_free_data (PinPointData *data)
{
  g_list_free (data->pp_slides);
  g_free (data->pp_output_filename);
  g_free (data);
}

static ClutterActor *
pinpoint_new_internal (const gchar *file,
                       const gchar *text)
{
  gchar *text_alloc;
  PinPointData *data;
  PinPointRenderer *renderer;
  ClutterLayoutManager *layout;

  data = g_new0 (PinPointData, 1);
  text_alloc = NULL;

  renderer = pp_clutter_renderer ();

  if (file && !text)
    {
      if (!g_file_get_contents (file, &text_alloc, NULL, NULL))
        {
          g_warning ("failed to load presentation from %s\n", file);
          return NULL;
        }

      data->pp_output_filename = g_strdup (file);
      text = text_alloc;
    }

  /* Setup the container with size-binding */
  layout = clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_FIXED,
                                   CLUTTER_BIN_ALIGNMENT_FIXED);

  data->pp_container = clutter_box_new (layout);
  clutter_actor_set_clip_to_allocation (data->pp_container, TRUE);

  renderer->init (renderer, data->pp_output_filename, data);

  pp_parse_slides (renderer, text);
  g_free (text_alloc);

  renderer->run (renderer);

  g_object_weak_ref (G_OBJECT (data->pp_container),
                     (GWeakNotify)pinpoint_free_renderer,
                     renderer);
  g_object_weak_ref (G_OBJECT (data->pp_container),
                     (GWeakNotify)pinpoint_free_data,
                     data);

  return data->pp_container;
}

ClutterActor *
pinpoint_new_from_file (const gchar *file)
{
  return pinpoint_new_internal (file, NULL);
}

ClutterActor *
pinpoint_new_from_text (const gchar *text)
{
  return pinpoint_new_internal (NULL, text);
}

