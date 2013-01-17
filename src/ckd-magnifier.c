#include "ckd-magnifier.h"
#include "ckd-meta-slides.h"

static void
_actor_fade (ClutterActor *actor, guint8 b, guint8 e, guint t)
{
        clutter_actor_set_opacity (actor, b);
        clutter_actor_save_easing_state (actor);
        clutter_actor_set_easing_mode (actor, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (actor, t);
        clutter_actor_set_opacity (actor, e);
        clutter_actor_restore_easing_state (actor);  
}

CkdMagnifier *
ckd_magnifier_alloc (CkdView *view, gfloat pos_x, gfloat pos_y)
{
        CkdMagnifier *mag = g_slice_new (CkdMagnifier);
        mag->view = view;

        /* 构造放大镜完整空间 */
        ClutterActor *current_slide;
        gint current_slide_number;
        CkdMetaSlides *meta_slides;
        gfloat old_scale, scale;
        
        g_object_get (mag->view, "meta-slides", &meta_slides, NULL);
        g_object_get (mag->view,
                      "slide", &current_slide,
                      "slide-number", &current_slide_number,
                      "scale", &scale,
                      NULL);

        g_object_get (meta_slides, "scale", &old_scale, NULL);
        mag->workspace = ckd_meta_slides_get_scaled_slide (meta_slides,
                                                           current_slide_number,
                                                           scale + old_scale);
        
        /* 设置 workspace 的长宽比例与 current_slide 相等，修正 mag->sclae 值 */
        gfloat slide_w, slide_h;
        clutter_actor_get_size (current_slide, &slide_w, &slide_h);
        clutter_actor_set_size (mag->workspace, scale * slide_w, scale * slide_h);

        ckd_magnifier_move (mag, pos_x, pos_y);
        
        ClutterActor *stage;
        g_object_get (mag->view, "stage", &stage, NULL);
        clutter_actor_add_child (stage, mag->workspace);
        
        _actor_fade (mag->workspace, 100, 240, 500);
        _actor_fade (current_slide, 255, 220, 500);
        
        return mag;
}

void
ckd_magnifier_free (CkdMagnifier *mag)
{
        clutter_actor_remove_clip (mag->workspace);
        clutter_actor_destroy (mag->workspace);
        g_slice_free (CkdMagnifier, mag);
}

void
ckd_magnifier_move (CkdMagnifier *mag, gfloat x, gfloat y)
{
        ClutterActor *current_slide;
        gfloat scale, offset_x, offset_y;
        g_object_get (mag->view, "slide", &current_slide, "scale", &scale, NULL);
        clutter_actor_get_position (current_slide, &offset_x, &offset_y);

        gfloat w_x, w_y;
        w_x = scale * (x - offset_x);
        w_y = scale * (y - offset_y);
        
        /* 裁剪出矩形放大镜区域 */
        gfloat mag_w = 0.5 * clutter_actor_get_width (current_slide);
        gfloat mag_h = 0.5 * clutter_actor_get_height (current_slide);


        /* 放大镜工作空间复原 */
        if (clutter_actor_has_clip (mag->workspace))
                clutter_actor_remove_clip (mag->workspace);
        clutter_actor_set_position (mag->workspace, 0.0, 0.0);
        
        clutter_actor_set_clip (mag->workspace,
                                w_x - 0.5 * mag_w,
                                w_y - 0.5 * mag_h,
                                mag_w,
                                mag_h);
        clutter_actor_move_by (mag->workspace,
                               x - w_x,
                               y - w_y);
}

static void
_magnifier_close_cb (ClutterAnimation *am, gpointer data)
{
        CkdMagnifier *mag = data;
        ckd_magnifier_free (mag);
}

void
ckd_magnifier_close (CkdMagnifier *mag)
{
        _actor_fade (mag->workspace, 240, 100, 500);

        ClutterActor *current_slide;
        g_object_get (mag->view, "slide", &current_slide, NULL);
        _actor_fade (current_slide, 220, 255, 500);
        
        /* 动画结束后销毁放大镜 */
        ClutterTransition *trans = clutter_actor_get_transition (mag->workspace, "opacity");
        g_signal_connect (trans, "completed", G_CALLBACK (_magnifier_close_cb), mag);
}
