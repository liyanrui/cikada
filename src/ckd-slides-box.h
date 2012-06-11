#ifndef CKD_SLIDES_BOX_H
#define CKD_SLIDES_BOX_H

#include <clutter/clutter.h>

#define CKD_TYPE_SLIDES_BOX (ckd_slides_box_get_type ())
#define CKD_SLIDES_BOX(o) (G_TYPE_CHECK_INSTANCE_CAST((o), CKD_TYPE_SLIDES_BOX, CkdSlidesBox))
#define CKD_SLIDES_BOX_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), CKD_TYPE_SLIDES_BOX, CkdSlidesBoxClass))
#define CKD_IS_SLIDES_BOX(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), CKD_TYPE_SLIDES_BOX))
#define CKD_IS_SLIDES_BOX_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), CKD_TYPE_SLIDES_BOX))
#define CKD_SLIDES_BOX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), CKD_TYPE_SLIDES_BOX, CkdSlidesBoxClass))

typedef struct _CkdSlidesBox CkdSlidesBox;
struct _CkdSlidesBox {
        ClutterActor parent;
};

typedef struct _CkdSlidesBoxClass CkdSlidesBoxClass;
struct _CkdSlidesBoxClass {
        ClutterActorClass parent_class;
};

typedef enum {
        CKD_SLIDE_LEFT_OUT,
        CKD_SLIDE_SCALE_IN
} CkdSlideStyle;

GType ckd_slides_box_get_type (void);

void ckd_slides_box_transit_slide (CkdSlidesBox *self);


#endif
