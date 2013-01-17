#ifndef CKD_SCRIPT_H
#define CKD_SCRIPT_H

#include <glib.h>

typedef enum {
        CKD_SLIDE_AM_NULL,
        CKD_SLIDE_AM_LEFT,
        CKD_SLIDE_AM_RIGHT,
        CKD_SLIDE_AM_TOP,
        CKD_SLIDE_AM_BOTTOM,
        CKD_SLIDE_AM_ENLARGEMENT,
        CKD_SLIDE_AM_SHRINK,
        CKD_SLIDE_AM_FADE
} CkdSlideAM;

typedef struct _CkdMetaEntry CkdMetaEntry;
struct _CkdMetaEntry {
        gpointer slide;
        CkdSlideAM am;
        gfloat tick;
        GString *text;
};

GNode *ckd_script_new (gchar *filename, gint n_of_slides);

void ckd_script_free (GNode *scripts);

GList *ckd_script_output_meta_entry_list (GNode *script);

ClutterColor *ckd_script_get_progress_bar_color (GNode *script);
ClutterColor *ckd_script_get_nonius_color (GNode *script);
gfloat ckd_script_get_progress_bar_vsize (GNode *script);
gfloat ckd_script_get_magnifier_ratio (GNode *script);

gboolean ckd_script_equal (GNode *a, GNode *b);

#endif
