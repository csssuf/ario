/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "covers/ario-cover-local.h"
#include "covers/ario-cover.h"
#include "ario-mpd.h"
#include "ario-util.h"
#include "preferences/ario-preferences.h"
#include "ario-debug.h"

static void ario_cover_local_class_init (ArioCoverLocalClass *klass);
static void ario_cover_local_init (ArioCoverLocal *cover_local);
static void ario_cover_local_finalize (GObject *object);
gboolean ario_cover_local_get_covers (ArioCoverProvider *cover_provider,
                                      const char *artist,
                                      const char *album,
                                      const char *file,
                                      GArray **file_size,
                                      GSList **file_contents,
                                      ArioCoverProviderOperation operation);

struct ArioCoverLocalPrivate
{
        gboolean dummy;
};

static const char *valid_cover_names[] = {
	"folder.png",
	".folder.png",
	"cover.png",
	"folder.jpg",
	".folder.jpg",
	"cover.jpg",
	NULL
};

static GObjectClass *parent_class = NULL;

GType
ario_cover_local_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioCoverLocalClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_cover_local_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioCoverLocal),
                        0,
                        (GInstanceInitFunc) ario_cover_local_init
                };

                type = g_type_register_static (ARIO_TYPE_COVER_PROVIDER,
                                               "ArioCoverLocal",
                                               &our_info, 0);
        }
        return type;
}

static gchar *
ario_cover_local_get_id (ArioCoverProvider *cover_provider)
{
        return "local";
}

static gchar *
ario_cover_local_get_name (ArioCoverProvider *cover_provider)
{
        return _("Music Directory");
}

static void
ario_cover_local_class_init (ArioCoverLocalClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioCoverProviderClass *cover_provider_class = ARIO_COVER_PROVIDER_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_cover_local_finalize;

        cover_provider_class->get_id = ario_cover_local_get_id;
        cover_provider_class->get_name = ario_cover_local_get_name;
        cover_provider_class->get_covers = ario_cover_local_get_covers;
}

static void
ario_cover_local_init (ArioCoverLocal *cover_local)
{
        ARIO_LOG_FUNCTION_START
        cover_local->priv = g_new0 (ArioCoverLocalPrivate, 1);
}

static void
ario_cover_local_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverLocal *cover_local;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_COVER_LOCAL (object));

        cover_local = ARIO_COVER_LOCAL (object);

        g_return_if_fail (cover_local->priv != NULL);
        g_free (cover_local->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

ArioCoverProvider*
ario_cover_local_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioCoverLocal *local;

        local = g_object_new (TYPE_ARIO_COVER_LOCAL,
                               NULL);

        g_return_val_if_fail (local->priv != NULL, NULL);

        return ARIO_COVER_PROVIDER (local);
}

gboolean
ario_cover_local_get_covers (ArioCoverProvider *cover_provider,
                             const char *artist,
                             const char *album,
                             const char *file,
                             GArray **file_size,
                             GSList **file_contents,
                             ArioCoverProviderOperation operation)
{
        ARIO_LOG_FUNCTION_START
        gchar *musicdir;
        gchar *filename;
        int i;
        gchar *data;
        gsize size;
        gboolean ret = FALSE;
        gboolean ret2;

        if (!file)
                return FALSE;
        musicdir = ario_conf_get_string (PREF_MUSIC_DIR, PREF_MUSIC_DIR_DEFAULT);
        if (musicdir && strlen (musicdir) > 1) {
                for (i = 0; valid_cover_names[i]; i++) {
                        filename = g_build_filename (musicdir, file, valid_cover_names[i], NULL);
                        if (ario_util_uri_exists (filename)) {
                                ret2 = g_file_get_contents (filename,
                                                            &data,
                                                            &size,
                                                            NULL);
                                if (ret2 && ario_cover_size_is_valid (size)) {
                                        /* If the cover is not too big and not too small (blank image), we append it to file_contents */
                                        g_array_append_val (*file_size, size);
                                        *file_contents = g_slist_append (*file_contents, data);
                                        /* If at least one cover is found, we return OK */
                                        ret = TRUE;
                                        if (operation == GET_FIRST_COVER) {
                                                g_free (filename);
                                                break;
                                        }
                                }
                        }
                        g_free (filename);
                }
        }
        g_free (musicdir);

        return ret;
}

