/*
 * Generated by gdbus-codegen 2.47.1. DO NOT EDIT.
 *
 * The license of this code is the same as for the source it was derived from.
 */

#ifndef __TIMEDATED_H__
#define __TIMEDATED_H__

#include <gio/gio.h>

G_BEGIN_DECLS


/* ------------------------------------------------------------------------ */
/* Declarations for org.freedesktop.timedate1 */

#define TYPE_TIMEDATE1 (timedate1_get_type ())
#define TIMEDATE1(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TIMEDATE1, Timedate1))
#define IS_TIMEDATE1(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TIMEDATE1))
#define TIMEDATE1_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), TYPE_TIMEDATE1, Timedate1Iface))

struct _Timedate1;
typedef struct _Timedate1 Timedate1;
typedef struct _Timedate1Iface Timedate1Iface;

struct _Timedate1Iface
{
  GTypeInterface parent_iface;


  gboolean (*handle_set_local_rtc) (
    Timedate1 *object,
    GDBusMethodInvocation *invocation,
    gboolean arg_local_rtc,
    gboolean arg_fix_system,
    gboolean arg_user_interaction);

  gboolean (*handle_set_ntp) (
    Timedate1 *object,
    GDBusMethodInvocation *invocation,
    gboolean arg_use_ntp,
    gboolean arg_user_interaction);

  gboolean (*handle_set_time) (
    Timedate1 *object,
    GDBusMethodInvocation *invocation,
    gint64 arg_usec_utc,
    gboolean arg_relative,
    gboolean arg_user_interaction);

  gboolean (*handle_set_timezone) (
    Timedate1 *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_timezone,
    gboolean arg_user_interaction);

  gboolean  (*get_can_ntp) (Timedate1 *object);

  gboolean  (*get_local_rtc) (Timedate1 *object);

  gboolean  (*get_ntp) (Timedate1 *object);

  const gchar * (*get_timezone) (Timedate1 *object);

};

GType timedate1_get_type (void) G_GNUC_CONST;

GDBusInterfaceInfo *timedate1_interface_info (void);
guint timedate1_override_properties (GObjectClass *klass, guint property_id_begin);


/* D-Bus method call completion functions: */
void timedate1_complete_set_time (
    Timedate1 *object,
    GDBusMethodInvocation *invocation);

void timedate1_complete_set_timezone (
    Timedate1 *object,
    GDBusMethodInvocation *invocation);

void timedate1_complete_set_local_rtc (
    Timedate1 *object,
    GDBusMethodInvocation *invocation);

void timedate1_complete_set_ntp (
    Timedate1 *object,
    GDBusMethodInvocation *invocation);



/* D-Bus method calls: */
void timedate1_call_set_time (
    Timedate1 *proxy,
    gint64 arg_usec_utc,
    gboolean arg_relative,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean timedate1_call_set_time_finish (
    Timedate1 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean timedate1_call_set_time_sync (
    Timedate1 *proxy,
    gint64 arg_usec_utc,
    gboolean arg_relative,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GError **error);

void timedate1_call_set_timezone (
    Timedate1 *proxy,
    const gchar *arg_timezone,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean timedate1_call_set_timezone_finish (
    Timedate1 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean timedate1_call_set_timezone_sync (
    Timedate1 *proxy,
    const gchar *arg_timezone,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GError **error);

void timedate1_call_set_local_rtc (
    Timedate1 *proxy,
    gboolean arg_local_rtc,
    gboolean arg_fix_system,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean timedate1_call_set_local_rtc_finish (
    Timedate1 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean timedate1_call_set_local_rtc_sync (
    Timedate1 *proxy,
    gboolean arg_local_rtc,
    gboolean arg_fix_system,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GError **error);

void timedate1_call_set_ntp (
    Timedate1 *proxy,
    gboolean arg_use_ntp,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean timedate1_call_set_ntp_finish (
    Timedate1 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean timedate1_call_set_ntp_sync (
    Timedate1 *proxy,
    gboolean arg_use_ntp,
    gboolean arg_user_interaction,
    GCancellable *cancellable,
    GError **error);



/* D-Bus property accessors: */
const gchar *timedate1_get_timezone (Timedate1 *object);
gchar *timedate1_dup_timezone (Timedate1 *object);
void timedate1_set_timezone (Timedate1 *object, const gchar *value);

gboolean timedate1_get_local_rtc (Timedate1 *object);
void timedate1_set_local_rtc (Timedate1 *object, gboolean value);

gboolean timedate1_get_can_ntp (Timedate1 *object);
void timedate1_set_can_ntp (Timedate1 *object, gboolean value);

gboolean timedate1_get_ntp (Timedate1 *object);
void timedate1_set_ntp (Timedate1 *object, gboolean value);


/* ---- */

#define TYPE_TIMEDATE1_PROXY (timedate1_proxy_get_type ())
#define TIMEDATE1_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TIMEDATE1_PROXY, Timedate1Proxy))
#define TIMEDATE1_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), TYPE_TIMEDATE1_PROXY, Timedate1ProxyClass))
#define TIMEDATE1_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_TIMEDATE1_PROXY, Timedate1ProxyClass))
#define IS_TIMEDATE1_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TIMEDATE1_PROXY))
#define IS_TIMEDATE1_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_TIMEDATE1_PROXY))

typedef struct _Timedate1Proxy Timedate1Proxy;
typedef struct _Timedate1ProxyClass Timedate1ProxyClass;
typedef struct _Timedate1ProxyPrivate Timedate1ProxyPrivate;

struct _Timedate1Proxy
{
  /*< private >*/
  GDBusProxy parent_instance;
  Timedate1ProxyPrivate *priv;
};

struct _Timedate1ProxyClass
{
  GDBusProxyClass parent_class;
};

GType timedate1_proxy_get_type (void) G_GNUC_CONST;

void timedate1_proxy_new (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
Timedate1 *timedate1_proxy_new_finish (
    GAsyncResult        *res,
    GError             **error);
Timedate1 *timedate1_proxy_new_sync (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);

void timedate1_proxy_new_for_bus (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
Timedate1 *timedate1_proxy_new_for_bus_finish (
    GAsyncResult        *res,
    GError             **error);
Timedate1 *timedate1_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);


/* ---- */

#define TYPE_TIMEDATE1_SKELETON (timedate1_skeleton_get_type ())
#define TIMEDATE1_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_TIMEDATE1_SKELETON, Timedate1Skeleton))
#define TIMEDATE1_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), TYPE_TIMEDATE1_SKELETON, Timedate1SkeletonClass))
#define TIMEDATE1_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_TIMEDATE1_SKELETON, Timedate1SkeletonClass))
#define IS_TIMEDATE1_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_TIMEDATE1_SKELETON))
#define IS_TIMEDATE1_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_TIMEDATE1_SKELETON))

typedef struct _Timedate1Skeleton Timedate1Skeleton;
typedef struct _Timedate1SkeletonClass Timedate1SkeletonClass;
typedef struct _Timedate1SkeletonPrivate Timedate1SkeletonPrivate;

struct _Timedate1Skeleton
{
  /*< private >*/
  GDBusInterfaceSkeleton parent_instance;
  Timedate1SkeletonPrivate *priv;
};

struct _Timedate1SkeletonClass
{
  GDBusInterfaceSkeletonClass parent_class;
};

GType timedate1_skeleton_get_type (void) G_GNUC_CONST;

Timedate1 *timedate1_skeleton_new (void);


G_END_DECLS

#endif /* __TIMEDATED_H__ */
