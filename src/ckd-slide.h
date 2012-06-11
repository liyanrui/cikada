#ifndef CKD_SLIDES_H
#define CKD_SLIDES_H
#include <clutter/clutter.h>
#include <poppler.h>

#define CKD_TYPE_SLIDE (ckd_slide_get_type ())
#define CKD_SLIDE(o) (G_TYPE_CHECK_INSTANCE_CAST((o), CKD_TYPE_SLIDE, CkdSlide))
#define CKD_SLIDE_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), CKD_TYPE_SLIDE, CkdSlideClass))
#define CKD_IS_SLIDE(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), CKD_TYPE_SLIDE))
#define CKD_IS_SLIDE_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), CKD_TYPE_SLIDE))
#define CKD_SLIDE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), CKD_TYPE_SLIDE, CkdSlideClass))

typedef struct _CkdSlide CkdSlide;
struct _CkdSlide {
        ClutterActor parent;
};

typedef struct _CkdSlideClass CkdSlideClass;
struct _CkdSlideClass {
        ClutterActorClass parent;
};

typedef enum _CkdSlideType CkdSlideType;
enum _CkdSlideType {
        CKD_SLIDE_FROM_IMAGE_PATH,
        CKD_SLIDE_FROM_POPPLER_PAGE
};

GType ckd_slides_get_type (void);

ClutterActor *ckd_slide_new_for_image (GFile *file);
ClutterActor *ckd_slide_new_for_poppler_page (PopplerPage *page, gdouble scale);

#endif
