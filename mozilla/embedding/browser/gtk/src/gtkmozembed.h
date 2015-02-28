/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Christopher Blizzard.
 * Portions created by Christopher Blizzard are Copyright (C)
 * Christopher Blizzard.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
 *   Ramiro Estrugo <ramiro@eazel.com>
 */

#ifndef gtkmozembed_h
#define gtkmozembed_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <gtk/gtk.h>

#define GTK_TYPE_MOZ_EMBED             (gtk_moz_embed_get_type())
#define GTK_MOZ_EMBED(obj)             GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_EMBED, GtkMozEmbed)
#define GTK_MOZ_EMBED_CLASS(klass)     GTK_CHECK_CLASS_CAST((klass), GTK_TYPE_MOZ_EMBED, GtkMozEmbedClass)
#define GTK_IS_MOZ_EMBED(obj)          GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_EMBED)
#define GTK_IS_MOZ_EMBED_CLASS(klass)  GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_EMBED)

typedef struct _GtkMozEmbed      GtkMozEmbed;
typedef struct _GtkMozEmbedClass GtkMozEmbedClass;

struct _GtkMozEmbed
{
  GtkBin    bin;
  void     *data;
};

struct _GtkMozEmbedClass
{
  GtkBinClass parent_class;

  void (* link_message)        (GtkMozEmbed *embed);
  void (* js_status)           (GtkMozEmbed *embed);
  void (* location)            (GtkMozEmbed *embed);
  void (* title)               (GtkMozEmbed *embed);
  void (* progress)            (GtkMozEmbed *embed, gint curprogress,
				gint maxprogress);
  void (* progress_all)        (GtkMozEmbed *embed, const char *aURI,
				gint curprogress, gint maxprogress);
  void (* net_state)           (GtkMozEmbed *embed, gint state, guint status);
  void (* net_state_all)       (GtkMozEmbed *embed, const char *aURI,
				gint state, guint status);
  void (* net_start)           (GtkMozEmbed *embed);
  void (* net_stop)            (GtkMozEmbed *embed);
  void (* new_window)          (GtkMozEmbed *embed, GtkMozEmbed **newEmbed,
				guint chromemask);
  void (* visibility)          (GtkMozEmbed *embed, gboolean visibility);
  void (* destroy_brsr)        (GtkMozEmbed *embed);
  gint (* open_uri)            (GtkMozEmbed *embed, const char *aURI);
  void (* size_to)             (GtkMozEmbed *embed, gint width, gint height);
  gint (* dom_key_down)        (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_key_press)       (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_key_up)          (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_down)      (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_up)        (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_click)     (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_dbl_click) (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_over)      (GtkMozEmbed *embed, gpointer dom_event);
  gint (* dom_mouse_out)       (GtkMozEmbed *embed, gpointer dom_event);
  void (* security_change)     (GtkMozEmbed *embed, gpointer request,
				guint state);
  void (* status_change)       (GtkMozEmbed *embed, gpointer request,
				gint status, gpointer message);
};

GtkType      gtk_moz_embed_get_type         (void);
GtkWidget   *gtk_moz_embed_new              (void);
void         gtk_moz_embed_push_startup     (void);
void         gtk_moz_embed_pop_startup      (void);
void         gtk_moz_embed_set_comp_path    (char *aPath);
void         gtk_moz_embed_set_profile_path (char *aDir, char *aName);
void         gtk_moz_embed_load_url         (GtkMozEmbed *embed, 
					     const char *url);
void         gtk_moz_embed_stop_load        (GtkMozEmbed *embed);
gboolean     gtk_moz_embed_can_go_back      (GtkMozEmbed *embed);
gboolean     gtk_moz_embed_can_go_forward   (GtkMozEmbed *embed);
void         gtk_moz_embed_go_back          (GtkMozEmbed *embed);
void         gtk_moz_embed_go_forward       (GtkMozEmbed *embed);
void         gtk_moz_embed_render_data      (GtkMozEmbed *embed, 
					     const char *data,
					     guint32 len,
					     const char *base_uri, 
					     const char *mime_type);
void         gtk_moz_embed_open_stream      (GtkMozEmbed *embed,
					     const char *base_uri,
					     const char *mime_type);
void         gtk_moz_embed_append_data      (GtkMozEmbed *embed,
					     const char *data, guint32 len);
void         gtk_moz_embed_close_stream     (GtkMozEmbed *embed);
char        *gtk_moz_embed_get_link_message (GtkMozEmbed *embed);
char        *gtk_moz_embed_get_js_status    (GtkMozEmbed *embed);
char        *gtk_moz_embed_get_title        (GtkMozEmbed *embed);
char        *gtk_moz_embed_get_location     (GtkMozEmbed *embed);
void         gtk_moz_embed_reload           (GtkMozEmbed *embed, gint32 flags);
void         gtk_moz_embed_set_chrome_mask  (GtkMozEmbed *embed, 
					     guint32 flags);
guint32      gtk_moz_embed_get_chrome_mask  (GtkMozEmbed *embed);

/* enum types */
#define GTK_TYPE_MOZ_EMBED_PROGRESS_FLAGS \
             (gtk_moz_embed_progress_flags_get_type())
#define GTK_TYPE_MOZ_EMBED_STATUS_ENUMS \
             (gtk_moz_embed_status_enums_get_type())
#define GTK_TYPE_MOZ_EMBED_RELOAD_FLAGS \
             (gtk_moz_embed_reload_flags_get_type())
#define GTK_TYPE_MOZ_EMBED_CHROME_FLAGS \
             (gtk_moz_embed_chrome_flags_get_type())

GtkType      gtk_moz_embed_progress_flags_get_type (void);
GtkType      gtk_moz_embed_status_enums_get_type (void);
GtkType      gtk_moz_embed_reload_flags_get_type (void);
GtkType      gtk_moz_embed_chrome_flags_get_type (void);

/* These are straight out of nsIWebProgressListener.h */

typedef enum
{
  GTK_MOZ_EMBED_FLAG_START = 1,
  GTK_MOZ_EMBED_FLAG_REDIRECTING = 2,
  GTK_MOZ_EMBED_FLAG_TRANSFERRING = 4,
  GTK_MOZ_EMBED_FLAG_NEGOTIATING = 8,
  GTK_MOZ_EMBED_FLAG_STOP = 16,
  
  GTK_MOZ_EMBED_FLAG_IS_REQUEST = 65536,
  GTK_MOZ_EMBED_FLAG_IS_DOCUMENT = 131072,
  GTK_MOZ_EMBED_FLAG_IS_NETWORK = 262144,
  GTK_MOZ_EMBED_FLAG_IS_WINDOW = 524288 
} GtkMozEmbedProgressFlags;

/* These are from various networking headers */

typedef enum
{
  /* NS_ERROR_UNKNOWN_HOST */
  GTK_MOZ_EMBED_STATUS_FAILED_DNS     = 2152398878U,
 /* NS_ERROR_CONNECTION_REFUSED */
  GTK_MOZ_EMBED_STATUS_FAILED_CONNECT = 2152398861U,
 /* NS_ERROR_NET_TIMEOUT */
  GTK_MOZ_EMBED_STATUS_FAILED_TIMEOUT = 2152398862U,
 /* NS_BINDING_ABORTED */
  GTK_MOZ_EMBED_STATUS_FAILED_USERCANCELED = 2152398850U
} GtkMozEmbedStatusFlags;

/* These used to be straight out of nsIWebNavigation.h until the API
   changed.  Now there's a mapping table that maps these values to the
   internal values. */

typedef enum 
{
  GTK_MOZ_EMBED_FLAG_RELOADNORMAL = 0,
  GTK_MOZ_EMBED_FLAG_RELOADBYPASSCACHE = 1,
  GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXY = 2,
  GTK_MOZ_EMBED_FLAG_RELOADBYPASSPROXYANDCACHE = 3,
  GTK_MOZ_EMBED_FLAG_RELOADCHARSETCHANGE = 4
} GtkMozEmbedReloadFlags;

/* These are straight out of nsIWebBrowserChrome.h */

typedef enum
{
  GTK_MOZ_EMBED_FLAG_DEFAULTCHROME = 1U,
  GTK_MOZ_EMBED_FLAG_WINDOWBORDERSON = 2U,
  GTK_MOZ_EMBED_FLAG_WINDOWCLOSEON = 4U,
  GTK_MOZ_EMBED_FLAG_WINDOWRESIZEON = 8U,
  GTK_MOZ_EMBED_FLAG_MENUBARON = 16U,
  GTK_MOZ_EMBED_FLAG_TOOLBARON = 32U,
  GTK_MOZ_EMBED_FLAG_LOCATIONBARON = 64U,
  GTK_MOZ_EMBED_FLAG_STATUSBARON = 128U,
  GTK_MOZ_EMBED_FLAG_PERSONALTOOLBARON = 256U,
  GTK_MOZ_EMBED_FLAG_SCROLLBARSON = 512U,
  GTK_MOZ_EMBED_FLAG_TITLEBARON = 1024U,
  GTK_MOZ_EMBED_FLAG_EXTRACHROMEON = 2048U,
  GTK_MOZ_EMBED_FLAG_ALLCHROME = 4094U,
  GTK_MOZ_EMBED_FLAG_WINDOWRAISED = 33554432U,
  GTK_MOZ_EMBED_FLAG_WINDOWLOWERED = 67108864U,
  GTK_MOZ_EMBED_FLAG_CENTERSCREEN = 134217728U,
  GTK_MOZ_EMBED_FLAG_DEPENDENT = 268435456U,
  GTK_MOZ_EMBED_FLAG_MODAL = 536870912U,
  GTK_MOZ_EMBED_FLAG_OPENASDIALOG = 1073741824U,
  GTK_MOZ_EMBED_FLAG_OPENASCHROME = 2147483648U 
} GtkMozEmbedChromeFlags;

/* this is a singleton object that you can hook up to to get signals
   that are not handed out on a per widget basis. */

typedef struct _GtkMozEmbedSingle      GtkMozEmbedSingle;
typedef struct _GtkMozEmbedSingleClass GtkMozEmbedSingleClass;

struct _GtkMozEmbedSingle
{
  GtkObject  object;
  void      *data;
};

struct _GtkMozEmbedSingleClass
{
  GtkObjectClass parent_class;

  void (* new_window_orphan)   (GtkMozEmbedSingle *embed,
				GtkMozEmbed **newEmbed,
				guint chromemask);
};

GtkMozEmbedSingle *
gtk_moz_embed_single_get(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* gtkmozembed_h */
