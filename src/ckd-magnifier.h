#ifndef CKD_MAGNIFIER_H
#define CKD_MAGNIFIER_H

#include <glib-object.h>
#include <poppler.h>
#include <clutter/clutter.h>
#include "ckd-view.h"

typedef struct _CkdMagnifier CkdMagnifier;
struct _CkdMagnifier {
        CkdView *view;
        ClutterActor *workspace;
};

CkdMagnifier * ckd_magnifier_alloc (CkdView *view, gfloat pos_x, gfloat pos_y);

void ckd_magnifier_free (CkdMagnifier *mag);

void ckd_magnifier_close (CkdMagnifier *mag);

void ckd_magnifier_move (CkdMagnifier *mag, gfloat x, gfloat y);

#endif
