#ifndef CKD_VIEW_H
#define CKD_VIEW_H

#include <clutter/clutter.h>
#include "ckd-script.h"

#define CKD_TYPE_VIEW (ckd_view_get_type ())
#define CKD_VIEW(o) (G_TYPE_CHECK_INSTANCE_CAST((o), CKD_TYPE_VIEW, CkdView))
#define CKD_VIEW_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), CKD_TYPE_VIEW, CkdViewClass))
#define CKD_IS_VIEW(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), CKD_TYPE_VIEW))
#define CKD_IS_VIEW_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), CKD_TYPE_VIEW))
#define CKD_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), CKD_TYPE_VIEW, CkdViewClass))

typedef struct _CkdView CkdView;
struct _CkdView {
        GObject parent;
};

typedef struct _CkdViewClass CkdViewClass;
struct _CkdViewClass {
        GObjectClass parent_class;
};

GType ckd_view_get_type (void);

ClutterActor *ckd_view_load_ith_slide (CkdView *self, gint i);

void ckd_view_get_nonius_position (CkdView *self, gfloat *x, gfloat *y);

#endif
