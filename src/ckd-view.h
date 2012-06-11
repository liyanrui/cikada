#ifndef CKD_VIEW_H
#define CKD_VIEW_H

#include <clutter/clutter.h>

#define CKD_TYPE_VIEW (ckd_view_get_type ())
#define CKD_VIEW(o) (G_TYPE_CHECK_INSTANCE_CAST((o), CKD_TYPE_VIEW, CkdView))
#define CKD_VIEW_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), CKD_TYPE_VIEW, CkdViewClass))
#define CKD_IS_VIEW(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), CKD_TYPE_VIEW))
#define CKD_IS_VIEW_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), CKD_TYPE_VIEW))
#define CKD_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), CKD_TYPE_VIEW, CkdViewClass))

typedef struct _CkdView CkdView;
struct _CkdView {
        ClutterActor parent;
};

typedef struct _CkdViewClass CkdViewClass;
struct _CkdViewClass {
        ClutterActorClass parent_class;
};

typedef enum {
        CKD_SLIDE_SCALE_ENTERING,
        CKD_SLIDE_FADE_ENTERING
} CkdViewSlideInEffect;

typedef enum {
        CKD_SLIDE_LEFT_EXIT,
        CKD_SLIDE_RIGHT_EXIT,
        CKD_SLIDE_UP_EXIT,
        CKD_SLIDE_DOWN_EXIT,
        CKD_SLIDE_FADE_EXIT,
} CkdViewSlideOutEffect;

GType ckd_view_get_type (void);

void ckd_view_transit_slide (CkdView *self, ClutterActor *next_slide);

#endif
