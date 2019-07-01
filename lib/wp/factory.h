/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIREPLUMBER_FACTORY_H__
#define __WIREPLUMBER_FACTORY_H__

#include <gio/gio.h>

#include "core.h"

G_BEGIN_DECLS

#define WP_TYPE_FACTORY (wp_factory_get_type ())
G_DECLARE_FINAL_TYPE (WpFactory, wp_factory, WP, FACTORY, GObject)

typedef void (*WpFactoryFunc) (WpFactory * self, GType type,
    GVariant * properties, GAsyncReadyCallback ready, gpointer user_data);

WpFactory * wp_factory_new (WpCore * core, const gchar * name,
    WpFactoryFunc func);

const gchar * wp_factory_get_name (WpFactory * self);
WpCore * wp_factory_get_core (WpFactory * self);
void wp_factory_create_object (WpFactory * self, GType type,
    GVariant * properties, GAsyncReadyCallback ready, gpointer user_data);

WpFactory * wp_factory_find (WpCore * core, const gchar * name);
void wp_factory_make (WpCore * core, const gchar * name, GType type,
    GVariant * properties, GAsyncReadyCallback ready, gpointer user_data);

G_END_DECLS

#endif
