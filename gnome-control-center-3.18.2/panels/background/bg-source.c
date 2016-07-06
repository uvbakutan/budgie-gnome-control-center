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

#include "bg-source.h"
#include "cc-background-item.h"

#include <cairo-gobject.h>

#define THUMBNAIL_WIDTH 256
#define THUMBNAIL_HEIGHT (THUMBNAIL_WIDTH * 3 / 4)

G_DEFINE_ABSTRACT_TYPE (BgSource, bg_source, G_TYPE_OBJECT)

#define SOURCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BG_TYPE_SOURCE, BgSourcePrivate))

struct _BgSourcePrivate
{
  GtkListStore *store;
  GtkWidget *window;
  gint thumbnail_height;
  gint thumbnail_width;
};

enum
{
  PROP_LISTSTORE = 1,
  PROP_WINDOW
};


static void
bg_source_calculate_thumbnail_dimensions (BgSource *source)
{
  BgSourcePrivate *priv = source->priv;
  gint scale_factor;

  priv->thumbnail_height = THUMBNAIL_HEIGHT;
  priv->thumbnail_width = THUMBNAIL_WIDTH;

  if (priv->window == NULL)
    return;

  scale_factor = gtk_widget_get_scale_factor (priv->window);
  if (scale_factor > 1)
    {
      priv->thumbnail_height *= scale_factor;
      priv->thumbnail_width *= scale_factor;
    }
}

static void
bg_source_constructed (GObject *object)
{
  G_OBJECT_CLASS (bg_source_parent_class)->constructed (object);

  bg_source_calculate_thumbnail_dimensions (BG_SOURCE (object));
}

static void
bg_source_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  BgSource *source = BG_SOURCE (object);

  switch (property_id)
    {
    case PROP_LISTSTORE:
      g_value_set_object (value, bg_source_get_liststore (source));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bg_source_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  BgSourcePrivate *priv = BG_SOURCE (object)->priv;

  switch (property_id)
    {
    case PROP_WINDOW:
      priv->window = GTK_WIDGET (g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bg_source_dispose (GObject *object)
{
  BgSourcePrivate *priv = BG_SOURCE (object)->priv;

  g_clear_object (&priv->store);

  G_OBJECT_CLASS (bg_source_parent_class)->dispose (object);
}

static void
bg_source_class_init (BgSourceClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BgSourcePrivate));

  object_class->constructed = bg_source_constructed;
  object_class->get_property = bg_source_get_property;
  object_class->set_property = bg_source_set_property;
  object_class->dispose = bg_source_dispose;

  pspec = g_param_spec_object ("liststore",
                               "Liststore",
                               "Liststore used in the source",
                               GTK_TYPE_LIST_STORE,
                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_LISTSTORE, pspec);

  pspec = g_param_spec_object ("window",
                               "Window",
                               "Toplevel window used to view the source",
                               GTK_TYPE_WINDOW,
                               G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_WINDOW, pspec);
}

static void
bg_source_init (BgSource *self)
{
  BgSourcePrivate *priv;

  priv = self->priv = SOURCE_PRIVATE (self);

  priv->store = gtk_list_store_new (3, CAIRO_GOBJECT_TYPE_SURFACE, G_TYPE_OBJECT, G_TYPE_STRING);
}

GtkListStore*
bg_source_get_liststore (BgSource *source)
{
  g_return_val_if_fail (BG_IS_SOURCE (source), NULL);

  return source->priv->store;
}

gint
bg_source_get_scale_factor (BgSource *source)
{
  g_return_val_if_fail (BG_IS_SOURCE (source), 1);

  return gtk_widget_get_scale_factor (source->priv->window);
}

gint
bg_source_get_thumbnail_height (BgSource *source)
{
  g_return_val_if_fail (BG_IS_SOURCE (source), THUMBNAIL_HEIGHT);

  return source->priv->thumbnail_height;
}

gint
bg_source_get_thumbnail_width (BgSource *source)
{
  g_return_val_if_fail (BG_IS_SOURCE (source), THUMBNAIL_WIDTH);

  return source->priv->thumbnail_width;
}
