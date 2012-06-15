#ifndef CKD_CONFIGER_H
#define CKD_CONFIGER_H

#include <glib.h>

typedef enum {
        CKD_SLIDE_NULL_ENTER,
        CKD_SLIDE_LEFT_ENTER,
        CKD_SLIDE_RIGHT_ENTER,
        CKD_SLIDE_UP_ENTER,
        CKD_SLIDE_DOWN_ENTER,
        CKD_SLIDE_SCALE_ENTER,
        CKD_SLIDE_FADE_ENTER
} CkdSlideEnteringEffect;

typedef enum {
        CKD_SLIDE_NULL_EXIT,
        CKD_SLIDE_LEFT_EXIT,
        CKD_SLIDE_RIGHT_EXIT,
        CKD_SLIDE_UP_EXIT,
        CKD_SLIDE_DOWN_EXIT,
        CKD_SLIDE_SCALE_EXIT,
        CKD_SLIDE_FADE_EXIT,
} CkdSlideExitEffect;

typedef struct _CkdMetaEntry CkdMetaEntry;
struct _CkdMetaEntry {
        gpointer slide;
        gdouble duration;
        CkdSlideEnteringEffect enter;
        CkdSlideExitEffect exit;
        GString *text;
};

GNode *ckd_slide_script_new (gchar *filename);

GList *ckd_slide_script_out_meta_entry_list (GNode *script, gint n_of_slides);

void ckd_slide_script_free (GNode *scripts);

#endif
