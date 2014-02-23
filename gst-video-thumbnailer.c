/*
 * Bickley - a meta data management framework.
 * Copyright © 2008 - 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>

#ifdef USE_CLUTTER_GST

#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gst-video-thumbnailer.h"

GdkPixbuf *
convert_buffer_to_pixbuf (GstCaps      *caps,
                          GstBuffer    *buffer,
                          GCancellable *cancellable)
{
    GstMapInfo info;
    int dw, dh;
    GstStructure *s;

    s = gst_caps_get_structure (caps, 0);
    gst_structure_get_int (s, "width", &dw);
    gst_structure_get_int (s, "height", &dh);

    if (gst_buffer_map (buffer, &info, GST_MAP_READ)) {
        gchar *data = g_memdup (info.data, info.size);
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data ((guchar *) data,
                                                      GDK_COLORSPACE_RGB, FALSE,
                                                      8, dw, dh, GST_ROUND_UP_4 (dw * 3),
                                                      (GdkPixbufDestroyNotify) g_free,
                                                      NULL);

        gst_buffer_unmap (buffer, &info);

        return pixbuf;
    }

    return NULL;
}

GdkPixbuf *
gst_video_thumbnailer_get_shot (const gchar  *location,
                                GCancellable *cancellable)
{
    GstElement *playbin, *audio_sink, *video_sink;
    GstStateChangeReturn state;
    GdkPixbuf *shot = NULL;
    int count = 0;
    gchar *uri = g_strconcat ("file://", location, NULL);
    GMainContext *context = g_main_context_new ();

    g_main_context_push_thread_default  (context);

    playbin = gst_element_factory_make ("playbin", "playbin");
    audio_sink = gst_element_factory_make ("fakesink", "audiosink");
    video_sink = gst_element_factory_make ("appsink", "videosink");

    gst_app_sink_set_caps (GST_APP_SINK (video_sink),
                           gst_caps_new_simple ("video/x-raw",
                                                "format", G_TYPE_STRING, "RGB",
                                                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                                NULL));

    g_object_set (playbin,
                  "uri", uri,
                  "audio-sink", audio_sink,
                  "video-sink", video_sink,
                  NULL);
    g_object_set (video_sink,
                  "sync", TRUE,
                  NULL);
    state = gst_element_set_state (playbin, GST_STATE_PAUSED);
    while (state == GST_STATE_CHANGE_ASYNC
           && count < 5
           && !g_cancellable_is_cancelled (cancellable)) {
        state = gst_element_get_state (playbin, NULL, 0, 1 * GST_SECOND);
        count++;

        /* Spin mainloop so we can pick up the cancels */
        while (g_main_context_pending (context)) {
            g_main_context_iteration (context, FALSE);
        }
    }

    if (g_cancellable_is_cancelled (cancellable)) {
        g_print ("Video %s was cancelled\n", uri);
        state = GST_STATE_CHANGE_FAILURE;
    }

    if (state != GST_STATE_CHANGE_FAILURE &&
        state != GST_STATE_CHANGE_ASYNC) {
        gint64 duration;

        if (gst_element_query_duration (playbin, GST_FORMAT_TIME, &duration)) {
            gint64 seekpos;
            GstSample *sample;

            if (duration > 0) {
                if (duration / (3 * GST_SECOND) > 90) {
                    seekpos = (rand () % (duration / (3 * GST_SECOND))) * GST_SECOND;
                } else if (duration > GST_SECOND) {
                    seekpos = (rand () % (duration / (GST_SECOND))) * GST_SECOND;
                } else {
                    seekpos = duration / 2;
                }
            } else {
                seekpos = 5 * GST_SECOND;
            }

            gst_element_seek_simple (playbin, GST_FORMAT_TIME,
                                     GST_SEEK_FLAG_FLUSH |
                                     GST_SEEK_FLAG_ACCURATE, seekpos);

            /* Wait for seek to complete */
            count = 0;
            state = gst_element_get_state (playbin, NULL, 0,
                                           0.2 * GST_SECOND);
            while (state == GST_STATE_CHANGE_ASYNC && count < 3) {
                state = gst_element_get_state (playbin, NULL, 0, 1 * GST_SECOND);
                count++;
            }

            sample = gst_base_sink_get_last_sample (GST_BASE_SINK (video_sink));
            if (sample == NULL) {
                g_warning ("No frame for %s", uri);
                shot = NULL;
                goto finish;
            }

            shot = convert_buffer_to_pixbuf (gst_sample_get_caps (sample),
                                             gst_sample_get_buffer (sample),
                                             cancellable);

            gst_sample_unref (sample);
        }
    }

    gst_element_set_state (playbin, GST_STATE_NULL);
    g_object_unref (playbin);
    g_free (uri);

 finish:

    g_main_context_pop_thread_default (context);
    g_main_context_unref (context);

    return shot;
}
#endif /* USE_CLUTTER_GST */
