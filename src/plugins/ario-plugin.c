/*
 * heavily based on code from Gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
 * Copyright (C) 2008 Marc Pavot <marc.pavot@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugins/ario-plugin.h"

G_DEFINE_TYPE(ArioPlugin, ario_plugin, G_TYPE_OBJECT)

static void
dummy (ArioPlugin *plugin, ArioShell *shell)
{
        /* Empty */
}

static GtkWidget *
create_configure_dialog (ArioPlugin *plugin)
{
        return NULL;
}

static gboolean
is_configurable (ArioPlugin *plugin)
{
        return (ARIO_PLUGIN_GET_CLASS (plugin)->create_configure_dialog !=
                create_configure_dialog);
}

static void 
ario_plugin_class_init (ArioPluginClass *klass)
{
        klass->activate = dummy;
        klass->deactivate = dummy;
        
        klass->create_configure_dialog = create_configure_dialog;
        klass->is_configurable = is_configurable;
}

static void
ario_plugin_init (ArioPlugin *plugin)
{
        /* Empty */
}

void
ario_plugin_activate (ArioPlugin *plugin,
                       ArioShell *shell)
{
        g_return_if_fail (ARIO_IS_PLUGIN (plugin));
        g_return_if_fail (IS_ARIO_SHELL (shell));
        
        ARIO_PLUGIN_GET_CLASS (plugin)->activate (plugin, shell);
}

void
ario_plugin_deactivate (ArioPlugin *plugin,
                         ArioShell *shell)
{
        g_return_if_fail (ARIO_IS_PLUGIN (plugin));
        g_return_if_fail (IS_ARIO_SHELL (shell));

        ARIO_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, shell);
}

gboolean
ario_plugin_is_configurable (ArioPlugin *plugin)
{
        g_return_val_if_fail (ARIO_IS_PLUGIN (plugin), FALSE);

        return ARIO_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

GtkWidget *
ario_plugin_create_configure_dialog (ArioPlugin *plugin)
{
        g_return_val_if_fail (ARIO_IS_PLUGIN (plugin), NULL);
        
        return ARIO_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}
