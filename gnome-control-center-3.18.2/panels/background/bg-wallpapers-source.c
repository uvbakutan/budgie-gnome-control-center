/* bg-wallpapers-source.c */
/*
 * Copyright (C) 2010 Intel, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */


#include "bg-wallpapers-source.h"

#include "cc-background-item.h"
#include "cc-background-xml.h"

#include <cairo-gobject.h>
#include <libgnome-desktop/gnome-desktop-thumbnail.h>
#include <gio/gio.h>

G_DEFINE_TYPE (BgWallpapersSource, bg_wallpapers_source, BG_TYPE_SOURCE)

#define WALLPAPERS_SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BG_TYPE_WALLPAPERS_SOURCE, BgWallpapersSourcePrivate))

struct _BgWallpapersSourcePrivate
{
  GnomeDesktopThumbnailFactory *thumb_factory;
  CcBackgroundXml *xml;
};


static void
load_wallpapers (gchar              *key,
                 CcBackgroundItem   *item,
                 BgWallpapersSource *source)
{
  BgWallpapersSourcePrivate *priv = source->priv;
  GtkTreeIter iter;
  GdkPixbuf *pixbuf;
  GtkListStore *store = bg_source_get_liststore (BG_SOURCE (source));
  cairo_surface_t *surface = NULL;
  gboolean deleted;
  gint scale_factor;
  gint thumbnail_height;
  gint thumbnail_width;

  g_object_get (G_OBJECT (item), "is-deleted", &deleted, NULL);

  if (deleted)
    return;

  gtk_list_store_append (store, &iter);

  scale_factor = bg_source_get_scale_factor (BG_SOURCE (source));
  thumbnail_height = bg_source_get_thumbnail_height (BG_SOURCE (source));
  thumbnail_width = bg_source_get_thumbnail_width (BG_SOURCE (source));
  pixbuf = cc_background_item_get_thumbnail (item, priv->thumb_factory,
					     thumbnail_width, thumbnail_height,
					     scale_factor);
  if (pixbuf == NULL)
    goto out;

  surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
  gtk_list_store_set (store, &iter,
                      0, surface,
                      1, item,
                      2, cc_background_item_get_name (item),
                      -1);

 out:
  g_clear_pointer (&surface, (GDestroyNotify) cairo_surface_destroy);
  if (pixbuf)
    g_object_unref (pixbuf);
}

static void
list_load_cb (GObject *source_object,
	      GAsyncResult *res,
	      gpointer user_data)
{
  cc_background_xml_load_list_finish (res);
}

static void
item_added (CcBackgroundXml    *xml,
	    CcBackgroundItem   *item,
	    BgWallpapersSource *self)
{
  load_wallpapers (NULL, item, self);
}

static void
load_default_bg (BgWallpapersSource *self)
{
  const char * const *system_data_dirs;
  char *filename;
  guint i;

  /* FIXME We could do this nicer if we had the XML source in GSettings */

  system_data_dirs = g_get_system_data_dirs ();
  for (i = 0; system_data_dirs[i]; i++) {
    filename = g_build_filename (system_data_dirs[i],
				 "gnome-background-properties",
				 "adwaita.xml",
				 NULL);
    if (cc_background_xml_load_xml (self->priv->xml, filename)) {
      g_free (filename);
      break;
    }
    g_free (filename);
  }
}

static void
bg_wallpapers_source_constructed (GObject *object)
{
  BgWallpapersSource *self = BG_WALLPAPERS_SOURCE (object);
  BgWallpapersSourcePrivate *priv = self->priv;

  G_OBJECT_CLASS (bg_wallpapers_source_parent_class)->constructed (object);

  g_signal_connect (G_OBJECT (priv->xml), "added",
		    G_CALLBACK (item_added), self);

  /* Try adding the default background first */
  load_default_bg (self);

  cc_background_xml_load_list_async (priv->xml, NULL, list_load_cb, self);
}

static void
bg_wallpapers_source_dispose (GObject *object)
{
  BgWallpapersSourcePrivate *priv = BG_WALLPAPERS_SOURCE (object)->priv;

  g_clear_object (&priv->thumb_factory);
  g_clear_object (&priv->xml);

  G_OBJECT_CLASS (bg_wallpapers_source_parent_class)->dispose (object);
}

static void
bg_wallpapers_source_init (BgWallpapersSource *self)
{
  BgWallpapersSourcePrivate *priv;

  priv = self->priv = WALLPAPERS_SOURCE_PRIVATE (self);

  priv->thumb_factory =
    gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE);
  priv->xml = cc_background_xml_new ();
}

static void
bg_wallpapers_source_class_init (BgWallpapersSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BgWallpapersSourcePrivate));

  object_class->constructed = bg_wallpapers_source_constructed;
  object_class->dispose = bg_wallpapers_source_dispose;
}

BgWallpapersSource *
bg_wallpapers_source_new (GtkWindow *window)
{
  return g_object_new (BG_TYPE_WALLPAPERS_SOURCE, "window", window, NULL);
}

