/*
 * Pinpoint: A small-ish presentation tool
 *
 * Copyright (C) 2010 Intel Corporation
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#ifdef USE_CLUTTER_GST
#include <clutter-gst/clutter-gst.h>
#endif

#include "pinpoint-main.h"

static PinPointData data = { 0, };

static GOptionEntry entries[] =
{
    { "maximized", 'm', 0, G_OPTION_ARG_NONE, &data.pp_maximized,
    "Maximize without window decoration, instead\n"
"                                         of fullscreening, this is useful\n"
"                                         to enable window management when running\n"
"                                         [command=] spawned apps.", NULL},
    { "fullscreen", 'f', 0, G_OPTION_ARG_NONE, &data.pp_fullscreen,
    "Start in fullscreen mode", NULL},
    { "output", 'o', 0, G_OPTION_ARG_STRING, &data.pp_output_filename,
      "Output presentation to FILE\n"
"                                         (formats supported: pdf)", "FILE" },
    { NULL }
};

PinPointRenderer *pp_clutter_renderer (void);
#ifdef HAVE_PDF
PinPointRenderer *pp_cairo_renderer   (void);
#endif

static void pp_set_fullscreen (gboolean fullscreen)
{
  static gboolean is_fullscreen = FALSE;
  static gfloat old_width=640, old_height=480;

  struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
  } MWMHints = { 2, 0, 0, 0, 0};

  Display      *xdisplay = clutter_x11_get_default_display ();
  int           xscreen  = clutter_x11_get_default_screen ();
  Atom          wm_hints = XInternAtom(xdisplay, "_MOTIF_WM_HINTS", True);
  ClutterStage *stage    = CLUTTER_STAGE (clutter_stage_get_default ());
  Window        xwindow  = clutter_x11_get_stage_window (stage);

  if (!data.pp_maximized)
    return clutter_stage_set_fullscreen (stage, fullscreen);

  data.pp_fullscreen = fullscreen;
  if (is_fullscreen == fullscreen)
    return;

  is_fullscreen = fullscreen;

  if (fullscreen)
    {
      int full_width = DisplayWidth (xdisplay, xscreen);
      int full_height = DisplayHeight (xdisplay, xscreen)+5; /* avoid being detected as fullscreen */
      clutter_actor_get_size (CLUTTER_ACTOR (stage), &old_width, &old_height);

      if (wm_hints != None)
        XChangeProperty (xdisplay, xwindow, wm_hints, wm_hints, 32,
                         PropModeReplace, (guchar*)&MWMHints,
                         sizeof(MWMHints)/sizeof(long));
      clutter_actor_set_size (CLUTTER_ACTOR (stage), full_width, full_height);
      XMoveResizeWindow (xdisplay, xwindow,
                         0, 0, full_width, full_height);
    }
  else
    {
      MWMHints.decorations = 7;
      if (wm_hints != None )
        XChangeProperty (xdisplay, xwindow, wm_hints, wm_hints, 32,
                         PropModeReplace, (guchar*)&MWMHints,
                         sizeof(MWMHints)/sizeof(long));
      clutter_stage_set_fullscreen (stage, FALSE);
      clutter_actor_set_size (CLUTTER_ACTOR (stage), old_width, old_height);
    }
}

static gboolean pp_get_fullscreen ()
{
  if (!data.pp_maximized)
    return clutter_stage_get_fullscreen (CLUTTER_STAGE (clutter_stage_get_default ()));
  return data.pp_fullscreen;
}

static gboolean
key_pressed (ClutterActor *actor,
             ClutterEvent *event)
{
  if (event && (event->type == CLUTTER_KEY_PRESS)) /* There is no event for the first triggering */
    switch (clutter_event_get_key_symbol (event))
      {
        case CLUTTER_Escape:
          clutter_main_quit ();
          return TRUE;
        case CLUTTER_F11:
          pp_set_fullscreen (!pp_get_fullscreen ());
          return TRUE;
      }

  return FALSE;
}

int
main (int    argc,
      char **argv)
{
  PinPointRenderer *renderer;
  GOptionContext *context;
  GError *error = NULL;
  char   *text = NULL;
  gboolean use_clutter = TRUE;

  renderer = pp_clutter_renderer ();

  context = g_option_context_new ("- Presentations made easy");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, cogl_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      return EXIT_FAILURE;
    }

  if (!argv[1])
    {
      g_print ("usage: %s [options] <presentation>\n", argv[0]);
      text = g_strdup ("[no-markup][transition=sheet][red]\n--\nusage: pinpoint [options] <presentation.txt>\n");
    }
  else
    {
      if (!g_file_get_contents (argv[1], &text, NULL, NULL))
        {
          g_print ("failed to load presentation from %s\n", argv[1]);
          return -1;
        }
    }

#ifdef USE_CLUTTER_GST
  clutter_gst_init (&argc, &argv);
#else
  clutter_init (&argc, &argv);
#endif
#ifdef USE_DAX
  dax_init (&argc, &argv);
#endif

  /* select the cairo renderer if we have requested pdf output */
  if (data.pp_output_filename &&
      g_str_has_suffix (data.pp_output_filename, ".pdf"))
    {
#ifdef HAVE_PDF
      renderer = pp_cairo_renderer ();
      use_clutter = FALSE;
#else
      g_warning ("Pinpoint was built without PDF support");
      return EXIT_FAILURE;
#endif
    }
  else
    {
      /* Setup the stage and container with size-binding if we're
       * in 'normal' (Clutter) mode.
       */
      ClutterConstraint *bind_width, *bind_height;

      ClutterLayoutManager *layout =
        clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_FIXED,
                                CLUTTER_BIN_ALIGNMENT_FIXED);

      ClutterActor *stage = clutter_stage_get_default ();

      clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
      g_signal_connect (stage, "captured-event",
                        G_CALLBACK (key_pressed), NULL);

      data.pp_container = clutter_box_new (layout);
      clutter_actor_set_clip_to_allocation (data.pp_container, TRUE);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                   data.pp_container);
      clutter_actor_grab_key_focus (data.pp_container);

      bind_width = clutter_bind_constraint_new (stage, CLUTTER_BIND_WIDTH, 0);
      bind_height = clutter_bind_constraint_new (stage, CLUTTER_BIND_HEIGHT, 0);
      clutter_actor_add_constraint (data.pp_container, bind_width);
      clutter_actor_add_constraint (data.pp_container, bind_height);

      if (data.pp_fullscreen)
        pp_set_fullscreen (TRUE);
    }

  renderer->init (renderer, argv[1], &data);

  pp_parse_slides (renderer, &data, text);
  g_free (text);

  renderer->run (renderer);

  if (use_clutter)
    {
      clutter_actor_show (clutter_stage_get_default ());
      clutter_main ();
    }

  if (renderer->source)
    g_free (renderer->source);
  renderer->finalize (renderer);

  g_list_free (data.pp_slides);

  return 0;
}

