#ifndef CKD_SLIDES_H
#define CKD_SLIDES_H

#include <glib.h>
#include "ckd-page.h"

typedef struct _CkdSlides CkdSlides;

CkdSlides *ckd_slides_new_with_segment_length (
        ClutterActor *slide_box,
        const gchar * path,
        gint segment_length
        );
void ckd_slides_switch_to_next_page (CkdSlides * slides, gint direction);
        
#endif
