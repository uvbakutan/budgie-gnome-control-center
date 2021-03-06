/*
 * Generated by gdbus-codegen 2.43.92. DO NOT EDIT.
 *
 * The license of this code is the same as for the source it was derived from.
 */

#ifndef __CC_SHELL_SEARCH_PROVIDER_GENERATED_H__
#define __CC_SHELL_SEARCH_PROVIDER_GENERATED_H__

#include <gio/gio.h>

G_BEGIN_DECLS


/* ------------------------------------------------------------------------ */
/* Declarations for org.gnome.Shell.SearchProvider2 */

#define CC_TYPE_SHELL_SEARCH_PROVIDER2 (cc_shell_search_provider2_get_type ())
#define CC_SHELL_SEARCH_PROVIDER2(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2, CcShellSearchProvider2))
#define CC_IS_SHELL_SEARCH_PROVIDER2(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2))
#define CC_SHELL_SEARCH_PROVIDER2_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2, CcShellSearchProvider2Iface))

struct _CcShellSearchProvider2;
typedef struct _CcShellSearchProvider2 CcShellSearchProvider2;
typedef struct _CcShellSearchProvider2Iface CcShellSearchProvider2Iface;

struct _CcShellSearchProvider2Iface
{
  GTypeInterface parent_iface;

  gboolean (*handle_activate_result) (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_identifier,
    const gchar *const *arg_terms,
    guint arg_timestamp);

  gboolean (*handle_get_initial_result_set) (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *arg_terms);

  gboolean (*handle_get_result_metas) (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *arg_identifiers);

  gboolean (*handle_get_subsearch_result_set) (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *arg_previous_results,
    const gchar *const *arg_terms);

  gboolean (*handle_launch_search) (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *arg_terms,
    guint arg_timestamp);

};

GType cc_shell_search_provider2_get_type (void) G_GNUC_CONST;

GDBusInterfaceInfo *cc_shell_search_provider2_interface_info (void);
guint cc_shell_search_provider2_override_properties (GObjectClass *klass, guint property_id_begin);


/* D-Bus method call completion functions: */
void cc_shell_search_provider2_complete_get_initial_result_set (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *results);

void cc_shell_search_provider2_complete_get_subsearch_result_set (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    const gchar *const *results);

void cc_shell_search_provider2_complete_get_result_metas (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation,
    GVariant *metas);

void cc_shell_search_provider2_complete_activate_result (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation);

void cc_shell_search_provider2_complete_launch_search (
    CcShellSearchProvider2 *object,
    GDBusMethodInvocation *invocation);



/* D-Bus method calls: */
void cc_shell_search_provider2_call_get_initial_result_set (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_terms,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean cc_shell_search_provider2_call_get_initial_result_set_finish (
    CcShellSearchProvider2 *proxy,
    gchar ***out_results,
    GAsyncResult *res,
    GError **error);

gboolean cc_shell_search_provider2_call_get_initial_result_set_sync (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_terms,
    gchar ***out_results,
    GCancellable *cancellable,
    GError **error);

void cc_shell_search_provider2_call_get_subsearch_result_set (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_previous_results,
    const gchar *const *arg_terms,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean cc_shell_search_provider2_call_get_subsearch_result_set_finish (
    CcShellSearchProvider2 *proxy,
    gchar ***out_results,
    GAsyncResult *res,
    GError **error);

gboolean cc_shell_search_provider2_call_get_subsearch_result_set_sync (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_previous_results,
    const gchar *const *arg_terms,
    gchar ***out_results,
    GCancellable *cancellable,
    GError **error);

void cc_shell_search_provider2_call_get_result_metas (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_identifiers,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean cc_shell_search_provider2_call_get_result_metas_finish (
    CcShellSearchProvider2 *proxy,
    GVariant **out_metas,
    GAsyncResult *res,
    GError **error);

gboolean cc_shell_search_provider2_call_get_result_metas_sync (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_identifiers,
    GVariant **out_metas,
    GCancellable *cancellable,
    GError **error);

void cc_shell_search_provider2_call_activate_result (
    CcShellSearchProvider2 *proxy,
    const gchar *arg_identifier,
    const gchar *const *arg_terms,
    guint arg_timestamp,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean cc_shell_search_provider2_call_activate_result_finish (
    CcShellSearchProvider2 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean cc_shell_search_provider2_call_activate_result_sync (
    CcShellSearchProvider2 *proxy,
    const gchar *arg_identifier,
    const gchar *const *arg_terms,
    guint arg_timestamp,
    GCancellable *cancellable,
    GError **error);

void cc_shell_search_provider2_call_launch_search (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_terms,
    guint arg_timestamp,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean cc_shell_search_provider2_call_launch_search_finish (
    CcShellSearchProvider2 *proxy,
    GAsyncResult *res,
    GError **error);

gboolean cc_shell_search_provider2_call_launch_search_sync (
    CcShellSearchProvider2 *proxy,
    const gchar *const *arg_terms,
    guint arg_timestamp,
    GCancellable *cancellable,
    GError **error);



/* ---- */

#define CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY (cc_shell_search_provider2_proxy_get_type ())
#define CC_SHELL_SEARCH_PROVIDER2_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY, CcShellSearchProvider2Proxy))
#define CC_SHELL_SEARCH_PROVIDER2_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY, CcShellSearchProvider2ProxyClass))
#define CC_SHELL_SEARCH_PROVIDER2_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY, CcShellSearchProvider2ProxyClass))
#define CC_IS_SHELL_SEARCH_PROVIDER2_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY))
#define CC_IS_SHELL_SEARCH_PROVIDER2_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), CC_TYPE_SHELL_SEARCH_PROVIDER2_PROXY))

typedef struct _CcShellSearchProvider2Proxy CcShellSearchProvider2Proxy;
typedef struct _CcShellSearchProvider2ProxyClass CcShellSearchProvider2ProxyClass;
typedef struct _CcShellSearchProvider2ProxyPrivate CcShellSearchProvider2ProxyPrivate;

struct _CcShellSearchProvider2Proxy
{
  /*< private >*/
  GDBusProxy parent_instance;
  CcShellSearchProvider2ProxyPrivate *priv;
};

struct _CcShellSearchProvider2ProxyClass
{
  GDBusProxyClass parent_class;
};

GType cc_shell_search_provider2_proxy_get_type (void) G_GNUC_CONST;

void cc_shell_search_provider2_proxy_new (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
CcShellSearchProvider2 *cc_shell_search_provider2_proxy_new_finish (
    GAsyncResult        *res,
    GError             **error);
CcShellSearchProvider2 *cc_shell_search_provider2_proxy_new_sync (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);

void cc_shell_search_provider2_proxy_new_for_bus (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
CcShellSearchProvider2 *cc_shell_search_provider2_proxy_new_for_bus_finish (
    GAsyncResult        *res,
    GError             **error);
CcShellSearchProvider2 *cc_shell_search_provider2_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);


/* ---- */

#define CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON (cc_shell_search_provider2_skeleton_get_type ())
#define CC_SHELL_SEARCH_PROVIDER2_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON, CcShellSearchProvider2Skeleton))
#define CC_SHELL_SEARCH_PROVIDER2_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON, CcShellSearchProvider2SkeletonClass))
#define CC_SHELL_SEARCH_PROVIDER2_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON, CcShellSearchProvider2SkeletonClass))
#define CC_IS_SHELL_SEARCH_PROVIDER2_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON))
#define CC_IS_SHELL_SEARCH_PROVIDER2_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), CC_TYPE_SHELL_SEARCH_PROVIDER2_SKELETON))

typedef struct _CcShellSearchProvider2Skeleton CcShellSearchProvider2Skeleton;
typedef struct _CcShellSearchProvider2SkeletonClass CcShellSearchProvider2SkeletonClass;
typedef struct _CcShellSearchProvider2SkeletonPrivate CcShellSearchProvider2SkeletonPrivate;

struct _CcShellSearchProvider2Skeleton
{
  /*< private >*/
  GDBusInterfaceSkeleton parent_instance;
  CcShellSearchProvider2SkeletonPrivate *priv;
};

struct _CcShellSearchProvider2SkeletonClass
{
  GDBusInterfaceSkeletonClass parent_class;
};

GType cc_shell_search_provider2_skeleton_get_type (void) G_GNUC_CONST;

CcShellSearchProvider2 *cc_shell_search_provider2_skeleton_new (void);


G_END_DECLS

#endif /* __CC_SHELL_SEARCH_PROVIDER_GENERATED_H__ */
