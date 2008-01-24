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
#include "preferences/ario-stats-preferences.h"
#include "lib/rb-glade-helpers.h"
#include "ario-debug.h"
#include "ario-util.h"

static void ario_stats_preferences_class_init (ArioStatsPreferencesClass *klass);
static void ario_stats_preferences_init (ArioStatsPreferences *stats_preferences);
static void ario_stats_preferences_finalize (GObject *object);
static void ario_stats_preferences_set_property (GObject *object,
                                                      guint prop_id,
                                                      const GValue *value,
                                                      GParamSpec *pspec);
static void ario_stats_preferences_get_property (GObject *object,
                                                      guint prop_id,
                                                      GValue *value,
                                                      GParamSpec *pspec);
static void ario_stats_preferences_sync_stats (ArioStatsPreferences *stats_preferences);
static void ario_stats_preferences_stats_changed_cb (ArioMpd *mpd,
                                                     ArioStatsPreferences *stats_preferences);

enum
{
        PROP_0,
        PROP_MPD
};

struct ArioStatsPreferencesPrivate
{
        ArioMpd *mpd;

        GtkWidget *nbartists_label;
        GtkWidget *nbalbums_label;
        GtkWidget *nbsongs_label;
        GtkWidget *uptime_label;
        GtkWidget *playtime_label;
        GtkWidget *dbplay_time_label;
};

static GObjectClass *parent_class = NULL;

GType
ario_stats_preferences_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType ario_stats_preferences_type = 0;

        if (ario_stats_preferences_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioStatsPreferencesClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_stats_preferences_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioStatsPreferences),
                        0,
                        (GInstanceInitFunc) ario_stats_preferences_init
                };

                ario_stats_preferences_type = g_type_register_static (GTK_TYPE_VBOX,
                                                                      "ArioStatsPreferences",
                                                                      &our_info, 0);
        }

        return ario_stats_preferences_type;
}

static void
ario_stats_preferences_class_init (ArioStatsPreferencesClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_stats_preferences_finalize;
        object_class->set_property = ario_stats_preferences_set_property;
        object_class->get_property = ario_stats_preferences_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_ARIO_MPD,
                                                              G_PARAM_READWRITE));
}

static void
ario_stats_preferences_init (ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START
        stats_preferences->priv = g_new0 (ArioStatsPreferencesPrivate, 1);
}

GtkWidget *
ario_stats_preferences_new (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GladeXML *xml;
        ArioStatsPreferences *stats_preferences;

        stats_preferences = g_object_new (TYPE_ARIO_STATS_PREFERENCES,
                                          "mpd", mpd,
                                          NULL);

        g_return_val_if_fail (stats_preferences->priv != NULL, NULL);

        xml = rb_glade_xml_new (GLADE_PATH "stats-prefs.glade",
                                "vbox",
                                stats_preferences);

        stats_preferences->priv->nbartists_label = 
                glade_xml_get_widget (xml, "nbartists_label");
        stats_preferences->priv->nbalbums_label = 
                glade_xml_get_widget (xml, "nbalbums_label");
        stats_preferences->priv->nbsongs_label = 
                glade_xml_get_widget (xml, "nbsongs_label");
        stats_preferences->priv->uptime_label = 
                glade_xml_get_widget (xml, "uptime_label");
        stats_preferences->priv->playtime_label = 
                glade_xml_get_widget (xml, "playtime_label");
        stats_preferences->priv->dbplay_time_label = 
                glade_xml_get_widget (xml, "dbplay_time_label");
        gtk_widget_set_size_request(stats_preferences->priv->nbartists_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->nbalbums_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->nbsongs_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->uptime_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->playtime_label, 250, -1);
        gtk_widget_set_size_request(stats_preferences->priv->dbplay_time_label, 250, -1);

        ario_stats_preferences_sync_stats (stats_preferences);

        gtk_box_pack_start (GTK_BOX (stats_preferences), glade_xml_get_widget (xml, "vbox"), TRUE, TRUE, 0);

        g_object_unref (G_OBJECT (xml));

        return GTK_WIDGET (stats_preferences);
}

static void
ario_stats_preferences_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioStatsPreferences *stats_preferences;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_STATS_PREFERENCES (object));

        stats_preferences = ARIO_STATS_PREFERENCES (object);

        g_return_if_fail (stats_preferences->priv != NULL);

        g_free (stats_preferences->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_stats_preferences_set_property (GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatsPreferences *stats_preferences = ARIO_STATS_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                stats_preferences->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (stats_preferences->priv->mpd),
                                         "state_changed", G_CALLBACK (ario_stats_preferences_stats_changed_cb),
                                         stats_preferences, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_stats_preferences_get_property (GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioStatsPreferences *stats_preferences = ARIO_STATS_PREFERENCES (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, stats_preferences->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ario_stats_preferences_sync_stats (ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START
        ArioMpdStats *stats = ario_mpd_get_stats (stats_preferences->priv->mpd);
        gchar *tmp;

        if (stats) {
                tmp = g_strdup_printf ("%d", stats->numberOfArtists);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbartists_label), tmp);
                g_free (tmp);

                tmp = g_strdup_printf ("%d", stats->numberOfAlbums);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbalbums_label), tmp);
                g_free (tmp);

                tmp = g_strdup_printf ("%d", stats->numberOfSongs);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbsongs_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->uptime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->uptime_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->playTime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->playtime_label), tmp);
                g_free (tmp);

                tmp = ario_util_format_total_time (stats->dbPlayTime);
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->dbplay_time_label), tmp);
                g_free (tmp);
        } else {
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbartists_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbalbums_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->nbsongs_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->uptime_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->playtime_label), _("Not connected"));
                gtk_label_set_text (GTK_LABEL (stats_preferences->priv->dbplay_time_label), _("Not connected"));
        }
}

static void
ario_stats_preferences_stats_changed_cb (ArioMpd *mpd,
                                          ArioStatsPreferences *stats_preferences)
{
        ARIO_LOG_FUNCTION_START
        ario_stats_preferences_sync_stats (stats_preferences);
}