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

#include <config.h>

#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gdesktop-enums.h>

#include "cc-background-panel.h"

#include "cc-background-chooser-dialog.h"
#include "cc-background-item.h"
#include "cc-background-resources.h"
#include "cc-background-xml.h"

#include "bg-pictures-source.h"

#define WP_PATH_ID "org.gnome.desktop.background"
#define WP_LOCK_PATH_ID "org.gnome.desktop.screensaver"
#define WP_URI_KEY "picture-uri"
#define WP_OPTIONS_KEY "picture-options"
#define WP_SHADING_KEY "color-shading-type"
#define WP_PCOLOR_KEY "primary-color"
#define WP_SCOLOR_KEY "secondary-color"

CC_PANEL_REGISTER (CcBackgroundPanel, cc_background_panel)

#define BACKGROUND_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_BACKGROUND_PANEL, CcBackgroundPanelPrivate))

struct _CcBackgroundPanelPrivate
{
  GtkBuilder *builder;
  GDBusConnection *connection;

  GSettings *settings;
  GSettings *lock_settings;

  GnomeDesktopThumbnailFactory *thumb_factory;

  CcBackgroundItem *current_background;
  CcBackgroundItem *current_lock_background;

  GCancellable *copy_cancellable;
  GCancellable *capture_cancellable;

  GtkWidget *spinner;
  GtkWidget *chooser;

  GdkPixbuf *display_screenshot;
  char *screenshot_path;
};

#define WID(y) (GtkWidget *) gtk_builder_get_object (priv->builder, y)
#define CURRENT_BG (settings == priv->settings ? priv->current_background : priv->current_lock_background)
#define SAVE_PATH (settings == priv->settings ? "last-edited.xml" : "last-edited-lock.xml")

static const char *
cc_background_panel_get_help_uri (CcPanel *panel)
{
  return "help:gnome-help/look-background";
}

static void
cc_background_panel_dispose (GObject *object)
{
  CcBackgroundPanelPrivate *priv = CC_BACKGROUND_PANEL (object)->priv;

  g_clear_object (&priv->builder);

  /* destroying the builder object will also destroy the spinner */
  priv->spinner = NULL;

  g_clear_object (&priv->settings);
  g_clear_object (&priv->lock_settings);

  if (priv->copy_cancellable)
    {
      /* cancel any copy operation */
      g_cancellable_cancel (priv->copy_cancellable);

      g_clear_object (&priv->copy_cancellable);
    }

  if (priv->capture_cancellable)
    {
      /* cancel screenshot operations */
      g_cancellable_cancel (priv->capture_cancellable);

      g_clear_object (&priv->capture_cancellable);
    }

  if (priv->chooser)
    {
      gtk_widget_destroy (priv->chooser);
      priv->chooser = NULL;
    }

  g_clear_object (&priv->thumb_factory);
  g_clear_object (&priv->display_screenshot);

  g_clear_pointer (&priv->screenshot_path, g_free);

  G_OBJECT_CLASS (cc_background_panel_parent_class)->dispose (object);
}

static void
cc_background_panel_finalize (GObject *object)
{
  CcBackgroundPanelPrivate *priv = CC_BACKGROUND_PANEL (object)->priv;

  g_clear_object (&priv->current_background);
  g_clear_object (&priv->current_lock_background);

  G_OBJECT_CLASS (cc_background_panel_parent_class)->finalize (object);
}

static void
cc_background_panel_class_init (CcBackgroundPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcBackgroundPanelPrivate));

  panel_class->get_help_uri = cc_background_panel_get_help_uri;

  object_class->dispose = cc_background_panel_dispose;
  object_class->finalize = cc_background_panel_finalize;
}

static void
update_preview (CcBackgroundPanelPrivate *priv,
                GSettings                *settings,
                CcBackgroundItem         *item)
{
  gboolean changes_with_time;
  CcBackgroundItem *current_background;

  current_background = CURRENT_BG;

  if (item && current_background)
    {
      g_object_unref (current_background);
      current_background = cc_background_item_copy (item);
      if (settings == priv->settings)
        priv->current_background = current_background;
      else
        priv->current_lock_background = current_background;
      cc_background_item_load (current_background, NULL);
    }

  changes_with_time = FALSE;

  if (current_background)
    {
      changes_with_time = cc_background_item_changes_with_time (current_background);
    }

  if (settings == priv->settings)
    {
      gtk_widget_set_visible (WID ("slide_image"), changes_with_time);
      gtk_widget_set_visible (WID ("slide-label"), changes_with_time);

      gtk_widget_queue_draw (WID ("background-desktop-drawingarea"));
    }
  else
    {
      gtk_widget_set_visible (WID ("slide_image1"), changes_with_time);
      gtk_widget_set_visible (WID ("slide-label1"), changes_with_time);

      gtk_widget_queue_draw (WID ("background-lock-drawingarea"));
    }
}

static char *
get_save_path (const char *filename)
{
  return g_build_filename (g_get_user_config_dir (),
                           "gnome-control-center",
                           "backgrounds",
                           filename,
                           NULL);
}

static void
update_display_preview (CcBackgroundPanel *panel,
                        GtkWidget         *widget,
                        CcBackgroundItem  *current_background)
{
  CcBackgroundPanelPrivate *priv = panel->priv;
  GtkAllocation allocation;
  const gint preview_width = 309;
  const gint preview_height = 168;
  gint scale_factor;
  GdkPixbuf *pixbuf;
  cairo_t *cr;

  gtk_widget_get_allocation (widget, &allocation);

  if (!current_background)
    return;

  scale_factor = gtk_widget_get_scale_factor (widget);
  pixbuf = cc_background_item_get_frame_thumbnail (current_background,
                                                   priv->thumb_factory,
                                                   preview_width,
                                                   preview_height,
                                                   scale_factor,
                                                   -2, TRUE);

  cr = gdk_cairo_create (gtk_widget_get_window (widget));
  gdk_cairo_set_source_pixbuf (cr,
                               pixbuf,
                               0, 0);
  cairo_paint (cr);
  g_object_unref (pixbuf);

  pixbuf = NULL;
  if (current_background == priv->current_background &&
      panel->priv->display_screenshot != NULL)
    {
      pixbuf = gdk_pixbuf_scale_simple (panel->priv->display_screenshot,
                                        preview_width,
                                        preview_height,
                                        GDK_INTERP_BILINEAR);
    }

  if (pixbuf)
    {
      gdk_cairo_set_source_pixbuf (cr,
                                   pixbuf,
                                   0, 0);
      cairo_paint (cr);
      g_object_unref (pixbuf);
    }

  cairo_destroy (cr);
}

typedef struct {
  CcBackgroundPanel *panel;
  GdkRectangle capture_rect;
  GdkRectangle monitor_rect;
  GdkRectangle workarea_rect;
  gboolean whole_monitor;
} ScreenshotData;

static void
on_screenshot_finished (GObject *source,
                        GAsyncResult *res,
                        gpointer user_data)
{
  ScreenshotData *data = user_data;
  CcBackgroundPanel *panel = data->panel;
  CcBackgroundPanelPrivate *priv;
  GError *error;
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;
  cairo_t *cr;
  GVariant *result;

  error = NULL;
  result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source),
                                          res,
                                          &error);

  if (result == NULL) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_error_free (error);
      g_free (data);
      return;
    }
    g_debug ("Unable to get screenshot: %s",
             error->message);
    g_error_free (error);
    /* fallback? */
    priv = panel->priv;
    goto out;
  }
  g_variant_unref (result);

  priv = panel->priv;

  pixbuf = gdk_pixbuf_new_from_file (panel->priv->screenshot_path, &error);
  if (pixbuf == NULL)
    {
      g_debug ("Unable to use GNOME Shell's builtin screenshot interface: %s",
               error->message);
      g_error_free (error);
      goto out;
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        data->monitor_rect.width, data->monitor_rect.height);
  cr = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (cr, pixbuf,
                               data->capture_rect.x - data->monitor_rect.x,
                               data->capture_rect.y - data->monitor_rect.y);
  cairo_paint (cr);
  g_object_unref (pixbuf);

  if (data->whole_monitor) {
    /* clear the workarea */
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle (cr, data->workarea_rect.x - data->monitor_rect.x,
                     data->workarea_rect.y - data->monitor_rect.y,
                     data->workarea_rect.width,
                     data->workarea_rect.height);
    cairo_fill (cr);
    cairo_restore (cr);
  }

  g_clear_object (&panel->priv->display_screenshot);
  panel->priv->display_screenshot = gdk_pixbuf_get_from_surface (surface,
                                                                 0, 0,
                                                                 data->monitor_rect.width,
                                                                 data->monitor_rect.height);

  /* remove the temporary file created by the shell */
  g_unlink (panel->priv->screenshot_path);
  g_clear_pointer (&priv->screenshot_path, g_free);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

 out:
  update_display_preview (panel, WID ("background-desktop-drawingarea"), priv->current_background);
  g_free (data);
}

static gboolean
calculate_contiguous_workarea (ScreenshotData *data)
{
  /* Optimise for the shell panel being the only non-workarea
   * object at the top of the screen */
  if (data->workarea_rect.x != data->monitor_rect.x)
    return FALSE;
  if ((data->workarea_rect.y + data->workarea_rect.height) != (data->monitor_rect.y + data->monitor_rect.height))
    return FALSE;

  data->capture_rect.x = data->monitor_rect.x;
  data->capture_rect.width = data->monitor_rect.width;
  data->capture_rect.y = data->monitor_rect.y;
  data->capture_rect.height = data->monitor_rect.height - data->workarea_rect.height;

  return TRUE;
}

static void
get_screenshot_async (CcBackgroundPanel *panel)
{
  CcBackgroundPanelPrivate *priv = panel->priv;
  gchar *path, *tmpname;
  const gchar *method_name;
  GVariant *method_params;
  GtkWidget *widget;
  ScreenshotData *data;
  int primary;

  data = g_new0 (ScreenshotData, 1);
  data->panel = panel;

  widget = WID ("background-desktop-drawingarea");
  primary = gdk_screen_get_primary_monitor (gtk_widget_get_screen (widget));
  gdk_screen_get_monitor_geometry (gtk_widget_get_screen (widget), primary, &data->monitor_rect);
  gdk_screen_get_monitor_workarea (gtk_widget_get_screen (widget), primary, &data->workarea_rect);
  if (calculate_contiguous_workarea (data)) {
    g_debug ("Capturing only a portion of the screen");
  } else {
    g_debug ("Capturing the whole monitor");
    data->whole_monitor = TRUE;
    data->capture_rect = data->monitor_rect;
  }

  g_debug ("Trying to capture rectangle %dx%d (at %d,%d)",
           data->capture_rect.width, data->capture_rect.height, data->capture_rect.x, data->capture_rect.y);

  path = g_build_filename (g_get_user_cache_dir (), "gnome-control-center", NULL);
  g_mkdir_with_parents (path, USER_DIR_MODE);

  tmpname = g_strdup_printf ("scr-%d.png", g_random_int ());
  g_free (panel->priv->screenshot_path);
  panel->priv->screenshot_path = g_build_filename (path, tmpname, NULL);
  g_free (path);
  g_free (tmpname);

  method_name = "ScreenshotArea";
  method_params = g_variant_new ("(iiiibs)",
                                 data->capture_rect.x, data->capture_rect.y,
                                 data->capture_rect.width, data->capture_rect.height,
                                 FALSE, /* flash */
                                 panel->priv->screenshot_path);

  g_dbus_connection_call (panel->priv->connection,
                          "org.gnome.Shell.Screenshot",
                          "/org/gnome/Shell/Screenshot",
                          "org.gnome.Shell.Screenshot",
                          method_name,
                          method_params,
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          priv->capture_cancellable,
                          on_screenshot_finished,
                          data);
}

static gboolean
on_preview_draw (GtkWidget         *widget,
                 cairo_t           *cr,
                 CcBackgroundPanel *panel)
{
  CcBackgroundPanelPrivate *priv = panel->priv;
  /* we have another shot in flight or an existing cache */
  if (panel->priv->display_screenshot == NULL
      && panel->priv->screenshot_path == NULL)
    {
      get_screenshot_async (panel);
    }
  else
    update_display_preview (panel, widget, priv->current_background);

  return TRUE;
}

static gboolean
on_lock_preview_draw (GtkWidget         *widget,
                      cairo_t           *cr,
                      CcBackgroundPanel *panel)
{
  CcBackgroundPanelPrivate *priv = panel->priv;
  update_display_preview (panel, widget, priv->current_lock_background);
  return TRUE;
}

static void
reload_current_bg (CcBackgroundPanel *self,
                   GSettings         *settings)
{
  CcBackgroundPanelPrivate *priv;
  CcBackgroundItem *saved, *configured;
  gchar *uri, *pcolor, *scolor;

  priv = self->priv;

  /* Load the saved configuration */
  uri = get_save_path (SAVE_PATH);
  saved = cc_background_xml_get_item (uri);
  g_free (uri);

  /* initalise the current background information from settings */
  uri = g_settings_get_string (settings, WP_URI_KEY);
  if (uri && *uri == '\0')
    {
      g_clear_pointer (&uri, g_free);
    }
  else
    {
      GFile *file;

      file = g_file_new_for_commandline_arg (uri);
      g_object_unref (file);
    }
  configured = cc_background_item_new (uri);
  g_free (uri);

  pcolor = g_settings_get_string (settings, WP_PCOLOR_KEY);
  scolor = g_settings_get_string (settings, WP_SCOLOR_KEY);
  g_object_set (G_OBJECT (configured),
                "name", _("Current background"),
                "placement", g_settings_get_enum (settings, WP_OPTIONS_KEY),
                "shading", g_settings_get_enum (settings, WP_SHADING_KEY),
                "primary-color", pcolor,
                "secondary-color", scolor,
                NULL);
  g_free (pcolor);
  g_free (scolor);

  if (saved != NULL && cc_background_item_compare (saved, configured))
    {
      CcBackgroundItemFlags flags;
      flags = cc_background_item_get_flags (saved);
      /* Special case for colours */
      if (cc_background_item_get_placement (saved) == G_DESKTOP_BACKGROUND_STYLE_NONE)
        flags &=~ (CC_BACKGROUND_ITEM_HAS_PCOLOR | CC_BACKGROUND_ITEM_HAS_SCOLOR);
      g_object_set (G_OBJECT (configured),
		    "name", cc_background_item_get_name (saved),
		    "flags", flags,
		    "source-url", cc_background_item_get_source_url (saved),
		    "source-xml", cc_background_item_get_source_xml (saved),
		    NULL);
    }
  if (saved != NULL)
    g_object_unref (saved);

  if (settings == priv->settings)
    {
      g_clear_object (&priv->current_background);
      priv->current_background = configured;
    }
  else
    {
      g_clear_object (&priv->current_lock_background);
      priv->current_lock_background = configured;
    }
  cc_background_item_load (configured, NULL);
}

static gboolean
create_save_dir (void)
{
  char *path;

  path = g_build_filename (g_get_user_config_dir (),
			   "gnome-control-center",
			   "backgrounds",
			   NULL);
  if (g_mkdir_with_parents (path, USER_DIR_MODE) < 0)
    {
      g_warning ("Failed to create directory '%s'", path);
      g_free (path);
      return FALSE;
    }
  g_free (path);
  return TRUE;
}

static void
copy_finished_cb (GObject      *source_object,
                  GAsyncResult *result,
                  gpointer      pointer)
{
  GError *err = NULL;
  CcBackgroundPanel *panel = (CcBackgroundPanel *) pointer;
  CcBackgroundPanelPrivate *priv = panel->priv;
  CcBackgroundItem *item;
  CcBackgroundItem *current_background;
  GSettings *settings;

  if (!g_file_copy_finish (G_FILE (source_object), result, &err))
    {
      if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
        g_error_free (err);
        return;
      }
      g_warning ("Failed to copy image to cache location: %s", err->message);
      g_error_free (err);
    }
  item = g_object_get_data (source_object, "item");
  settings = g_object_get_data (source_object, "settings");
  current_background = CURRENT_BG;

  g_settings_apply (settings);

  /* the panel may have been destroyed before the callback is run, so be sure
   * to check the widgets are not NULL */

  if (priv->spinner)
    {
      gtk_widget_destroy (GTK_WIDGET (priv->spinner));
      priv->spinner = NULL;
    }

  if (current_background)
    cc_background_item_load (current_background, NULL);

  if (priv->builder)
    {
      char *filename;

      update_preview (priv, settings, item);
      current_background = CURRENT_BG;

      /* Save the source XML if there is one */
      filename = get_save_path (SAVE_PATH);
      if (create_save_dir ())
        cc_background_xml_save (current_background, filename);
    }

  /* remove the reference taken when the copy was set up */
  g_object_unref (panel);
}

static void
set_background (CcBackgroundPanel *panel,
                GSettings         *settings,
                CcBackgroundItem  *item)
{
  CcBackgroundPanelPrivate *priv = panel->priv;
  GDesktopBackgroundStyle style;
  gboolean save_settings = TRUE;
  const char *uri;
  CcBackgroundItemFlags flags;
  char *filename;

  if (item == NULL)
    return;

  uri = cc_background_item_get_uri (item);
  flags = cc_background_item_get_flags (item);

  if ((flags & CC_BACKGROUND_ITEM_HAS_URI) && uri == NULL)
    {
      g_settings_set_enum (settings, WP_OPTIONS_KEY, G_DESKTOP_BACKGROUND_STYLE_NONE);
      g_settings_set_string (settings, WP_URI_KEY, "");
    }
  else if (cc_background_item_get_source_url (item) != NULL &&
           cc_background_item_get_needs_download (item))
    {
      GFile *source, *dest;
      char *cache_path, *basename, *dest_path, *display_name, *dest_uri;
      GdkPixbuf *pixbuf;

      cache_path = bg_pictures_source_get_cache_path ();
      if (g_mkdir_with_parents (cache_path, USER_DIR_MODE) < 0)
        {
          g_warning ("Failed to create directory '%s'", cache_path);
          g_free (cache_path);
          return;
        }
      g_free (cache_path);

      dest_path = bg_pictures_source_get_unique_path (cc_background_item_get_source_url (item));
      dest = g_file_new_for_path (dest_path);
      g_free (dest_path);
      source = g_file_new_for_uri (cc_background_item_get_source_url (item));
      basename = g_file_get_basename (source);
      display_name = g_filename_display_name (basename);
      dest_path = g_file_get_path (dest);
      g_free (basename);

      /* create a blank image to use until the source image is ready */
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
      gdk_pixbuf_fill (pixbuf, 0x00000000);
      gdk_pixbuf_save (pixbuf, dest_path, "png", NULL, NULL);
      g_object_unref (pixbuf);
      g_free (dest_path);

      if (priv->copy_cancellable)
        {
          g_cancellable_cancel (priv->copy_cancellable);
          g_cancellable_reset (priv->copy_cancellable);
        }

      if (priv->spinner)
        {
          gtk_widget_destroy (GTK_WIDGET (priv->spinner));
          priv->spinner = NULL;
        }

      /* create a spinner while the file downloads */
      priv->spinner = gtk_spinner_new ();
      gtk_spinner_start (GTK_SPINNER (priv->spinner));
      gtk_box_pack_start (GTK_BOX (WID ("bottom-hbox")), priv->spinner, FALSE,
                          FALSE, 6);
      gtk_widget_show (priv->spinner);

      /* reference the panel in case it is removed before the copy is
       * finished */
      g_object_ref (panel);
      g_object_set_data_full (G_OBJECT (source), "item", g_object_ref (item), g_object_unref);
      g_object_set_data (G_OBJECT (source), "settings", settings);
      g_file_copy_async (source, dest, G_FILE_COPY_OVERWRITE,
                         G_PRIORITY_DEFAULT, priv->copy_cancellable,
                         NULL, NULL,
                         copy_finished_cb, panel);
      g_object_unref (source);
      dest_uri = g_file_get_uri (dest);
      g_object_unref (dest);

      g_settings_set_string (settings, WP_URI_KEY, dest_uri);
      g_object_set (G_OBJECT (item),
                    "uri", dest_uri,
                    "needs-download", FALSE,
                    "name", display_name,
                    NULL);
      g_free (display_name);
      g_free (dest_uri);

      /* delay the updated drawing of the preview until the copy finishes */
      save_settings = FALSE;
    }
  else
    {
      g_settings_set_string (settings, WP_URI_KEY, uri);
    }

  /* Also set the placement if we have a URI and the previous value was none */
  if (flags & CC_BACKGROUND_ITEM_HAS_PLACEMENT)
    {
      g_settings_set_enum (settings, WP_OPTIONS_KEY, cc_background_item_get_placement (item));
    }
  else if (uri != NULL)
    {
      style = g_settings_get_enum (settings, WP_OPTIONS_KEY);
      if (style == G_DESKTOP_BACKGROUND_STYLE_NONE)
        g_settings_set_enum (settings, WP_OPTIONS_KEY, cc_background_item_get_placement (item));
    }

  if (flags & CC_BACKGROUND_ITEM_HAS_SHADING)
    g_settings_set_enum (settings, WP_SHADING_KEY, cc_background_item_get_shading (item));

  g_settings_set_string (settings, WP_PCOLOR_KEY, cc_background_item_get_pcolor (item));
  g_settings_set_string (settings, WP_SCOLOR_KEY, cc_background_item_get_scolor (item));

  /* update the preview information */
  if (save_settings != FALSE)
    {
      /* Apply all changes */
      g_settings_apply (settings);

      /* Save the source XML if there is one */
      filename = get_save_path (SAVE_PATH);
      if (create_save_dir ())
        cc_background_xml_save (CURRENT_BG, filename);
    }
}

static void
on_chooser_dialog_response (GtkDialog         *dialog,
                            int                response_id,
                            CcBackgroundPanel *self)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      CcBackgroundItem *item;

      item = cc_background_chooser_dialog_get_item (CC_BACKGROUND_CHOOSER_DIALOG (dialog));
      if (item != NULL)
        {
          set_background (self, g_object_get_data (G_OBJECT (dialog), "settings"), item);
          g_object_unref (item);
        }
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
launch_chooser (CcBackgroundPanel *self,
                GSettings         *settings)
{
  CcBackgroundPanelPrivate *priv = self->priv;
  GtkWidget *dialog;

  dialog = cc_background_chooser_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (WID ("background-panel"))));
  g_object_set_data (G_OBJECT (dialog), "settings", settings);
  gtk_widget_show (dialog);
  g_signal_connect (dialog, "response", G_CALLBACK (on_chooser_dialog_response), self);
  priv->chooser = dialog;
  g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer *) &priv->chooser);
}

static void
on_background_button_clicked (GtkButton         *button,
                              CcBackgroundPanel *self)
{
  launch_chooser (self, self->priv->settings);
}

static void
on_lock_button_clicked (GtkButton         *button,
                        CcBackgroundPanel *self)
{
  launch_chooser (self, self->priv->lock_settings);
}

static void
on_settings_changed (GSettings         *settings,
                     gchar             *key,
                     CcBackgroundPanel *self)
{
  reload_current_bg (self, settings);
  update_preview (self->priv, settings, NULL);
}

static void
cc_background_panel_init (CcBackgroundPanel *self)
{
  CcBackgroundPanelPrivate *priv;
  gchar *objects[] = {"background-panel", NULL };
  GError *err = NULL;
  GtkWidget *widget;

  priv = self->priv = BACKGROUND_PANEL_PRIVATE (self);

  priv->connection = g_application_get_dbus_connection (g_application_get_default ());
  g_resources_register (cc_background_get_resource ());

  priv->builder = gtk_builder_new ();
  gtk_builder_add_objects_from_resource (priv->builder,
                                         "/org/gnome/control-center/background/background.ui",
                                         objects, &err);

  if (err)
    {
      g_warning ("Could not load ui: %s", err->message);
      g_error_free (err);
      return;
    }

  priv->settings = g_settings_new (WP_PATH_ID);
  g_settings_delay (priv->settings);

  priv->lock_settings = g_settings_new (WP_LOCK_PATH_ID);
  g_settings_delay (priv->lock_settings);

  /* add the top level widget */
  widget = WID ("background-panel");

  gtk_container_add (GTK_CONTAINER (self), widget);
  gtk_widget_show_all (GTK_WIDGET (self));

  /* setup preview area */
  widget = WID ("background-desktop-drawingarea");
  g_signal_connect (widget, "draw", G_CALLBACK (on_preview_draw), self);
  widget = WID ("background-lock-drawingarea");
  g_signal_connect (widget, "draw", G_CALLBACK (on_lock_preview_draw), self);

  priv->copy_cancellable = g_cancellable_new ();
  priv->capture_cancellable = g_cancellable_new ();

  priv->thumb_factory = gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE);

  /* Load the backgrounds */
  reload_current_bg (self, priv->settings);
  update_preview (priv, priv->settings, NULL);
  reload_current_bg (self, priv->lock_settings);
  update_preview (priv, priv->lock_settings, NULL);

  /* Background settings */
  g_signal_connect (priv->settings, "changed", G_CALLBACK (on_settings_changed), self);
  g_signal_connect (priv->lock_settings, "changed", G_CALLBACK (on_settings_changed), self);

  /* Background buttons */
  widget = WID ("background-set-button");
  g_signal_connect (widget, "clicked", G_CALLBACK (on_background_button_clicked), self);
  widget = WID ("background-lock-set-button");
  g_signal_connect (widget, "clicked", G_CALLBACK (on_lock_button_clicked), self);
}
