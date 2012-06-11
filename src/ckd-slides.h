#ifndef CKD_SLIDES_H
#define CKD_SLIDES_H

#include <clutter/clutter.h>

#define CKD_TYPE_SLIDES (ckd_slides_get_type ())
#define CKD_SLIDES(object) (G_TYPE_CHECK_INSTANCE_CAST((object), CKD_TYPE_SLIDES, CkdSlides))
#define CKD_SLIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), CKD_TYPE_SLIDES, CkdClass))
#define CKD_IS_SLIDES(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), CKD_TYPE_SLIDES))
#define CKD_IS_SLIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CKD_TYPE_SLIDES))
#define CKD_SLIDES_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), CKD_TYPE_SLIDES, CkdSlidesClass))

typedef struct _CkdSlides CkdSlides;
struct _CkdSlides {
        ClutterActor parent_instance;
};

typedef struct _CkdSlidesClass CkdSlidesClass;
struct _CkdSlidesClass {
        ClutterActorClass parent_class;
};

GType ckd_slides_get_type (void);

void ckd_slides_switch_to_next_slide (CkdSlides * self, gint direction);
void ckd_slides_goto (CkdSlides * self, gint index);

#endif
