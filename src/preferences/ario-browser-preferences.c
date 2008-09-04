/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "preferences/ario-browser-preferences.h"
#include "preferences/ario-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "lib/ario-conf.h"
#include "sources/ario-browser.h"
#include "sources/ario-tree.h"
#include "ario-debug.h"

static void ario_browser_preferences_class_init (ArioBrowserPreferencesClass *klass);
static void ario_browser_preferences_init (ArioBrowserPreferences *browser_preferences);
static void ario_browser_preferences_finalize (GObject *object);
static void ario_browser_preferences_sync_browser (ArioBrowserPreferences *browser_preferences);
G_MODULE_EXPORT void ario_browser_preferences_sort_changed_cb (GtkComboBoxEntry *combobox,
                                                                 ArioBrowserPreferences *browser_preferences);
G_MODULE_EXPORT void ario_browser_preferences_treesnb_changed_cb (GtkWidget *widget,
                                                                  ArioBrowserPreferences *browser_preferences);
static void ario_browser_preferences_tree_combobox_changed_cb (GtkComboBox *widget,
                                                               ArioBrowserPreferences *browser_preferences);


static const char *sort_behavior[] = {
        N_("Alphabetically"),   // SORT_ALPHABETICALLY
        N_("By year"),          // SORT_YEAR
        NULL
};

struct ArioBrowserPreferencesPrivate
{
        GtkWidget *sort_combobox;
        GtkWidget *treesnb_spinbutton;
        GtkWidget *hbox;
        GSList *tree_comboboxs;
};

static GObjectClass *parent_class = NULL;

GType
ario_browser_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_browser_preferences_type = 0;

        if (ario_browser_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioBrowserPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_browser_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioBrowserPreferences),
                        0,
                        (GInstanceInitFunc) ario_browser_preferences_init
                };

                ario_browser_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                          "ArioBrowserPreferences",
                                                                          &our_info, 0);
        }

        return ario_browser_preferences_type;
}

static void
ario_browser_preferences_class_init (ArioBrowserPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_browser_preferences_finalize;
}

static void
ario_browser_preferences_init (ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START
        browser_preferences->priv = g_new0 (ArioBrowserPreferencesPrivate, 1);
}

GtkWidget *
ario_browser_preferences_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowserPreferences *browser_preferences;
        GladeXML *xml;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int i;

        browser_preferences = g_object_new (TYPE_ARIO_BROWSER_PREFERENCES, NULL);

        g_return_val_if_fail (browser_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "browser-prefs.glade",
                                "browser_vbox",
                                browser_preferences);

        browser_preferences->priv->sort_combobox = 
                glade_xml_get_widget (xml, "sort_combobox");
        browser_preferences->priv->hbox = 
                glade_xml_get_widget (xml, "trees_hbox");
        browser_preferences->priv->treesnb_spinbutton = 
                glade_xml_get_widget (xml, "treesnb_spinbutton");

        rb_glade_boldify_label (xml, "options_label");
        rb_glade_boldify_label (xml, "organisation_label");

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
        for (i = 0; i < SORT_N_BEHAVIOR; ++i) {
                gtk_list_store_append (list_store, &iter);
                gtk_list_store_set (list_store, &iter,
                                    0, gettext (sort_behavior[i]),
                                    1, i,
                                    -1);
        }
        gtk_combo_box_set_model (GTK_COMBO_BOX (browser_preferences->priv->sort_combobox),
                                 GTK_TREE_MODEL (list_store));
        g_object_unref (list_store);

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_clear (GTK_CELL_LAYOUT (browser_preferences->priv->sort_combobox));
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (browser_preferences->priv->sort_combobox), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (browser_preferences->priv->sort_combobox), renderer,
                                        "text", 0, NULL);

        ario_browser_preferences_sync_browser (browser_preferences);

        gtk_box_pack_start (GTK_BOX (browser_preferences), glade_xml_get_widget (xml, "browser_vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (browser_preferences);
}

static void
ario_browser_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioBrowserPreferences *browser_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_BROWSER_PREFERENCES (object));

        browser_preferences = ARIO_BROWSER_PREFERENCES (object);

        g_return_if_fail (browser_preferences->priv != NULL);

        g_free (browser_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_browser_preferences_sync_browser (ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START
        GtkWidget *tree_combobox;
        int i, j;
        gchar **splited_conf;
        gchar *conf;
        GSList *tmp;
        GtkListStore *list_store;
        GtkCellRenderer *renderer;
        GtkTreeIter iter;
        int a, b;

        gtk_combo_box_set_active (GTK_COMBO_BOX (browser_preferences->priv->sort_combobox),
                                  ario_conf_get_integer (PREF_ALBUM_SORT, PREF_ALBUM_SORT_DEFAULT));

        /* Remove all trees */
        for (tmp = browser_preferences->priv->tree_comboboxs; tmp; tmp = g_slist_next (tmp)) {
                gtk_container_remove (GTK_CONTAINER (browser_preferences->priv->hbox), GTK_WIDGET (tmp->data));
        }
        g_slist_free (browser_preferences->priv->tree_comboboxs);
        browser_preferences->priv->tree_comboboxs = NULL;

        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);
        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        g_free (conf);
        for (i = 0; splited_conf[i]; ++i) {
                tree_combobox = gtk_combo_box_new ();
                browser_preferences->priv->tree_comboboxs = g_slist_append (browser_preferences->priv->tree_comboboxs, tree_combobox);

                list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
                for (j = 0; j < MPD_TAG_NUM_OF_ITEM_TYPES - 1; ++j) {
                        if (ario_mpd_get_items_names ()[j]) {
                                gtk_list_store_append (list_store, &iter);
                                gtk_list_store_set (list_store, &iter,
                                                0, gettext (ario_mpd_get_items_names ()[j]),
                                                1, j,
                                                -1);
                        }
                }
                gtk_combo_box_set_model (GTK_COMBO_BOX (tree_combobox),
                                         GTK_TREE_MODEL (list_store));
                g_object_unref (list_store);

                renderer = gtk_cell_renderer_text_new ();
                gtk_cell_layout_clear (GTK_CELL_LAYOUT (tree_combobox));
                gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (tree_combobox), renderer, TRUE);
                gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (tree_combobox), renderer,
                                                "text", 0, NULL);

                a = atoi (splited_conf[i]);
                b = 0;
                for (j = 0; j < MPD_TAG_NUM_OF_ITEM_TYPES - 1 && j < a; ++j) {
                        if (ario_mpd_get_items_names ()[j])
                                ++b;                        
                }
                gtk_combo_box_set_active (GTK_COMBO_BOX (tree_combobox), b);

                g_signal_connect (G_OBJECT (tree_combobox),
                                  "changed", G_CALLBACK (ario_browser_preferences_tree_combobox_changed_cb), browser_preferences);

                gtk_box_pack_start (GTK_BOX (browser_preferences->priv->hbox), tree_combobox, TRUE, TRUE, 0);
        }
        gtk_widget_show_all (browser_preferences->priv->hbox);
        g_strfreev (splited_conf);

        gtk_spin_button_set_value (GTK_SPIN_BUTTON (browser_preferences->priv->treesnb_spinbutton), (gdouble) i);
}

void
ario_browser_preferences_sort_changed_cb (GtkComboBoxEntry *combobox,
                                          ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START
        int i;

        i = gtk_combo_box_get_active (GTK_COMBO_BOX (browser_preferences->priv->sort_combobox));

        ario_conf_set_integer (PREF_ALBUM_SORT, 
                               i);
}

void
ario_browser_preferences_treesnb_changed_cb (GtkWidget *widget,
                                             ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START
        gchar **splited_conf;
        gchar *conf, *new_conf, *tmp;
        int old_nb, new_nb, i;

        conf = ario_conf_get_string (PREF_BROWSER_TREES, PREF_BROWSER_TREES_DEFAULT);

        splited_conf = g_strsplit (conf, ",", MAX_TREE_NB);
        for (old_nb = 0; splited_conf[old_nb]; ++old_nb) {}

        new_nb = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (browser_preferences->priv->treesnb_spinbutton));
        if (new_nb > old_nb) {
                new_conf = g_strdup_printf ("%s,0", conf);
                ario_conf_set_string (PREF_BROWSER_TREES, new_conf);
                g_free (new_conf);
                ario_browser_preferences_sync_browser (browser_preferences);
        } else if (new_nb < old_nb) {
                new_conf = g_strdup (splited_conf[0]);
                for (i = 1; i < new_nb; ++i) {
                        tmp = g_strdup_printf ("%s,%s", new_conf, splited_conf[i]);
                        g_free (new_conf);
                        new_conf = tmp;
                }
                ario_conf_set_string (PREF_BROWSER_TREES, new_conf);
                g_free (new_conf);
                ario_browser_preferences_sync_browser (browser_preferences);
        }
        g_strfreev (splited_conf);
        g_free (conf);
}

static void
ario_browser_preferences_tree_combobox_changed_cb (GtkComboBox *widget,
                                                   ArioBrowserPreferences *browser_preferences)
{
        ARIO_LOG_FUNCTION_START
        GSList *temp;
        GtkComboBox *combobox;
        gchar *conf = NULL, *tmp;
        GValue *value;
        GtkTreeIter iter;

        for (temp = browser_preferences->priv->tree_comboboxs; temp; temp = g_slist_next (temp)) {
                combobox = temp->data;
                gtk_combo_box_get_active_iter (combobox, &iter);
                value = (GValue*)g_malloc(sizeof(GValue));
                value->g_type = 0;
                gtk_tree_model_get_value (gtk_combo_box_get_model (combobox),
                                          &iter,
                                          1, value);
                if (!conf) {
                        conf = g_strdup_printf ("%d", g_value_get_int (value));
                } else {
                        tmp = g_strdup_printf ("%s,%d", conf, g_value_get_int (value));
                        g_free (conf);
                        conf = tmp;
                }
                g_free (value);
        }
        ario_conf_set_string (PREF_BROWSER_TREES, conf);
        g_free (conf);
}
