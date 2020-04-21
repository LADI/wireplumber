/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author George Kiagiadakis <george.kiagiadakis@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIREPLUMBER_OBJECT_MANAGER_H__
#define __WIREPLUMBER_OBJECT_MANAGER_H__

#include <glib-object.h>
#include "proxy.h"
#include "iterator.h"

G_BEGIN_DECLS

/**
 * WpObjectManagerConstraintType:
 * @WP_OBJECT_MANAGER_CONSTRAINT_PW_GLOBAL_PROPERTY: constraint applies
 *   to a PipeWire global property of an object (the ones returned by
 *   wp_proxy_get_global_properties())
 * @WP_OBJECT_MANAGER_CONSTRAINT_PW_PROPERTY: constraint applies
 *   to a PipeWire property of the object (the ones returned by
 *   wp_proxy_get_properties())
 * @WP_OBJECT_MANAGER_CONSTRAINT_G_PROPERTY: constraint applies to a #GObject
 *   property of the managed object
 */
typedef enum {
  WP_OBJECT_MANAGER_CONSTRAINT_PW_GLOBAL_PROPERTY,
  WP_OBJECT_MANAGER_CONSTRAINT_PW_PROPERTY,
  WP_OBJECT_MANAGER_CONSTRAINT_G_PROPERTY,
} WpObjectManagerConstraintType;

/**
 * WP_TYPE_OBJECT_MANAGER:
 *
 * The #WpObjectManager #GType
 */
#define WP_TYPE_OBJECT_MANAGER (wp_object_manager_get_type ())
WP_API
G_DECLARE_FINAL_TYPE (WpObjectManager, wp_object_manager, WP, OBJECT_MANAGER, GObject)

WP_API
WpObjectManager * wp_object_manager_new (void);

WP_API
void wp_object_manager_add_interest (WpObjectManager *self,
    GType gtype, GVariant * constraints, WpProxyFeatures wanted_features);

WP_API
guint wp_object_manager_get_n_objects (WpObjectManager * self);

WP_API
WpIterator * wp_object_manager_iterate (WpObjectManager * self);

WP_API
WpProxy * wp_object_manager_find_proxy (WpObjectManager *self, guint bound_id);

G_END_DECLS

#endif
