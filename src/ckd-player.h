#ifndef CKD_PLAYER_H
#define CKD_PLAYER_H

#include <glib-object.h>
#include <poppler.h>
#include <clutter/clutter.h>

#define CKD_TYPE_PLAYER (ckd_player_get_type ())
#define CKD_PLAYER(o) (G_TYPE_CHECK_INSTANCE_CAST((o), CKD_TYPE_PLAYER, CkdPlayer))
#define CKD_PLAYER_CLASS(c) (G_TYPE_CHECK_CLASS_CAST((c), CKD_TYPE_PLAYER, CkdPlayerClass))
#define CKD_IS_PLAYER(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), CKD_TYPE_PLAYER))
#define CKD_IS_PLAYER_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE((c), CKD_TYPE_PLAYER))
#define CKD_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), CKD_TYPE_PLAYER, CkdPlayerClass))

typedef struct _CkdPlayer CkdPlayer;
typedef struct _CkdPlayerClass CkdPlayerClass;

struct _CkdPlayer {
        GObject parent;
};

struct _CkdPlayerClass {
        GObjectClass parent_class;
};

GType ckd_player_get_type (void);

void ckd_player_step (CkdPlayer *self, gint step);

#endif
