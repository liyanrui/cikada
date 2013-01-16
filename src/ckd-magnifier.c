#include "ckd-magnifier.h"
#include "ckd-meta-slides.h"

CkdMagnifier *
ckd_magnifier_alloc (CkdView *view, gfloat pos_x, gfloat pos_y)
{
        CkdMagnifier *mag = g_slice_new (CkdMagnifier);
        mag->view = view;
        mag->scale = 2.0;

        /* 构造放大镜完整空间 */
        ClutterActor *current_slide;
        gint current_slide_number;
        CkdMetaSlides *meta_slides;
        gdouble old_scale;
        
        g_object_get (mag->view, "meta-slides", &meta_slides, NULL);
        g_object_get (mag->view,
                      "slide", &current_slide,
                      "slide-number", &current_slide_number,
                      NULL);
        g_object_get (meta_slides, "scale", &old_scale, NULL);
        mag->workspace = ckd_meta_slides_get_scaled_slide (meta_slides,
                                                           current_slide_number,
                                                           mag->scale + old_scale);
        /* 设置 workspace 的长宽比例与 current_slide 相等，修正 mag->sclae 值 */
        gfloat slide_w, slide_h;
        clutter_actor_get_size (current_slide, &slide_w, &slide_h);
        clutter_actor_set_size (mag->workspace, mag->scale * slide_w, mag->scale * slide_h);

        ckd_magnifier_move (mag, pos_x, pos_y);
        
        ClutterActor *stage;
        g_object_get (mag->view, "stage", &stage, NULL);
        clutter_actor_add_child (stage, mag->workspace);
        
        clutter_actor_set_opacity (mag->workspace, 0);
        clutter_actor_save_easing_state (mag->workspace);
        clutter_actor_set_easing_mode (mag->workspace, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (mag->workspace, 500);
        clutter_actor_set_opacity (mag->workspace, 240);
        clutter_actor_restore_easing_state (mag->workspace);

        clutter_actor_set_opacity (current_slide, 255);
        clutter_actor_save_easing_state (current_slide);
        clutter_actor_set_easing_mode (current_slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (current_slide, 500);
        clutter_actor_set_opacity (current_slide, 220);
        clutter_actor_restore_easing_state (current_slide);        
        
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
        mag->pos_x = x;
        mag->pos_y = y;

        ClutterActor *current_slide;
        gfloat offset_x, offset_y;
        g_object_get (mag->view, "slide", &current_slide, NULL);
        clutter_actor_get_position (current_slide, &offset_x, &offset_y);

        gfloat w_x, w_y;
        w_x = mag->scale * (mag->pos_x - offset_x);
        w_y = mag->scale * (mag->pos_y - offset_y);
        
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
                               mag->pos_x - w_x,
                               mag->pos_y - w_y);
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
        clutter_actor_set_opacity (mag->workspace, 240);
        clutter_actor_save_easing_state (mag->workspace);
        clutter_actor_set_easing_mode (mag->workspace, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (mag->workspace, 500);
        clutter_actor_set_opacity (mag->workspace, 0);
        clutter_actor_restore_easing_state (mag->workspace);

        ClutterActor *current_slide;
        g_object_get (mag->view, "slide", &current_slide, NULL);

        clutter_actor_set_opacity (current_slide, 220);
        clutter_actor_save_easing_state (current_slide);
        clutter_actor_set_easing_mode (current_slide, CLUTTER_LINEAR);
        clutter_actor_set_easing_duration (current_slide, 500);
        clutter_actor_set_opacity (current_slide, 255);
        clutter_actor_restore_easing_state (current_slide);
        
        /* 动画结束后销毁放大镜 */
        ClutterTransition *trans = clutter_actor_get_transition (mag->workspace, "opacity");
        g_signal_connect (trans, "completed", G_CALLBACK (_magnifier_close_cb), mag);
}
