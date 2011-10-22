#ifndef CKD_RING_H
#define CKD_RING_H

#include <glib.h>
#include <clutter/clutter.h>

#define CKD_TYPE_RING (ckd_ring_get_type ())
#define CKD_RING(object) (G_TYPE_CHECK_INSTANCE_CAST((object), CKD_TYPE_RING, CkdRing))
#define CKD_RING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), CKD_TYPE_RING, CkdClass))
#define CKD_IS_RING(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), CKD_TYPE_RING))
#define CKD_IS_RING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), CKD_TYPE_RING))
#define CKD_RING_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), CKD_TYPE_RING, CkdRingClass))

typedef struct _CkdRing CkdRing;
typedef struct _CkdRingClass CkdRingClass;

struct _CkdRing {
        ClutterActor parent_instance;
};

struct _CkdRingClass {
        ClutterActorClass parent_class;
};

GType ckd_ring_get_type (void);

#endif
