#include <math.h>
#include "ckd-slides.h"

G_DEFINE_TYPE (CkdSlides, ckd_slides, CKD_TYPE_PAGE_MANAGER);

#define CKD_SLIDES_GET_PRIVATE(obj) (\
        G_TYPE_INSTANCE_GET_PRIVATE ((obj), CKD_TYPE_SLIDES, CkdSlidesPriv))

#define CKD_SLIDES_AM_TIME_BASE 1000

typedef struct _CkdSlidesPriv CkdSlidesPriv;
struct  _CkdSlidesPriv {
        PopplerDocument *doc;
        ClutterActor *layout_space;
        ClutterActor *time_bar;
        ClutterActor *current_slide;
        CkdScript *script;
};

enum {
        PROP_SLIDES_0,
        PROP_SLIDES_BOX,
        PROP_SLIDES_PROGRESS_BAR,
        PROP_SLIDES_CURRENT_SLIDE,
        PROP_SLIDES_NEXT_SLIDE,
        PROP_SLIDES_IS_OVERVIEW,
        N_SLIDES_PROPS
};

