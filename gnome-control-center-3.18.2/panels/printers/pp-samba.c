/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2012 - 2013 Red Hat, Inc,
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
 * Author: Marek Kasik <mkasik@redhat.com>
 */

#include "pp-samba.h"

#include "config.h"

#include <glib/gi18n.h>
#include <libsmbclient.h>
#include <errno.h>

#include "pp-authentication-dialog.h"

#define POLL_DELAY 100000

struct _PpSambaPrivate
{
  GtkWindow *parent;

  gchar *hostname;
};

G_DEFINE_TYPE (PpSamba, pp_samba, G_TYPE_OBJECT);

enum
{
  PROP_0 = 0,
  PROP_HOSTNAME,
  PROP_PARENT,
};

static void
pp_samba_finalize (GObject *object)
{
  PpSambaPrivate *priv;

  priv = PP_SAMBA (object)->priv;

  g_free (priv->hostname);
  g_object_unref (priv->parent);

  G_OBJECT_CLASS (pp_samba_parent_class)->finalize (object);
}

static void
pp_samba_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *param_spec)
{
  PpSamba *self = PP_SAMBA (object);

  switch (prop_id)
    {
      case PROP_HOSTNAME:
        g_value_set_string (value, self->priv->hostname);
        break;
      case PROP_PARENT:
        g_value_set_pointer (value, self->priv->parent);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                           prop_id,
                                           param_spec);
      break;
    }
}

static void
pp_samba_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *param_spec)
{
  PpSamba *self = PP_SAMBA (object);

  switch (prop_id)
    {
      case PROP_HOSTNAME:
        g_free (self->priv->hostname);
        self->priv->hostname = g_value_dup_string (value);
        break;
      case PROP_PARENT:
        if (self->priv->parent)
          g_object_unref (self->priv->parent);
        self->priv->parent = g_object_ref (G_OBJECT (g_value_get_pointer (value)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                           prop_id,
                                           param_spec);
        break;
    }
}

static void
pp_samba_class_init (PpSambaClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PpSambaPrivate));

  gobject_class->set_property = pp_samba_set_property;
  gobject_class->get_property = pp_samba_get_property;
  gobject_class->finalize = pp_samba_finalize;

  /*
   * Used for searching on specific host.
   */
  g_object_class_install_property (gobject_class, PROP_HOSTNAME,
    g_param_spec_string ("hostname",
                         "Hostname",
                         "The hostname to search",
                         NULL,
                         G_PARAM_READWRITE));

  /*
   * Used for authentication dialog.
   */
  g_object_class_install_property (gobject_class, PROP_PARENT,
    g_param_spec_pointer ("parent",
                          "Parent",
                          "Parent window",
                          G_PARAM_READWRITE));
}

static void
pp_samba_init (PpSamba *samba)
{
  samba->priv = G_TYPE_INSTANCE_GET_PRIVATE (samba,
                                             PP_TYPE_SAMBA,
                                             PpSambaPrivate);
}

PpSamba *
pp_samba_new (GtkWindow   *parent,
              const gchar *hostname)
{
  return g_object_new (PP_TYPE_SAMBA,
                       "parent", parent,
                       "hostname", hostname,
                       NULL);
}

typedef struct
{
  gchar    *server;
  gchar    *share;
  gchar    *workgroup;
  gchar    *username;
  gchar    *password;
} SMBAuthInfo;

static void
smb_auth_info_free (SMBAuthInfo *auth_info)
{
  if (auth_info)
    {
      g_free (auth_info->server);
      g_free (auth_info->share);
      g_free (auth_info->workgroup);
      g_free (auth_info->username);
      g_free (auth_info->password);
      g_free (auth_info);
    }
}

typedef struct
{
  PpDevicesList *devices;
  GMainContext  *context;
  gboolean       waiting;
  gboolean       auth_if_needed;
  GtkWindow     *parent;
  SMBAuthInfo   *auth_info;
  gboolean       hostname_set;
  gboolean       cancelled;
} SMBData;

static void
smb_data_free (SMBData *data)
{
  if (data)
    {
      pp_devices_list_free (data->devices);
      smb_auth_info_free (data->auth_info);
      g_object_unref (data->parent);

      g_free (data);
    }
}

static void
auth_cb (PpAuthenticationDialog *auth_dialog,
         gint                    response_id,
         const gchar            *username,
         const gchar            *password,
         gpointer                user_data)
{
  SMBData *data = (SMBData *) user_data;

  if (username && username[0] != '\0')
    {
      g_free (data->auth_info->username);
      data->auth_info->username = g_strdup (username);
    }

  if (password && password[0] != '\0')
    {
      g_free (data->auth_info->password);
      data->auth_info->password = g_strdup (password);
    }

  g_object_unref (auth_dialog);

  if (response_id == GTK_RESPONSE_CANCEL ||
      response_id == GTK_RESPONSE_DELETE_EVENT)
    data->cancelled = TRUE;

  data->waiting = FALSE;
}

static gboolean
get_auth_info (gpointer user_data)
{
  PpAuthenticationDialog *auth_dialog;
  SMBData                *data = (SMBData *) user_data;
  gchar                  *text;

  /* Translators: Samba server needs authentication of the user to show list of its printers. */
  text = g_strdup_printf (_("Enter your username and password to view printers available on %s."),
                          data->auth_info->server);
  auth_dialog = pp_authentication_dialog_new (data->parent,
                                              text,
                                              data->auth_info->username);
  g_signal_connect (auth_dialog, "response", G_CALLBACK (auth_cb), user_data);

  g_free (text);

  return FALSE;
}

static void
auth_fn (SMBCCTX    *smb_context,
         const char *server,
         const char *share,
         char       *workgroup,
         int         wgmaxlen,
         char       *username,
         int         unmaxlen,
         char       *password,
         int         pwmaxlen)
{
  GSource *source;
  SMBData *data;

  data = (SMBData *) smbc_getOptionUserData (smb_context);

  if (!data->cancelled)
    {
      data->auth_info = g_new (SMBAuthInfo, 1);
      data->auth_info->server = g_strdup (server);
      data->auth_info->share = g_strdup (share);
      data->auth_info->workgroup = g_strdup (workgroup);
      data->auth_info->username = g_strdup (username);
      data->auth_info->password = g_strdup (password);

      data->waiting = TRUE;

      source = g_idle_source_new ();
      g_source_set_callback (source,
                             get_auth_info,
                             data,
                             NULL);
      g_source_attach (source, data->context);
      g_source_unref (source);

      /*
       * smbclient needs to get authentication data
       * from this synchronous callback so we are blocking
       * until we get them
       */
      while (data->waiting)
        {
          g_usleep (POLL_DELAY);
        }

      if (g_strcmp0 (username, data->auth_info->username) != 0)
        g_strlcpy (username, data->auth_info->username, unmaxlen);

      if (g_strcmp0 (password, data->auth_info->password) != 0)
        g_strlcpy (password, data->auth_info->password, pwmaxlen);

      smb_auth_info_free (data->auth_info);
      data->auth_info = NULL;
    }
}

static void
anonymous_auth_fn (SMBCCTX    *smb_context,
                   const char *server,
                   const char *share,
                   char       *workgroup,
                   int         wgmaxlen,
                   char       *username,
                   int         unmaxlen,
                   char       *password,
                   int         pwmaxlen)
{
  username[0] = '\0';
  password[0] = '\0';
}

static void
list_dir (SMBCCTX      *smb_context,
          const gchar  *dirname,
          const gchar  *path,
          GCancellable *cancellable,
          SMBData      *data)
{
  struct smbc_dirent *dirent;
  smbc_closedir_fn    smbclient_closedir;
  smbc_readdir_fn     smbclient_readdir;
  smbc_opendir_fn     smbclient_opendir;
  PpPrintDevice      *device;
  const gchar        *host_name;
  SMBCFILE           *dir;

  if (!g_cancellable_is_cancelled (cancellable))
    {
      smbclient_closedir = smbc_getFunctionClosedir (smb_context);
      smbclient_readdir = smbc_getFunctionReaddir (smb_context);
      smbclient_opendir = smbc_getFunctionOpendir (smb_context);

      dir = smbclient_opendir (smb_context, dirname);
      if (!dir && errno == EACCES)
        {
          if (g_str_has_prefix (dirname, "smb://"))
            host_name = dirname + 6;
          else
            host_name = dirname;

          if (data->auth_if_needed)
            {
              data->cancelled = FALSE;
              smbc_setFunctionAuthDataWithContext (smb_context, auth_fn);
              dir = smbclient_opendir (smb_context, dirname);
              smbc_setFunctionAuthDataWithContext (smb_context, anonymous_auth_fn);

              if (data->cancelled)
                {
                  device = g_object_new (PP_TYPE_PRINT_DEVICE,
                                         "host-name", host_name,
                                         "is-authenticated-server", TRUE,
                                         NULL);

                  data->devices->devices = g_list_append (data->devices->devices, device);

                  if (dir)
                    smbclient_closedir (smb_context, dir);
                  return;
                }
            }
          else
            {
              device = g_object_new (PP_TYPE_PRINT_DEVICE,
                                     "host-name", host_name,
                                     "is-authenticated-server", TRUE,
                                     NULL);

              data->devices->devices = g_list_append (data->devices->devices, device);
            }
        }

      while (dir && (dirent = smbclient_readdir (smb_context, dir)))
        {
          gchar *device_name;
          gchar *device_uri;
          gchar *subdirname = NULL;
          gchar *subpath = NULL;
          gchar *uri;

          if (dirent->smbc_type == SMBC_WORKGROUP)
            {
              subdirname = g_strdup_printf ("%s%s", dirname, dirent->name);
              subpath = g_strdup_printf ("%s%s", path, dirent->name);
            }

          if (dirent->smbc_type == SMBC_SERVER)
            {
              subdirname = g_strdup_printf ("smb://%s", dirent->name);
              subpath = g_strdup_printf ("%s//%s", path, dirent->name);
            }

          if (dirent->smbc_type == SMBC_PRINTER_SHARE)
            {
              uri = g_strdup_printf ("%s/%s", dirname, dirent->name);
              device_uri = g_uri_escape_string (uri,
                                                G_URI_RESERVED_CHARS_GENERIC_DELIMITERS
                                                G_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS,
                                                FALSE);

              device_name = g_strdup (dirent->name);
              device_name = g_strcanon (device_name, ALLOWED_CHARACTERS, '-');

              device = g_object_new (PP_TYPE_PRINT_DEVICE,
                                     "device-uri", device_uri,
                                     "is-network-device", TRUE,
                                     "device-info", dirent->comment,
                                     "device-name", device_name,
                                     "acquisition-method", data->hostname_set ? ACQUISITION_METHOD_SAMBA_HOST : ACQUISITION_METHOD_SAMBA,
                                     "device-location", path,
                                     "host-name", dirname,
                                     NULL);

              g_free (device_name);
              g_free (device_uri);
              g_free (uri);

              data->devices->devices = g_list_append (data->devices->devices, device);
            }

          if (subdirname)
            {
              list_dir (smb_context,
                        subdirname,
                        subpath,
                        cancellable,
                        data);
              g_free (subdirname);
              g_free (subpath);
            }
        }

      if (dir)
        smbclient_closedir (smb_context, dir);
    }
}

static void
_pp_samba_get_devices_thread (GSimpleAsyncResult *res,
                              GObject            *object,
                              GCancellable       *cancellable)
{
  PpSambaPrivate *priv;
  static GMutex   mutex;
  PpSamba        *samba = PP_SAMBA (object);
  SMBData        *data;
  SMBCCTX        *smb_context;
  gchar          *dirname;
  gchar          *path;

  priv = samba->priv;

  data = g_simple_async_result_get_op_res_gpointer (res);
  data->devices = g_new0 (PpDevicesList, 1);
  data->devices->devices = NULL;

  g_mutex_lock (&mutex);

  smb_context = smbc_new_context ();
  if (smb_context)
    {
      if (smbc_init_context (smb_context))
        {
          smbc_setOptionUserData (smb_context, data);

          if (priv->hostname)
            {
              dirname = g_strdup_printf ("smb://%s", priv->hostname);
              path = g_strdup_printf ("//%s", priv->hostname);
            }
          else
            {
              dirname = g_strdup_printf ("smb://");
              path = g_strdup_printf ("//");
            }

          smbc_setFunctionAuthDataWithContext (smb_context, anonymous_auth_fn);
          list_dir (smb_context, dirname, path, cancellable, data);

          g_free (dirname);
          g_free (path);
        }

      smbc_free_context (smb_context, 1);
    }

  g_mutex_unlock (&mutex);
}

void
pp_samba_get_devices_async (PpSamba             *samba,
                            gboolean             auth_if_needed,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  GSimpleAsyncResult *res;
  PpSambaPrivate     *priv = samba->priv;
  SMBData            *data;

  res = g_simple_async_result_new (G_OBJECT (samba), callback, user_data, pp_samba_get_devices_async);
  data = g_new0 (SMBData, 1);
  data->devices = NULL;
  data->context = g_main_context_default ();
  data->hostname_set = priv->hostname != NULL;
  data->parent = g_object_ref (priv->parent);
  data->auth_if_needed = auth_if_needed;

  g_simple_async_result_set_check_cancellable (res, cancellable);
  g_simple_async_result_set_op_res_gpointer (res, data, (GDestroyNotify) smb_data_free);
  g_simple_async_result_run_in_thread (res, _pp_samba_get_devices_thread, 0, cancellable);

  g_object_unref (res);
}

PpDevicesList *
pp_samba_get_devices_finish (PpSamba       *samba,
                             GAsyncResult  *res,
                             GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  SMBData            *data;
  PpDevicesList      *result;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == pp_samba_get_devices_async);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  data = g_simple_async_result_get_op_res_gpointer (simple);
  result = data->devices;
  data->devices = NULL;

  return result;
}
