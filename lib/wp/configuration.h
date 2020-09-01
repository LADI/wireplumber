/* WirePlumber
 *
 * Copyright © 2019 Collabora Ltd.
 *    @author Julian Bouzas <julian.bouzas@collabora.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIREPLUMBER_CONFIGURATION_H__
#define __WIREPLUMBER_CONFIGURATION_H__

#include "core.h"

G_BEGIN_DECLS

/* WpConfigParser */

/**
 * WP_TYPE_CONFIG_PARSER:
 *
 * The #WpConfigParser #GType
 */
#define WP_TYPE_CONFIG_PARSER (wp_config_parser_get_type ())
WP_API
G_DECLARE_INTERFACE (WpConfigParser, wp_config_parser, WP,
  CONFIG_PARSER, GObject)

/**
 * WpConfigParserInterface:
 * @add_file: See wp_config_parser_add_file()
 * @get_matched_data: See wp_config_parser_get_matched_data()
 * @reset: See wp_config_parser_reset()
 */
struct _WpConfigParserInterface
{
  GTypeInterface parent;

  gboolean (*add_file) (WpConfigParser *self, const gchar *location);
  gconstpointer (*get_matched_data) (WpConfigParser *self, gpointer data);
  void (*reset) (WpConfigParser *self);
};

WP_API
gboolean wp_config_parser_add_file (WpConfigParser *self, const char *location);

WP_API
gconstpointer wp_config_parser_get_matched_data (WpConfigParser *self,
    gpointer data);

WP_API
void wp_config_parser_reset (WpConfigParser *self);

/* WpConfiguration */

/**
 * WP_TYPE_CONFIGURATION:
 *
 * The #WpConfiguration #GType
 */
#define WP_TYPE_CONFIGURATION (wp_configuration_get_type ())
WP_API
G_DECLARE_FINAL_TYPE (WpConfiguration, wp_configuration, WP, CONFIGURATION,
  GObject)

WP_API
WpConfiguration * wp_configuration_get_instance (WpCore *core);

WP_API
void wp_configuration_add_path (WpConfiguration *self, const char *path);

WP_API
void wp_configuration_remove_path (WpConfiguration *self, const char *path);

WP_API
gchar * wp_configuration_find_file (WpConfiguration * self,
    const gchar * filename);

WP_API
gboolean wp_configuration_add_extension (WpConfiguration *self,
    const gchar * extension, GType parser_type);

WP_API
gboolean wp_configuration_remove_extension (WpConfiguration *self,
    const gchar * extension);

WP_API
WpConfigParser *wp_configuration_get_parser (WpConfiguration *self,
    const char *extension);

WP_API
void wp_configuration_reload (WpConfiguration *self, const char *extension);

G_END_DECLS

#endif
