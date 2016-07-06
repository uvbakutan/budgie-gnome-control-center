/*
 * gsd-wacom-key-shortcut-button.c
 *
 * Copyright © 2013 Red Hat, Inc.
 *
 * Author: Joaquim Rocha <jrocha@redhat.com>
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "gsd-wacom-key-shortcut-button.h"

/**
 * SECTION:gsd-wacom-key-shortcut-button
 * @short_description: A button which captures and displays a keyboard shortcut
 * @title: GsdWacomKeyShortcutButton
 *
 * GsdWacomKeyShortcutButton is a button which, when clicked, captures a keyboard
 * shortcut and displays it.
 * It works in a similar way to #GtkCellRendererAccel but, being a #GtkWidget,
 * can be added to e.g. containers.
 */

#define DEFAULT_CANCEL_KEY GDK_KEY_Escape
#define DEFAULT_CLEAR_KEY  GDK_KEY_BackSpace

#define GSD_WACOM_KEY_SHORTCUT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON, GsdWacomKeyShortcutButtonPrivate))

G_DEFINE_TYPE (GsdWacomKeyShortcutButton, gsd_wacom_key_shortcut_button, GTK_TYPE_BUTTON);

enum {
  KEY_SHORTCUT_EDITED,
  KEY_SHORTCUT_CLEARED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SHORTCUT_KEY_VAL,
  PROP_SHORTCUT_KEY_MODS,
  PROP_SHORTCUT_MODE,
  PROP_SHORTCUT_CANCEL_KEY,
  PROP_SHORTCUT_CLEAR_KEY,
  N_PROPERTIES
};

struct _GsdWacomKeyShortcutButtonPrivate
{
  gboolean editing_mode;

  GdkDevice *grab_keyboard;
  GdkDevice *grab_pointer;

  guint keyval;
  guint keycode;
  GdkModifierType mods;

  /* Temporary shortcut info used for allowing
   * modifier-only shortcuts */
  guint tmp_shortcut_keyval;
  GdkModifierType tmp_shortcut_mods;
  guint32 tmp_shortcut_time;

  GsdWacomKeyShortcutButtonMode mode;

  guint cancel_keyval;
  guint clear_keyval;
};

static guint signals[LAST_SIGNAL] = { 0 };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void gsd_wacom_key_shortcut_button_changed (GsdWacomKeyShortcutButton *self);

static void
gsd_wacom_key_shortcut_button_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  GsdWacomKeyShortcutButton *self = GSD_WACOM_KEY_SHORTCUT_BUTTON (object);
  gboolean changed = FALSE;

  switch (property_id)
    {
    case PROP_SHORTCUT_KEY_VAL:
      self->priv->keyval = g_value_get_uint (value);
      changed = TRUE;
      break;

    case PROP_SHORTCUT_KEY_MODS:
      self->priv->mods = g_value_get_uint (value);
      changed = TRUE;
      break;

    case PROP_SHORTCUT_MODE:
      self->priv->mode = g_value_get_enum (value);
      break;

    case PROP_SHORTCUT_CANCEL_KEY:
      self->priv->cancel_keyval = g_value_get_uint (value);
      break;

    case PROP_SHORTCUT_CLEAR_KEY:
      self->priv->clear_keyval = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (changed)
    gsd_wacom_key_shortcut_button_changed (self);
}

static void
gsd_wacom_key_shortcut_button_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec)
{
  GsdWacomKeyShortcutButton *self = GSD_WACOM_KEY_SHORTCUT_BUTTON (object);

  switch (property_id)
    {
    case PROP_SHORTCUT_KEY_VAL:
      g_value_set_uint (value, self->priv->keyval);
      break;

    case PROP_SHORTCUT_KEY_MODS:
      g_value_set_uint (value, self->priv->mods);
      break;

    case PROP_SHORTCUT_MODE:
      g_value_set_enum (value, self->priv->mode);
      break;

    case PROP_SHORTCUT_CANCEL_KEY:
      g_value_set_uint (value, self->priv->cancel_keyval);
      break;

    case PROP_SHORTCUT_CLEAR_KEY:
      g_value_set_uint (value, self->priv->clear_keyval);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gsd_wacom_key_shortcut_set_editing_mode (GsdWacomKeyShortcutButton *self,
                                         GdkEvent                  *event)
{
  GsdWacomKeyShortcutButtonPrivate *priv;
  GdkDevice *kbd = NULL, *pointer = NULL;
  GdkDeviceManager *device_manager;
  GdkDisplay *display;
  GList *devices, *l;
  GdkWindow *window;
  guint32 time;

  priv = GSD_WACOM_KEY_SHORTCUT_BUTTON (self)->priv;

  priv->editing_mode = TRUE;
  gsd_wacom_key_shortcut_button_changed (self);

  window = gtk_widget_get_window (GTK_WIDGET (self));

  g_return_if_fail (window != NULL);

  display = gtk_widget_get_display (GTK_WIDGET (self));
  device_manager = gdk_display_get_device_manager (display);
  devices = gdk_device_manager_list_devices (device_manager, GDK_DEVICE_TYPE_MASTER);

  for (l = devices; l != NULL; l = l->next)
    {
      GdkDevice *current_device;

      current_device = l->data;
      if (!kbd && gdk_device_get_source (current_device) == GDK_SOURCE_KEYBOARD)
        kbd = current_device;
      else if (!pointer && gdk_device_get_source (current_device) == GDK_SOURCE_MOUSE)
        pointer = current_device;

      if (kbd && pointer)
        break;
    }
  g_list_free (devices);

  time = gdk_event_get_time (event);

  if (gdk_device_grab (kbd, window,
                       GDK_OWNERSHIP_WINDOW, FALSE,
                       GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK,
                       NULL, time) != GDK_GRAB_SUCCESS)
    return;

  if (gdk_device_grab (pointer, window,
                       GDK_OWNERSHIP_WINDOW, FALSE,
                       GDK_BUTTON_PRESS_MASK,
                       NULL, time) != GDK_GRAB_SUCCESS)
    {
      gdk_device_ungrab (kbd, time);
      return;
    }

  gtk_widget_grab_focus (GTK_WIDGET (self));

  priv->grab_keyboard = kbd;
  priv->grab_pointer = pointer;
}

static void
gsd_wacom_key_shortcut_remove_editing_mode (GsdWacomKeyShortcutButton *self)
{
  GsdWacomKeyShortcutButtonPrivate *priv;

  priv = GSD_WACOM_KEY_SHORTCUT_BUTTON (self)->priv;

  priv->editing_mode = FALSE;

  if (priv->grab_keyboard != NULL)
    {
      gdk_device_ungrab (priv->grab_keyboard, GDK_CURRENT_TIME);
      priv->grab_keyboard = NULL;
    }
  if (priv->grab_pointer != NULL)
    {
      gdk_device_ungrab (priv->grab_pointer, GDK_CURRENT_TIME);
      priv->grab_pointer = NULL;
    }

  priv->tmp_shortcut_keyval = 0;
  priv->tmp_shortcut_mods = 0;
  priv->tmp_shortcut_time = 0;
}

static void
gsd_wacom_key_shortcut_button_changed (GsdWacomKeyShortcutButton *self)
{
  gchar *text;

  if (self->priv->editing_mode)
    {
      gtk_button_set_label (GTK_BUTTON (self), _("New shortcut…"));

      gtk_widget_set_state_flags (GTK_WIDGET (self),
                                  GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_PRELIGHT,
                                  FALSE);

      return;
    }

  if (self->priv->keyval == 0 && self->priv->mods == 0)
    {
      gtk_button_set_label (GTK_BUTTON (self), "");
      return;
    }

  text = gtk_accelerator_get_label (self->priv->keyval, self->priv->mods);
  gtk_button_set_label (GTK_BUTTON (self), text);
  g_free (text);
}

static void
gsd_wacom_key_shortcut_button_activate (GtkButton *self)
{
  gsd_wacom_key_shortcut_set_editing_mode (GSD_WACOM_KEY_SHORTCUT_BUTTON (self), NULL);

  GTK_BUTTON_CLASS (gsd_wacom_key_shortcut_button_parent_class)->activate (self);
}

static void
gsd_wacom_key_shortcut_button_init (GsdWacomKeyShortcutButton *self)
{
  self->priv = GSD_WACOM_KEY_SHORTCUT_BUTTON_GET_PRIVATE (self);

  gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);

  self->priv->cancel_keyval = DEFAULT_CANCEL_KEY;
  self->priv->clear_keyval = DEFAULT_CLEAR_KEY;
}

static void
key_shortcut_finished_editing (GsdWacomKeyShortcutButton *self,
                               guint32                    time)
{
  GsdWacomKeyShortcutButtonPrivate *priv = self->priv;

  gdk_device_ungrab (priv->grab_keyboard, time);
  gdk_device_ungrab (priv->grab_pointer, time);

  priv->grab_keyboard = NULL;
  priv->grab_pointer = NULL;

  priv->editing_mode = FALSE;

  gsd_wacom_key_shortcut_remove_editing_mode (self);

  gsd_wacom_key_shortcut_button_changed (self);
}

static gboolean
gsd_wacom_key_shortcut_button_key_release (GtkWidget            *widget,
                                           GdkEventKey          *event)
{
  GsdWacomKeyShortcutButton *self = GSD_WACOM_KEY_SHORTCUT_BUTTON (widget);
  GsdWacomKeyShortcutButtonPrivate *priv = self->priv;

  if (priv->tmp_shortcut_keyval == 0)
    {
      GTK_WIDGET_CLASS (gsd_wacom_key_shortcut_button_parent_class)->key_release_event (widget, event);

      return FALSE;
    }

  priv->keyval = priv->tmp_shortcut_keyval;
  priv->mods = priv->tmp_shortcut_mods;

  key_shortcut_finished_editing (self, priv->tmp_shortcut_time);

  g_signal_emit (self, signals[KEY_SHORTCUT_EDITED], 0);

  return TRUE;
}

static gboolean
gsd_wacom_key_shortcut_button_key_press (GtkWidget   *widget,
                                         GdkEventKey *event)
{
  /* This code is based on the gtk_cell_renderer_accel_start_editing */
  GsdWacomKeyShortcutButton *self;
  GsdWacomKeyShortcutButtonPrivate *priv;
  GdkModifierType mods = 0;
  guint shortcut_keyval;
  guint keyval;
  gboolean edited;
  gboolean cleared;

  self = GSD_WACOM_KEY_SHORTCUT_BUTTON (widget);
  priv = self->priv;

  /* GTK and OTHER modes don't allow modifier keyvals */
  if (event->is_modifier && priv->mode != GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_ALL)
    return TRUE;

  if (!priv->editing_mode)
    {
      GTK_WIDGET_CLASS (gsd_wacom_key_shortcut_button_parent_class)->key_press_event (widget, event);

      return FALSE;
    }

  edited = FALSE;
  cleared = FALSE;

  mods = event->state;

  keyval = event->keyval;
  if (keyval == GDK_KEY_Sys_Req &&
      (mods & GDK_MOD1_MASK) != 0)
    {
      /* HACK: we don't want to use SysRq as a keybinding (but we do
       * want Alt+Print), so we avoid translation from Alt+Print to SysRq
       */
      keyval = GDK_KEY_Print;
    }

  shortcut_keyval = gdk_keyval_to_lower (keyval);

  if (shortcut_keyval == GDK_KEY_ISO_Left_Tab)
    shortcut_keyval = GDK_KEY_Tab;

  mods &= gtk_accelerator_get_default_mod_mask ();

  /* Put shift back if it changed the case of the key, not otherwise.
   */
  if (shortcut_keyval != keyval)
    mods |= GDK_SHIFT_MASK;

  if (mods == 0)
    {
      if (keyval == priv->cancel_keyval)
        {
          /* cancel the edition */
          goto out;
        }
      else if (keyval == priv->clear_keyval)
        {
	  /* clear the current shortcut */
	  cleared = TRUE;
	  goto out;
	}
    }

  priv->tmp_shortcut_keyval = 0;
  priv->tmp_shortcut_mods = 0;
  priv->tmp_shortcut_time = 0;

  if (event->is_modifier)
    {
      /* when the user presses a non-modifier key, it readily assigns the
       * shortcut but since we also support modifiers-only shortcuts, we
       * cannot assign the shortcut right when the user presses a modifier
       * key because the user might assign e.g. Alt, Alt+Ctrl, Alt+Ctrl+Shift, etc.
       * So, we keep track of the pressed shortcut's (keyval, mods and time) if
       * it is a modifier shortcut and assign them when a key-release happens */
      priv->tmp_shortcut_keyval = shortcut_keyval;
      priv->tmp_shortcut_mods = mods;
      priv->tmp_shortcut_time = event->time;

      return TRUE;
    }

  edited = TRUE;

 out:

  if (edited)
    {
      priv->keyval = shortcut_keyval;
      priv->mods = mods;
    }

  if (cleared)
    {
      priv->keyval = 0;
      priv->mods = 0;
    }

  key_shortcut_finished_editing (GSD_WACOM_KEY_SHORTCUT_BUTTON (widget), event->time);

  if (edited)
    g_signal_emit (self, signals[KEY_SHORTCUT_EDITED], 0);
  else if (cleared)
    g_signal_emit (self, signals[KEY_SHORTCUT_CLEARED], 0);

  return TRUE;
}

static gboolean
gsd_wacom_key_shortcut_button_button_press (GtkWidget      *widget,
                                            GdkEventButton *event)
{
  GsdWacomKeyShortcutButton *self;

  self = GSD_WACOM_KEY_SHORTCUT_BUTTON (widget);

  if (self->priv->editing_mode)
    return TRUE;

  gsd_wacom_key_shortcut_set_editing_mode (self, NULL);

  GTK_WIDGET_CLASS (gsd_wacom_key_shortcut_button_parent_class)->button_press_event (widget,
                                                                              event);

  return TRUE;
}

static void
gsd_wacom_key_shortcut_button_unrealize (GtkWidget *widget)
{
  GsdWacomKeyShortcutButton *self;

  self = GSD_WACOM_KEY_SHORTCUT_BUTTON (widget);

  gsd_wacom_key_shortcut_remove_editing_mode (self);

  GTK_WIDGET_CLASS (gsd_wacom_key_shortcut_button_parent_class)->unrealize (widget);
}

static void
gsd_wacom_key_shortcut_button_class_init (GsdWacomKeyShortcutButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  gobject_class->set_property = gsd_wacom_key_shortcut_button_set_property;
  gobject_class->get_property = gsd_wacom_key_shortcut_button_get_property;

  obj_properties[PROP_SHORTCUT_KEY_VAL] =
    g_param_spec_uint ("key-value",
                       "The key value",
                       "The key value of the shortcut currently set",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_SHORTCUT_KEY_MODS] =
    g_param_spec_uint ("key-mods",
                       "The key modifiers",
                       "The key modifiers of the shortcut currently set",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_SHORTCUT_CANCEL_KEY] =
    g_param_spec_uint ("cancel-key",
                       "The cancel key",
                       "The key which cancels the edition of the shortcut",
                       0,
                       G_MAXUINT,
                       DEFAULT_CANCEL_KEY,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  obj_properties[PROP_SHORTCUT_CLEAR_KEY] =
    g_param_spec_uint ("clear-key",
                       "The clear key",
                       "The key which clears the currently set shortcut",
                       0,
                       G_MAXUINT,
                       DEFAULT_CLEAR_KEY,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  /**
   * GsdWacomKeyShortcutButton:mode:
   *
   * Determines which type of keys are allowed in the captured shortcuts.
   * %GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_ALL is the same as
   * %GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_OTHER but allows shortcuts composed of
   * only modifier keys.
   */
  obj_properties[PROP_SHORTCUT_MODE] =
    g_param_spec_enum ("mode",
                       "The shortcut mode",
                       "The mode with which the shortcuts are captured",
                       GSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON_MODE,
                       GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_OTHER,
                       G_PARAM_READWRITE |
                       G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  widget_class->key_press_event = gsd_wacom_key_shortcut_button_key_press;
  widget_class->button_press_event = gsd_wacom_key_shortcut_button_button_press;
  widget_class->key_release_event = gsd_wacom_key_shortcut_button_key_release;
  widget_class->unrealize = gsd_wacom_key_shortcut_button_unrealize;

  button_class->activate = gsd_wacom_key_shortcut_button_activate;

  /**
   * GsdWacomKeyShortcutButton::key-shortcut-edited:
   * @keyshortcutbutton: the #GsdWacomKeyShortcutButton
   *
   * Emitted when the key shortcut of the @keyshortcutbutton is edited.
   *
   * The new shortcut can be retrieved by using the #GsdWacomKeyShortcutButton:key-value
   * and #GsdWacomKeyShortcutButton:key-mods properties.
   */
  signals[KEY_SHORTCUT_EDITED] = g_signal_new ("key-shortcut-edited",
                                               GSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON,
                                               G_SIGNAL_RUN_LAST,
                                               G_STRUCT_OFFSET (GsdWacomKeyShortcutButtonClass,
                                                                key_shortcut_edited),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__VOID,
                                               G_TYPE_NONE, 0);

  /**
   * GsdWacomKeyShortcutButton::key-shortcut-cleared:
   * @keyshortcutbutton: the #GsdWacomKeyShortcutButton
   *
   * Emitted when the key shortcut of the @keyshortcutbutton is cleared.
   */
  signals[KEY_SHORTCUT_CLEARED] = g_signal_new ("key-shortcut-cleared",
                                                GSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON,
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (GsdWacomKeyShortcutButtonClass,
                                                                 key_shortcut_cleared),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (GsdWacomKeyShortcutButtonPrivate));
}

/**
 * gsd_wacom_key_shortcut_button_new:
 *
 * Creates a new #GsdWacomKeyShortcutButton.
 *
 * Returns: a new #GsdWacomKeyShortcutButton object.
 *
 * Since: 3.10
 */
GtkWidget *
gsd_wacom_key_shortcut_button_new (void)
{
  return g_object_new (GSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON, NULL);
}

GType
gsd_wacom_key_shortcut_button_mode_type (void)
{
  static GType enum_type_id = 0;
  if (G_UNLIKELY (!enum_type_id))
    {
      static const GEnumValue values[] =
        {
          { GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_OTHER, "OTHER", "other" },
          { GSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_ALL, "ALL", "all" },
          { 0, NULL, NULL }
        };
      enum_type_id = g_enum_register_static ("GsdWacomKeyShortcutButtonMode", values);
    }
  return enum_type_id;
}
