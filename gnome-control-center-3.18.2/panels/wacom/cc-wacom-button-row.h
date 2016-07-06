/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Authors: Joaquim Rocha <jrocha@redhat.com>
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

#ifndef __CC_WACOM_BUTTON_ROW_H__
#define __CC_WACOM_BUTTON_ROW_H__

#include <gtk/gtk.h>

#include "gsd-wacom-device.h"

G_BEGIN_DECLS

#define CC_WACOM_TYPE_BUTTON_ROW         (cc_wacom_button_row_get_type ())
#define CC_WACOM_BUTTON_ROW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_WACOM_TYPE_BUTTON_ROW, CcWacomButtonRow))
#define CC_WACOM_BUTTON_ROW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GTK_TYPE_WACOM_BUTTON_ROW, CcWacomButtonRowClass))
#define CC_WACOM_IS_BUTTON_ROW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_WACOM_TYPE_BUTTON_ROW))
#define CC_WACOM_IS_BUTTON_ROW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CC_WACOM_TYPE_BUTTON_ROW))
#define CC_WACOM_BUTTON_ROW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CC_WACOM_TYPE_BUTTON_ROW, CcWacomButtonRowClass))

typedef struct _CcWacomButtonRow        CcWacomButtonRow;
typedef struct _CcWacomButtonRowClass   CcWacomButtonRowClass;
typedef struct _CcWacomButtonRowPrivate CcWacomButtonRowPrivate;

struct _CcWacomButtonRow {
  GtkListBoxRow parent;

  CcWacomButtonRowPrivate *priv;
};

struct _CcWacomButtonRowClass {
  GtkListBoxRowClass parent_class;
};

static struct {
  GsdWacomActionType  action_type;
  const gchar        *action_name;
} action_table[] = {
  { GSD_WACOM_ACTION_TYPE_NONE, NC_("Wacom action-type", "None") },
  { GSD_WACOM_ACTION_TYPE_CUSTOM, NC_("Wacom action-type", "Send Keystroke") },
  { GSD_WACOM_ACTION_TYPE_SWITCH_MONITOR, NC_("Wacom action-type", "Switch Monitor") },
  { GSD_WACOM_ACTION_TYPE_HELP, NC_("Wacom action-type", "Show On-Screen Help") }
};

GType       cc_wacom_button_row_get_type (void);

GtkWidget * cc_wacom_button_row_new      (GsdWacomTabletButton *button,
                                          GtkDirectionType      dir);

G_END_DECLS

#endif /* __CC_WACOM_PAGE_H__ */
