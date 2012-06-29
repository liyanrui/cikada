#include <stdio.h>
#include <string.h>
#include <math.h>
#include <clutter/clutter.h>
#include "ckd-script.h"

typedef enum {
        CKD_REPORT_SETUP,
        CKD_CONTINUATION,
        CKD_SLIDE,
        N_CKD_DOMAINS
} CkdScriptDomain;

typedef struct _CkdScriptNode CkdScriptNode;
struct _CkdScriptNode {
        gpointer data;
        CkdScriptDomain domain;
};

typedef struct _CkdScriptReportSetup CkdScriptReportSetup;
struct _CkdScriptReportSetup {
        CkdSlideAM default_am;
        ClutterColor *progress_bar_color;
        ClutterColor *nonius_color;
        gfloat progress_bar_vsize;

        /* 输出元幻灯片表时用于确定列表长度 */
        gint n_of_slides;
};

typedef struct _CkdScriptContinuation CkdScriptContinuation;
struct _CkdScriptContinuation {
        gint head;
        gint tail;
};

typedef struct _CkdScriptSlide CkdScriptSlide;
struct _CkdScriptSlide {
        gint id;
        CkdSlideAM am;
        GString *text;
};

typedef struct _CkdCommand CkdCommand;
struct _CkdCommand {
        GString *text;
        CkdScriptDomain domain;
};

static GString *
_get_script_text (GIOChannel *channel)
{
        gchar *line = NULL;
        gchar *no_whitespace_line = NULL;

        GIOStatus status;
        gsize length;
        GString *content = g_string_new (NULL);

        do {
                status = g_io_channel_read_line (channel,
                                                 &line,
                                                 &length,
                                                 NULL,
                                                 NULL);
                if (length == 0)
                        continue;

                no_whitespace_line = g_strstrip (line);
                if (g_utf8_strlen (no_whitespace_line, -1) != 0) {
                        g_string_append (content, no_whitespace_line);
                }

                g_free (line);

        } while (status == G_IO_STATUS_NORMAL);

        if (!g_utf8_validate (content->str, -1, NULL))
                g_error ("Cikada script should be utf-8 coded");

        return content;
}

static gboolean
_is_escape_mark (gchar *script_text, gchar *ch)
{
        gchar *next_char = ch + 1;
        gboolean result;

        switch (*next_char) {
        case '\\':
                result = TRUE;
                break;
        case '{':
                result = TRUE;
                break;
        case '}':
                result = TRUE;
                break;
        default:
                result = FALSE;
        }

        return result;
}

static gboolean
_is_escape_char (gchar *script_text, gchar *ch)
{
        gchar *prev_char = g_utf8_prev_char (ch);
        gboolean result;

        switch (*prev_char) {
        case '\\':
                result = TRUE;
                break;
        default:
                result = FALSE;
        }

        return result;
}

static GList *
_create_cmd_list (gchar *text)
{
        gchar *ch = NULL;
        GList *cmd_list = NULL;
        CkdCommand *cmd = NULL;

        enum {
                SCRIPT_INIT,
                SCRIPT_COMMAND_BEGIN,
                SCRIPT_OPTION_BEGIN,
                SCRIPT_TEXT,
                SCRIPT_OPTION_END,
                SCRIPT_COMMAND_END
        } state = SCRIPT_INIT;

        glong offset = 0;

        for (ch = text; *ch != '\0'; ch++, offset++) {
                switch (state) {
                case SCRIPT_INIT:
                        if (*ch == '\\' && !_is_escape_mark (text, ch)) {
                                state = SCRIPT_COMMAND_BEGIN;
                        }
                        break;
                case SCRIPT_COMMAND_BEGIN:
                        if (*ch == '['  && !_is_escape_char (text, ch)) {
                                state = SCRIPT_OPTION_BEGIN;
                        } else if (*ch == '{' && !_is_escape_char (text, ch)) {
                                state = SCRIPT_TEXT;
                        }
                        break;
                case SCRIPT_OPTION_BEGIN:
                        if (*ch == ']'  && !_is_escape_char (text, ch)) {
                                gchar *next = ch + 1;
                                if (*next == '\\' && !_is_escape_mark (text, next) || *next == '\0')
                                        state = SCRIPT_COMMAND_END;
                                else
                                        state = SCRIPT_OPTION_END;
                        }
                        break;
                case SCRIPT_OPTION_END:
                        if (*ch == '['  && !_is_escape_char (text, ch)) {
                                state = SCRIPT_OPTION_BEGIN;
                        } else if (*ch == '{' && !_is_escape_char (text, ch)) {
                                state = SCRIPT_TEXT;
                        }
                        break;
                case SCRIPT_TEXT:
                        if (*ch == '}' && !_is_escape_char (text, ch)) {
                                state = SCRIPT_COMMAND_END;
                        }
                        break;
                case SCRIPT_COMMAND_END:
                        if (*ch == '\\' && !_is_escape_mark (text, ch)) {
                                state = SCRIPT_COMMAND_BEGIN;
                        }
                        break;
                default:
                        break;
                }

                if (cmd == NULL && state == SCRIPT_COMMAND_BEGIN) {
                        cmd = g_slice_alloc (sizeof(CkdCommand));
                        cmd->text = g_string_new (NULL);
                }

                if (cmd) {
                        g_string_append_c (cmd->text, *ch);
                }

                if (state == SCRIPT_COMMAND_END) {
                        cmd_list = g_list_append (cmd_list, cmd);
                        cmd = NULL;
                }
        }

        return cmd_list;
}

static void
validate_cmd (gpointer data, gpointer user_data)
{
        CkdCommand *cmd = data;
        GRegex **regexes = user_data;
        gboolean result = FALSE;

        for (gint i = 0; i < N_CKD_DOMAINS; i++) {
                result = g_regex_match (regexes[i], cmd->text->str, 0, NULL);
                if (result) {
                        cmd->domain = i;
                        break;
                }
        }
        if (!result) {
                g_error ("I can not recognize `%s'", cmd->text->str);
        }
}

static void
ckd_script_validate_cmd_list (GList *cmd_list)
{
        /* 元素的次序是按照 CkdScriptDomain 排列的 */
        GRegex *regexes[N_CKD_DOMAINS] = {
                g_regex_new ("\\\\setupreport\\s*"
                             "\\[(.+\\s*=\\s*.+\\s*,\\s*)*"
                             ".+\\s*=\\s*.+\\s*\\]",
                             0, 0, NULL),
                g_regex_new ("\\\\continuation\\s*\\[\\s*\\d+\\s*\\-\\s*\\d+\\s*\\]", 0, 0, NULL),
                g_regex_new ("\\\\slide\\s*(\\[.+\\])+\\s*(\\{.*\\})*", 0, 0, NULL)
        };

        g_list_foreach (cmd_list, validate_cmd, regexes);

        for (gint i = 0; i < N_CKD_DOMAINS; i++) {
                g_regex_unref (regexes[i]);
        }
}

static CkdSlideAM
get_slide_am (gchar *text)
{
        CkdSlideAM am = CKD_SLIDE_AM_NULL;
        
        if (text) {
                if (g_str_equal (text, "fade"))
                        am = CKD_SLIDE_AM_FADE;
                else if (g_str_equal (text, "curl"))
                        am = CKD_SLIDE_AM_CURL;
                else if (g_str_equal (text, "enlargement"))
                        am = CKD_SLIDE_AM_ENLARGEMENT;
                else if (g_str_equal (text, "shrink"))
                        am = CKD_SLIDE_AM_SHRINK;
                else if (g_str_equal (text, "bottom"))
                        am = CKD_SLIDE_AM_BOTTOM;
                else if (g_str_equal (text, "top"))
                        am = CKD_SLIDE_AM_TOP;
                else if (g_str_equal (text, "right"))
                        am = CKD_SLIDE_AM_RIGHT;
                else if (g_str_equal (text, "left"))
                        am = CKD_SLIDE_AM_LEFT;
        }
        
        return am;
}

static ClutterColor *
get_color (gchar *text)
{
        ClutterColor *color = clutter_color_new (0, 0, 0, 0);
        
        gchar **splitted_text = g_strsplit (text, "=", 0);
        size_t len = strlen (splitted_text[1]);
        splitted_text[1][0] = '(';
        splitted_text[1][len - 1] = ')';
        
        gint color_text_len = len + 5;
        gchar *color_text = g_slice_alloc ((color_text_len) * sizeof (char));
        sprintf (color_text, "rgba%s", splitted_text[1]);
        
        clutter_color_from_string (color, color_text);
        
        g_slice_free1 (color_text_len, color_text);
        g_strfreev (splitted_text);
        
        return color;
}

static CkdScriptReportSetup *
parse_report_setup (CkdScriptReportSetup *rs, gchar *text)
{
        GMatchInfo *info;
                
        /* \begin 解析幻灯片默认动画 */
        GRegex *am_re = g_regex_new ("style\\s*=\\s*[a-z\\-\\s]+", 0, 0, NULL);
        gchar *am_text = NULL;
        g_regex_match (am_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                gchar **splitted_text = g_strsplit (s, "=", 0);
                rs->default_am = get_slide_am (g_strstrip (splitted_text[1]));
                g_free (am_text);
                g_strfreev (splitted_text);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (am_re);
        /* \end */

        /* \begin 解析进度条颜色 */
        GRegex *progress_bar_color_re = g_regex_new ("progress-bar-color"
                                                     "\\s*=\\s*"
                                                     "\\{\\s*\\d+\\s*,"
                                                     "\\s*\\d+\\s*,"
                                                     "\\s*\\d+\\s*,"
                                                     "\\s*\\d+\\s*\\}",
                                                     0, 0, NULL);
        g_regex_match (progress_bar_color_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                rs->progress_bar_color = get_color (s);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (am_re);
        /* \end */

        /* \begin 解析滑块颜色 */
        GRegex *nonius_color_re = g_regex_new ("nonius-color"
                                               "\\s*=\\s*"
                                               "\\{\\s*\\d+\\s*,"
                                               "\\s*\\d+\\s*,"
                                               "\\s*\\d+\\s*,"
                                               "\\s*\\d+\\s*\\}",
                                               0, 0, NULL);
        g_regex_match (nonius_color_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                rs->nonius_color = get_color (s);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (am_re);
        /* \end */

        /* \begin 解析进度条竖向尺寸 */
        GRegex *progress_bar_vsize_re = g_regex_new ("progress-bar-vsize"
                                                     "\\s*=\\s*"
                                                     "\\d+",
                                                     0, 0, NULL);
        g_regex_match (progress_bar_vsize_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                gchar **splitted_text = g_strsplit (s, "=", 0);
                rs->progress_bar_vsize = g_ascii_strtod (splitted_text[1], NULL);
                g_strfreev (splitted_text);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (am_re);
        /* \end */
}

static void
parse_continuation (CkdScriptContinuation *cont, gchar *text)
{
        GMatchInfo *info;
        
        /* \begin 解析幻灯片编号范围 */
        GRegex *id_re = g_regex_new ("\\[\\s*\\d+\\s*\\-\\s*\\d+\\s*\\]", 0, 0, NULL);
        g_regex_match (id_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                glong l = g_utf8_strlen (s, -1);
                gchar *am_text = g_utf8_substring (s, 1, l - 1);

                gchar **splitted_text = g_strsplit (am_text, "-", 0);

                cont->head = g_ascii_strtoll (splitted_text[0], NULL, 10) - 1;
                cont->tail = g_ascii_strtoll (splitted_text[1], NULL, 10) - 1;

                g_strfreev (splitted_text);
                g_free (am_text);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (id_re);
        /* \end */
}

static void
parse_slide (CkdScriptSlide *slide, gchar *text)
{
        GMatchInfo *info;

        /* \begin 解析 id */
        GRegex *id_re = g_regex_new ("\\[\\d+\\]", 0, 0, NULL);
        g_regex_match (id_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                slide->id = g_ascii_strtod (s + 1, NULL);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (id_re);
        /* \end */

        /* \begin 解析动画类型 */
        gchar *am_text = NULL;
        GRegex *am_re = g_regex_new ("\\[\\s*[a-z\\-\\s]+\\]", 0, 0, NULL);
        g_regex_match (am_re, text,  0, &info);
        
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                glong l = g_utf8_strlen (s, -1);
                am_text = g_utf8_substring (s, 1, l - 1);
                slide->am = get_slide_am (g_strstrip (am_text));
                g_free (am_text);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (am_re);
        /* \end */

        /* 现在还不具备解析 slide 文本的功能 */
        slide->text = NULL;
}

static void
parse_cmd (gpointer data, gpointer user_data)
{
        CkdCommand *cmd = data;
        GNode *script = user_data;
        GNode *node = NULL;
        CkdScriptNode *script_node = NULL;

        CkdScriptReportSetup *rs = NULL;
        CkdScriptContinuation *cont = NULL;
        CkdScriptSlide *slide = NULL;
        
        switch (cmd->domain) {
        case CKD_REPORT_SETUP:
                node = g_node_get_root (script);
                script_node = node->data;
                parse_report_setup (script_node->data, cmd->text->str);
                script_node->domain = CKD_REPORT_SETUP;
                break;
        case CKD_CONTINUATION:
                cont = g_slice_alloc (sizeof(CkdScriptContinuation));
                parse_continuation (cont, cmd->text->str);
                
                script_node = g_slice_alloc (sizeof(CkdScriptNode));
                script_node->data = cont;
                script_node->domain = CKD_CONTINUATION;

                g_node_append_data (script, script_node);
                break;
        case CKD_SLIDE:
                slide = g_slice_alloc (sizeof(CkdScriptSlide));
                parse_slide (slide, cmd->text->str);

                script_node = g_slice_alloc (sizeof(CkdScriptNode));
                script_node->data = slide;
                script_node->domain = CKD_SLIDE;
                
                g_node_append_data (script, script_node);
                break;
        default:
                break;
        }
}

static void
ckd_script_parse (GNode *script, GList *cmd_list)
{
        g_list_foreach (cmd_list, parse_cmd, script);
}

static void
cmd_free (gpointer data)
{
        CkdCommand *cmd = data;
        g_slice_free (CkdCommand, cmd);
}

GNode *
ckd_script_new (gchar *filename, gint n_of_slides)
{
        GIOChannel *channel = g_io_channel_new_file (filename, "r", NULL);
        if (!channel) {
                g_error ("I can not open %s", filename);
        }

        /* 去除空白字符 */
        GString *script_text = _get_script_text (channel);
        GList * cmd_list = _create_cmd_list (script_text->str);

        g_string_free (script_text, TRUE);

        ckd_script_validate_cmd_list (cmd_list);

        
        CkdScriptReportSetup *rs = g_slice_alloc (sizeof(CkdScriptReportSetup));
        rs->n_of_slides = n_of_slides;
        rs->default_am = CKD_SLIDE_AM_NULL;
        rs->progress_bar_color = NULL;
        rs->nonius_color = NULL;
        rs->progress_bar_vsize = 16.0;

        CkdScriptNode *root_data = g_slice_alloc (sizeof(CkdScriptNode));
        root_data->data = rs;
        
        GNode *script = g_node_new (root_data);
        ckd_script_parse (script, cmd_list);

        g_list_free_full (cmd_list, cmd_free);

        return script;
}

static gboolean
script_free_cb (GNode *node, gpointer data)
{
        CkdScriptNode *script_node = node->data;
        CkdScriptSlide *slide = NULL;

        switch (script_node->domain) {
        case CKD_REPORT_SETUP:
                g_slice_free (CkdScriptReportSetup, script_node->data);
                break;
        case CKD_CONTINUATION:
                g_slice_free (CkdScriptContinuation, script_node->data);
                break;
        case CKD_SLIDE:
                slide = script_node->data;
                if (slide->text)
                        g_string_free (slide->text, TRUE);
                g_slice_free (CkdScriptSlide, slide);
                break;
        default:
                g_error ("You should tell me which domain!");
                break;
        }
        g_slice_free (CkdScriptNode, script_node);

        return FALSE;
}

void
ckd_script_free (GNode *script)
{
        GNode *root = g_node_get_root (script);
        CkdScriptNode *script_node;

        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_LEAVES,
                         -1,
                         script_free_cb,
                         NULL);
        
        g_node_destroy (script);
}

static gboolean
ckd_script_output (GNode *node, gpointer data)
{
        GList *meta_entry_list = data;
        CkdScriptNode *script_node = node->data;
        CkdMetaEntry *entry = NULL;
        CkdScriptSlide *slide = NULL;
        GList *item = NULL;
        
        switch (script_node->domain) {
        case CKD_REPORT_SETUP:
                break;
        case CKD_CONTINUATION:
                break;
        case CKD_SLIDE:
                slide = script_node->data;
                item = g_list_nth (meta_entry_list, slide->id - 1);
                entry = item->data;
                entry->am = slide->am;
                entry->text = slide->text;
                break;
        default:
                g_error ("You should tell me which domain!");
                break;
        }

        return FALSE;
}

static gboolean
ckd_script_continuation_count (GNode *node, gpointer data)
{
        CkdScriptNode *script_node = node->data;
        gint *n = data;
        
        if (script_node->domain == CKD_CONTINUATION) {
                (*n) ++;
        }

        return FALSE;
}

static gboolean
ckd_script_slides_in_continuation_count (GNode *node, gpointer data)
{
        CkdScriptNode *script_node = node->data;
        gint *n = data;
        
        if (script_node->domain == CKD_CONTINUATION) {
                CkdScriptContinuation *cont = script_node->data;
                (*n) += (cont->tail - cont->head) + 1;
        }

        return FALSE;
}

struct _ContinuationData {
        gint slide_number;
        CkdScriptContinuation *continuation;
};

static gboolean
ckd_script_continuation_query_cb (GNode *node, gpointer data)
{
        CkdScriptNode *script_node = node->data;
        struct _ContinuationData *v = data;

        if (script_node->domain == CKD_CONTINUATION) {
                CkdScriptContinuation *cont = script_node->data;
                if (v->slide_number >= cont->head && v->slide_number <= cont->tail )
                        v->continuation = cont;
        }        

        if (v->continuation)
                return TRUE;
        else       
                return FALSE;
}

static CkdScriptContinuation *
ckd_script_continuation_query (GNode *script, gint slide_id)
{
        struct _ContinuationData data;

        data.slide_number = slide_id;
        data.continuation = NULL;
        
        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         ckd_script_continuation_query_cb,
                         &data);

        return data.continuation;
}

static void
ckd_script_update_slide_ticks (GNode *script, GList *list)
{
        GNode *root = g_node_get_root (script);
        CkdScriptNode *rs_node = root->data;
        CkdScriptReportSetup *rs = rs_node->data;

        gint n_of_slides = rs->n_of_slides;
        
        /* \begin 统计连续体的个数 */
        gint n_of_conts = 0;
        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         ckd_script_continuation_count,
                         &n_of_conts);
        /* \end */

        /* \begin 统计连续体所包含的幻灯片数量 */
        gint n_of_slides_in_conts = 0;
        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         ckd_script_slides_in_continuation_count,
                         &n_of_slides_in_conts);
        /* \end */
        
        gfloat tick_base = n_of_slides - n_of_slides_in_conts + n_of_conts - 1;
        if (tick_base == 0)
                tick_base = 1.0;

        gint real_id = 0, cont_slides_count = 0, cont_head_id;
        CkdScriptContinuation *cont = NULL, *next_cont = NULL;
        CkdMetaEntry *e = NULL;
        GList *iter = g_list_first (list), *cont_iter = NULL;

        enum {
                INIT,
                ENTER_CONT,
                EXIT_CONT,
                IN_CONT,
                NOT_IN_CONT
        } state = INIT;

        for (gint i = 0; i < n_of_slides; i++) {
                e = iter->data;

                switch (state) {
                case INIT:
                        cont = ckd_script_continuation_query (script, i);
                        if (cont)
                                state = ENTER_CONT;
                        else
                                state = NOT_IN_CONT;
                        break;
                case ENTER_CONT:
                        next_cont = ckd_script_continuation_query (script, i+1);
                        if (cont != next_cont)
                                state = EXIT_CONT;
                        else
                                state = IN_CONT;
                        break;
                case IN_CONT:
                        next_cont = ckd_script_continuation_query (script, i+1);
                        if (!next_cont || cont != next_cont)
                                state = EXIT_CONT;
                        break;
                case EXIT_CONT:
                        cont = ckd_script_continuation_query (script, i);
                        if (cont)
                                state = ENTER_CONT;
                        else
                                state = NOT_IN_CONT;
                        break;
                case NOT_IN_CONT:
                        cont = ckd_script_continuation_query (script, i);
                        if (cont)
                                state = ENTER_CONT;
                default:
                        break;
                }
                
                if (state == NOT_IN_CONT) {
                        e->tick = (gfloat)real_id / tick_base;
                        real_id ++;
                }
                if (state == ENTER_CONT) {
                        cont_head_id = real_id;
                        cont_iter = iter;
                        cont_slides_count ++;
                }
                if (state == IN_CONT) {
                        cont_slides_count ++;
                }
                if (state == EXIT_CONT) {
                        cont_slides_count ++;
                        /* \begin 设定延续体内的幻灯片的 tick */
                        gint j = 0;
                        while (cont_iter != g_list_next (iter)) {
                                CkdMetaEntry *cont_e = cont_iter->data;
                                gfloat d = (gfloat)j / (gfloat)cont_slides_count;
                                cont_e->tick = ((gfloat)real_id + d) / tick_base;

                                cont_iter = g_list_next (cont_iter);
                                j++;
                        }
                        /* \end */
                        cont_slides_count = 0;
                        real_id ++;
                }
                
                iter = g_list_next (iter);
        }
}

GList *
ckd_script_output_meta_entry_list (GNode *script)
{
        GList *meta_entry_list = NULL;
        GNode *root = g_node_get_root (script);
        CkdMetaEntry *entry = NULL;

        CkdScriptNode *rs_node = root->data;
        CkdScriptReportSetup *rs = rs_node->data;     
        
        /* \begin 初始化幻灯片元信息表 */
        for (gint i = 0; i < rs->n_of_slides; i++) {
                entry = g_slice_alloc (sizeof(CkdMetaEntry));
                if (ckd_script_continuation_query (script, i)) {
                        /* 延续体中的幻灯片的动画效果必须为 fade */
                        entry->am = CKD_SLIDE_AM_FADE;
                } else
                       entry->am = rs->default_am;   
                entry->text = NULL;
                meta_entry_list = g_list_append (meta_entry_list, entry);
        }
        /* \end */
        
        /* \begin 输出幻灯片配置 */
        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_ALL,
                         -1,
                         ckd_script_output,
                         meta_entry_list);
        /* \end */
        
        ckd_script_update_slide_ticks (script, meta_entry_list);
        
        return meta_entry_list;
}

ClutterColor *
ckd_script_get_progress_bar_color (GNode *script)
{
        GNode *root = g_node_get_root (script);
        CkdScriptNode *report_setup_node = root->data;
        CkdScriptReportSetup *report_setup = report_setup_node->data;
        
        if (report_setup) {
                return report_setup->progress_bar_color;
        } else
                return NULL;
}

ClutterColor *
ckd_script_get_nonius_color (GNode *script)
{
        GNode *root = g_node_get_root (script);
        CkdScriptNode *report_setup_node = root->data;
        CkdScriptReportSetup *report_setup = report_setup_node->data;

        if (report_setup) {
                return report_setup->nonius_color;
        } else
                return NULL;
}

gfloat
ckd_script_get_progress_bar_vsize (GNode *script)
{
        GNode *root = g_node_get_root (script);
        CkdScriptNode *report_setup_node = root->data;
        CkdScriptReportSetup *report_setup = report_setup_node->data;

        if (report_setup) {
                return report_setup->progress_bar_vsize;
        } else
                return -1.0;
}

static gboolean
ckd_meta_entry_equal (CkdMetaEntry *a, CkdMetaEntry *b)
{
        if (a->am != b->am)
                return FALSE;
        if (a->tick != b->tick)
                return FALSE;
        
        /* \begin 幻灯片备注文本尚未实现，所以在此未作比较 */
        /* todo */
        /* \end */

        return TRUE;
}

gboolean
ckd_script_equal (GNode *a, GNode *b)
{
        /* \begin 比较根结点 */
        GNode *root_a = g_node_get_root (a);
        CkdScriptNode *rs_node_a = root_a->data;
        
        GNode *root_b = g_node_get_root (b);
        CkdScriptNode *rs_node_b = root_b->data;
        
        if (rs_node_a && rs_node_b) {
                CkdScriptReportSetup *rs_a = rs_node_a->data;
                CkdScriptReportSetup *rs_b = rs_node_b->data;

                if (rs_a->default_am != rs_b->default_am)
                        return FALSE;

                if (rs_a->progress_bar_color != rs_b->progress_bar_color) {
                        if (!rs_a->progress_bar_color)
                                return FALSE;
                        else if (rs_b->progress_bar_color) {
                                if (!clutter_color_equal (rs_a->progress_bar_color,
                                                          rs_b->progress_bar_color))
                                        return FALSE;
                        }
                }
                if (rs_a->nonius_color != rs_b->nonius_color) {
                        if (!rs_a->nonius_color)
                                return FALSE;
                        else if (rs_b->nonius_color) {
                                if (!clutter_color_equal (rs_a->nonius_color, rs_b->nonius_color))
                                        return FALSE;
                        }
                }
                if (G_MINFLOAT < fabsf (rs_a->progress_bar_vsize - rs_b->progress_bar_vsize))
                        return FALSE;
        }
        /* \end */

        /* \begin 比较各个幻灯片的设置 */
        GList *list_a = ckd_script_output_meta_entry_list (a);
        GList *list_b = ckd_script_output_meta_entry_list (b);
        GList *iter_a = g_list_first (list_a);
        GList *iter_b = g_list_first (list_b);
        while (iter_a && iter_b) {
                CkdMetaEntry *e_a = iter_a->data;
                CkdMetaEntry *e_b = iter_b->data;
                if (!ckd_meta_entry_equal (e_a, e_b)) {
                        return FALSE;
                }

                iter_a = g_list_next (iter_a);
                iter_b = g_list_next (iter_b);
        }
        /* \end */

        return TRUE;
}
