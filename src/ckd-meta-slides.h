#ifndef CKD_META_SLIDES_H
#define CKD_META_SLIDES_H

#include <glib-object.h>
#include <poppler.h>
#include <clutter/clutter.h>
#include "ckd-slide-script.h"

#define CKD_META_SLIDES_QUALITY_MIN 0.1
#define CKD_META_SLIDES_QUALITY_MAX 2.1
#define CKD_META_SLIDES_QUALITY_DELTA (CKD_META_SLIDES_QUALITY_MAX \
                                       - CKD_META_SLIDES_QUALITY_MIN)


#define CKD_TYPE_META_SLIDES (ckd_meta_slides_get_type ())
#define CKD_META_SLIDES(object) (G_TYPE_CHECK_INSTANCE_CAST((object), CKD_TYPE_META_SLIDES, CkdMetaSlides))
#define CKD_META_SLIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), CKD_TYPE_META_SLIDES, CkdMetaSlidesClass))
#define CKD_IS_META_SLIDES(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), CKD_TYPE_META_SLIDES))
#define CKD_IS_META_SLIDES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CKD_TYPE_META_SLIDES))
#define CKD_META_SLIDES_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), CKD_TYPE_META_SLIDES, CkdMetaSlidesClass))


typedef struct _CkdMetaSlides CkdMetaSlides;
typedef struct _CkdMetaSlidesClass CkdMetaSlidesClass;

struct _CkdMetaSlides {
        GObject parent;
};

struct _CkdMetaSlidesClass {
        GObjectClass parent_class;
};


typedef enum _CkdMetaSlidesCacheMode CkdMetaSlidesCacheMode;
enum _CkdMetaSlidesCacheMode
{
        CKD_META_SLIDES_MEM_CACHE,
        CKD_META_SLIDES_DISK_CACHE
};

GType ckd_meta_slides_get_type (void);

void ckd_meta_slides_create_cache (CkdMetaSlides *self);

ClutterActor *ckd_meta_slides_get_slide (CkdMetaSlides *self, gint i);

CkdMetaEntry *ckd_meta_slides_get_meta_entry (CkdMetaSlides *self, gint i);

#endif
