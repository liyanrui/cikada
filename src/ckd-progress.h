#ifndef CKD_PROGRESS_H
#define CKD_PROGRESS_H

#include <glib.h>
#include <clutter/clutter.h>

#define CKD_TYPE_PAGE (ckd_progress_get_type ())
#define CKD_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CKD_TYPE_PAGE, CkdProgress))
#define CKD_IS_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CKD_TYPE_PAGE))
#define CKD_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CKD_TYPE_PAGE, CkdProgressClass))
#define CKD_IS_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CKD_TYPE_PAGE))
#define CKD_PROGRESS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CKD_TYPE_PAGE, CkdProgressClass))

typedef struct _CkdProgress CkdProgress;
typedef struct _CkdProgressClass CkdProgressClass;

struct _CkdProgress {
        ClutterActor parent_instance;
};

struct _CkdProgressClass {
        ClutterActorClass parent_class;
};

GType ckd_progress_get_type (void);

/* Public functons */
ClutterActor *ckd_progress_new (void);

#endif
