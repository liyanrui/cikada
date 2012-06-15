#ifndef CKD_PROGRESS_H
#define CKD_PROGRESS_H

#include <clutter/clutter.h>

#define CKD_TYPE_PROGRESS (ckd_progress_get_type ())
#define CKD_PROGRESS(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), CKD_TYPE_PROGRESS, CkdProgress))
#define CKD_IS_PROGRESS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CKD_TYPE_PROGRESS))
#define CKD_PROGRESS_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), CKD_TYPE_PROGRESS, CkdProgressClass))
#define CKD_IS_PROGRESS_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), CKD_TYPE_PROGRESS))
#define CKD_PROGRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CKD_TYPE_PROGRESS, CkdProgressClass))

typedef struct _CkdProgress CkdProgress;
typedef struct _CkdProgressClass CkdProgressClass;

struct _CkdProgress {
        ClutterActor parent_instance;
};

struct _CkdProgressClass {
        ClutterActorClass parent_class;
};

GType ckd_progress_get_type (void);

void ckd_progress_am (ClutterActor *self, gdouble tick);

#endif
