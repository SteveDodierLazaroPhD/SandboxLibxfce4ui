/* $Id$ */
/*-
 * Copyright (c) 2004-2006 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2016 Steve Dodier-Lazaro <sidi@xfce.org>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfce4ui/xfce-fadeout.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>


struct _XfceFadeout
{
  GSList *windows;
};



XfceFadeout*
xfce_fadeout_new (GdkDisplay *display)
{
#if GTK_CHECK_VERSION (3, 0, 0)
  GdkWindowAttr    attr;
  XfceFadeout     *fadeout;
  GdkWindow       *root;
  GdkCursor       *cursor;
  cairo_t         *cr;
  gint             width;
  gint             height;
  gint             n;
  GdkPixbuf       *root_pixbuf;
  cairo_surface_t *surface;
  GdkScreen       *gdk_screen;
  GdkWindow       *window;
  GdkRGBA          black = { 0, };

  fadeout = g_slice_new0 (XfceFadeout);

  cursor = gdk_cursor_new_for_display (display, GDK_WATCH);

  attr.x = 0;
  attr.y = 0;
  attr.event_mask = 0;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.window_type = GDK_WINDOW_TEMP;
  attr.cursor = cursor;
  attr.override_redirect = TRUE;

  for (n = 0; n < XScreenCount (gdk_x11_display_get_xdisplay ((display))); ++n)
    {
      gdk_screen = gdk_display_get_screen (display, n);

      root = gdk_screen_get_root_window (gdk_screen);

      width = gdk_window_get_width (root);
      height = gdk_window_get_height (root);

      attr.width = width;
      attr.height = height;
      window = gdk_window_new (root, &attr, GDK_WA_X | GDK_WA_Y
                               | GDK_WA_NOREDIR | GDK_WA_CURSOR);

      if (gdk_screen_is_composited (gdk_screen)
          && gdk_screen_get_rgba_visual (gdk_screen) != NULL)
        {
          /* transparent black window */
          gdk_window_set_background_rgba (window, &black);
          gdk_window_set_opacity (window, 0.50);
        }
      else
        {
          /* create background for window */
          surface = gdk_window_create_similar_surface (root, CAIRO_CONTENT_COLOR_ALPHA, width, height);
          cr = cairo_create (surface);

          /* make of copy of the root window */
          root_pixbuf = gdk_pixbuf_get_from_window (root, 0, 0, width, height);
          gdk_cairo_set_source_pixbuf (cr, root_pixbuf, 0, 0);
          cairo_paint (cr);
          g_object_unref (G_OBJECT (root_pixbuf));

          /* draw black layer */
          gdk_cairo_set_source_rgba (cr, &black);
          cairo_paint_with_alpha (cr, 0.50);
          cairo_destroy (cr);
          cairo_surface_destroy (surface);
        }

      fadeout->windows = g_slist_prepend (fadeout->windows, window);
    }

  /* show all windows all at once */
  g_slist_foreach (fadeout->windows, (GFunc) gdk_window_show, NULL);

  g_object_unref (cursor);

  return fadeout;
#else
  GdkWindowAttr  attr;
  XfceFadeout   *fadeout;
  GdkWindow     *root;
  GdkCursor     *cursor;
  cairo_t       *cr;
  gint           width;
  gint           height;
  gint           n;
  GdkPixbuf     *root_pixbuf;
  GdkPixmap     *backbuf;
  GdkScreen     *gdk_screen;
  GdkWindow     *window;
  GdkColor       black = { 0, };

  fadeout = g_slice_new0 (XfceFadeout);

  cursor = gdk_cursor_new (GDK_WATCH);

  attr.x = 0;
  attr.y = 0;
  attr.event_mask = 0;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.window_type = GDK_WINDOW_TEMP;
  attr.cursor = cursor;
  attr.override_redirect = TRUE;

  for (n = 0; n < gdk_display_get_n_screens (display); ++n)
    {
      gdk_screen = gdk_display_get_screen (display, n);

      root = gdk_screen_get_root_window (gdk_screen);
      gdk_drawable_get_size (GDK_DRAWABLE (root), &width, &height);

      attr.width = width;
      attr.height = height;
      window = gdk_window_new (root, &attr, GDK_WA_X | GDK_WA_Y
                               | GDK_WA_NOREDIR | GDK_WA_CURSOR);

      if (gdk_screen_is_composited (gdk_screen)
          && gdk_screen_get_rgba_colormap (gdk_screen) != NULL)
        {
          /* transparent black window */
          gdk_window_set_background (window, &black);
          gdk_window_set_opacity (window, 0.50);
        }
      else
        {
          /* create background for window */
          backbuf = gdk_pixmap_new (GDK_DRAWABLE (root), width, height, -1);
          cr = gdk_cairo_create (GDK_DRAWABLE (backbuf));

          /* make of copy of the root window */
          root_pixbuf = gdk_pixbuf_get_from_drawable (NULL, GDK_DRAWABLE (root), NULL,
                                                      0, 0, 0, 0, width, height);
          gdk_cairo_set_source_pixbuf (cr, root_pixbuf, 0, 0);
          cairo_paint (cr);
          g_object_unref (G_OBJECT (root_pixbuf));

          /* draw black layer */
          gdk_cairo_set_source_color (cr, &black);
          cairo_paint_with_alpha (cr, 0.50);
          cairo_destroy (cr);

          gdk_window_set_back_pixmap (window, backbuf, FALSE);
          g_object_unref (G_OBJECT (backbuf));
        }

      fadeout->windows = g_slist_prepend (fadeout->windows, window);
    }

  /* show all windows all at once */
  g_slist_foreach (fadeout->windows, (GFunc) gdk_window_show, NULL);

  gdk_cursor_unref (cursor);

  return fadeout;
#endif
}



void
xfce_fadeout_clear (XfceFadeout *fadeout)
{
#if !GTK_CHECK_VERSION (3, 0, 0)
  if (fadeout != NULL)
    g_slist_foreach (fadeout->windows, (GFunc) gdk_window_clear, NULL);
#else
  //TODO
#endif
}



void
xfce_fadeout_destroy (XfceFadeout *fadeout)
{
  g_slist_foreach (fadeout->windows, (GFunc) gdk_window_hide, NULL);
  g_slist_foreach (fadeout->windows, (GFunc) gdk_window_destroy, NULL);
  
  g_slist_free (fadeout->windows);
  g_slice_free (XfceFadeout, fadeout);
}
