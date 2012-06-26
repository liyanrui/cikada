#include <stdio.h>
#include "ckd-slide-script.h"

#define CKD_SCRIPT_CMD_N 4

typedef enum {
        CKD_SETUPREPORT,
        CKD_TITLE,
        CKD_SECTION,
        CKD_SLIDE
} CkdScriptCommandType;

typedef struct _SetupReport SetupReport;
struct _SetupReport {
        gdouble duration;
        CkdSlideEnteringEffect enter;
        CkdSlideExitEffect exit;
};

typedef struct _Slide Slide;
struct _Slide {
        gint id;
        gdouble duration;
        CkdSlideEnteringEffect enter;
        CkdSlideExitEffect exit;
        GString *text;
};

typedef struct _Command Command;
struct _Command {
        GString *text;
        CkdScriptCommandType type;
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
        Command *cmd = NULL;

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
                        cmd = g_slice_alloc (sizeof(Command));
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
        Command *cmd = data;
        GRegex **regexes = user_data;
        gboolean result = FALSE;
        g_print ("%s\n", cmd->text->str);

        for (gint i = 0; i < CKD_SCRIPT_CMD_N; i++) {
                result = g_regex_match (regexes[i], cmd->text->str, 0, NULL);
                if (result) {
                        cmd->type = i;
                        break;
                }
        }

        if (!result) {
                g_error ("I can not recognize `%s'", cmd->text->str);
        }
}

static void
ckd_slide_script_validate_cmd_list (GList *cmd_list)
{
        /* 元素的次序是按照 CkdScriptCommandType 排列的 */
        GRegex *regexes[CKD_SCRIPT_CMD_N] = {
                g_regex_new ("\\\\setupreport\\s*(\\[.+\\])+", 0, 0, NULL),
                g_regex_new ("\\\\title\\s*\\{.*\\}", 0, 0, NULL),
                g_regex_new ("\\\\(sub)*section\\s*(\\[.+\\])+\\s*\\{.*\\}", 0, 0, NULL),
                g_regex_new ("\\\\slide\\s*(\\[.+\\])+\\s*(\\{.*\\})*", 0, 0, NULL)
        };

        g_list_foreach (cmd_list, validate_cmd, regexes);

        for (gint i = 0; i < CKD_SCRIPT_CMD_N; i++) {
                g_regex_unref (regexes[i]);
        }
}

static gdouble
get_duration_from_text (gchar *text)
{
        gdouble result = 0.0;

        gchar **splitted_text = NULL;

        if (g_str_has_suffix (text, "h")) {
                splitted_text = g_strsplit (text, "h", -1);
                splitted_text[0] = g_strstrip (splitted_text[0]);
                result = 3600 * g_ascii_strtod (splitted_text[0], NULL);
        } else if (g_str_has_suffix (text, "m")) {
                splitted_text = g_strsplit (text, "m", -1);
                splitted_text[0] = g_strstrip (splitted_text[0]);
                result = 60 * g_ascii_strtod (splitted_text[0], NULL);
        } else if (g_str_has_suffix (text, "s")) {
                splitted_text = g_strsplit (text, "s", -1);
                splitted_text[0] = g_strstrip (splitted_text[0]);
                result = g_ascii_strtod (splitted_text[0], NULL);
        }

        g_strfreev (splitted_text);

        return result;
}

static CkdSlideEnteringEffect
get_entering_effect (gchar *effect)
{
        CkdSlideEnteringEffect enter = CKD_SLIDE_AM_FADE_ENTER;

        if (g_str_equal (effect, "fade"))
                enter = CKD_SLIDE_AM_FADE_ENTER;
        else if (g_str_equal (effect, "scale"))
                enter = CKD_SLIDE_SCALE_ENTER;
        else if (g_str_equal (effect, "down"))
                enter = CKD_SLIDE_DOWN_ENTER;
        else if (g_str_equal (effect, "up"))
                enter = CKD_SLIDE_UP_ENTER;
        else if (g_str_equal (effect, "right"))
                enter = CKD_SLIDE_AM_RIGHT_ENTER;
        else if (g_str_equal (effect, "left"))
                enter = CKD_SLIDE_AM_LEFT_ENTER;

        return enter;
}

static CkdSlideExitEffect
get_exit_effect (gchar *effect)
{
        CkdSlideExitEffect exit = CKD_SLIDE_AM_FADE_EXIT;

        if (g_str_equal (effect, "fade"))
                exit = CKD_SLIDE_AM_FADE_EXIT;
        else if (g_str_equal (effect, "scale"))
                exit = CKD_SLIDE_SCALE_EXIT;
        else if (g_str_equal (effect, "down"))
                exit = CKD_SLIDE_DOWN_EXIT;
        else if (g_str_equal (effect, "up"))
                exit = CKD_SLIDE_UP_EXIT;
        else if (g_str_equal (effect, "right"))
                exit = CKD_SLIDE_AM_RIGHT_EXIT;
        else if (g_str_equal (effect, "left"))
                exit = CKD_SLIDE_AM_LEFT_EXIT;

        return exit;
}

static SetupReport *
parse_setupreport (gchar *text)
{
        gchar **splitted_text = NULL;
        GMatchInfo *info;

        SetupReport *setupreport = g_slice_alloc (sizeof(SetupReport));
        setupreport->duration = -1.0;
        setupreport->enter = CKD_SLIDE_NULL_ENTER;
        setupreport->exit = CKD_SLIDE_NULL_EXIT;

        /* @begin: 解析 duration */
        GRegex *duration_re = g_regex_new ("duration\\s*=\\s*[\\d\\.]+[hms]",
                                           0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);
                setupreport->duration = get_duration_from_text (splitted_text[1]);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (duration_re);
        /* @end */

        /* @begin: 解析 enter */
        GRegex *enter_re = g_regex_new ("enter\\s*=\\s*[a-z\\s]+",
                                        0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);

                gchar *effect = g_strstrip (splitted_text[1]);
                setupreport->enter = get_exit_effect (effect);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (enter_re);
        /* @end */

        /* @begin: 解析 exit */
        GRegex *exit_re = g_regex_new ("exit\\s*=\\s*[a-z\\s]+",
                                       0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);

                gchar *effect = g_strstrip (splitted_text[1]);
                setupreport->exit = get_exit_effect (effect);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (exit_re);
        /* @end */

        return setupreport;
}

static Slide *
parse_slide (gchar *text)
{
        Slide *slide = g_slice_alloc (sizeof(Slide));
        slide->duration = -1.0;
        slide->enter = CKD_SLIDE_NULL_ENTER;
        slide->exit = CKD_SLIDE_NULL_EXIT;
        slide->text = NULL;

        gchar **splitted_text = NULL;
        GMatchInfo *info;

        /* @begin: 解析 id */
        GRegex *id_re = g_regex_new ("\\[\\d+\\]",
                                     0, 0, NULL);
        g_regex_match (id_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *s = g_match_info_fetch (info, 0);
                slide->id = g_ascii_strtod (s + 1, NULL);
                g_free (s);
        }
        g_match_info_free (info);
        g_regex_unref (id_re);
        /* @end */

        /* @begin: 解析 duration */
        GRegex *duration_re = g_regex_new ("duration\\s*=\\s*[\\d\\.]+[hms]",
                                           0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);
                slide->duration = get_duration_from_text (splitted_text[1]);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (duration_re);
        /* @end */

        /* @begin: 解析 enter */
        GRegex *enter_re = g_regex_new ("enter\\s*=\\s*[a-z\\s]+",
                                        0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);

                gchar *effect = g_strstrip (splitted_text[1]);
                slide->enter = get_exit_effect (effect);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (enter_re);
        /* @end */

        /* @begin: 解析 exit */
        GRegex *exit_re = g_regex_new ("exit\\s*=\\s*[a-z\\s]+",
                                       0, 0, NULL);
        g_regex_match (duration_re, text,  0, &info);
        if (g_match_info_matches (info)) {
                gchar *kv = g_match_info_fetch (info, 0);
                splitted_text = g_strsplit (kv, "=", 0);

                gchar *effect = g_strstrip (splitted_text[1]);
                slide->exit = get_exit_effect (effect);
                g_strfreev (splitted_text);
                g_free (kv);
        }
        g_match_info_free (info);
        g_regex_unref (exit_re);
        /* @end */

        return slide;
}

static void
parse_cmd (gpointer data, gpointer user_data)
{
        Command *cmd = data;
        GNode *script = user_data;
        GNode *node = NULL;

        switch (cmd->type) {
        case CKD_SETUPREPORT:
                node = g_node_get_root (script);
                node->data = parse_setupreport (cmd->text->str);
                break;
        case CKD_SLIDE:
                g_node_append_data (script, parse_slide (cmd->text->str));
                break;
        default:
                break;
        }
}

static GNode *
ckd_slide_script_parse (GList *cmd_list)
{
        GNode *script = g_node_new (NULL);

        g_list_foreach (cmd_list, parse_cmd, script);

        return script;
}

static void
cmd_free (gpointer data)
{
        Command *cmd = data;
        g_slice_free (Command, cmd);
}

GNode *
ckd_slide_script_new (gchar *filename)
{
        GIOChannel *channel = g_io_channel_new_file (filename, "r", NULL);
        if (!channel) {
                g_error ("I can not open %s", filename);
        }

        /* 去除空白字符 */
        GString *script_text = _get_script_text (channel);
        GList * cmd_list = _create_cmd_list (script_text->str);

        g_string_free (script_text, TRUE);

        ckd_slide_script_validate_cmd_list (cmd_list);

        GNode *script = ckd_slide_script_parse (cmd_list);

        g_list_free_full (cmd_list, cmd_free);

        return script;
}

static gboolean
slide_free (GNode *node, gpointer data)
{
        Slide *slide = node->data;

        if (slide->text)
                g_string_free (slide->text, TRUE);

        g_slice_free (Slide, slide);

        return FALSE;
}

void
ckd_slide_script_free (GNode *script)
{
        GNode *root = g_node_get_root (script);

        if (root->data) {
                g_slice_free (SetupReport, root->data);
        }

        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_LEAVES,
                         -1,
                         slide_free,
                         NULL);

}

static gboolean
script_output (GNode *node, gpointer data)
{
        GList *meta_entry_list = data;
        Slide *slide = node->data;

        GList *item = g_list_nth (meta_entry_list, slide->id - 1);
        CkdMetaEntry *entry = item->data;

        entry->duration = slide->duration;
        entry->enter = slide->enter;
        entry->exit = slide->exit;
        entry->text = slide->text;

        return FALSE;
}

GList *
ckd_slide_script_out_meta_entry_list (GNode *script, gint n_of_slides)
{
        GList *meta_entry_list = NULL;
        GNode *root = g_node_get_root (script);
        CkdMetaEntry *entry = NULL;

        if (root->data) {
                SetupReport *setupreport= root->data;

                for (gint i = 0; i < n_of_slides; i++) {
                        entry = g_slice_alloc (sizeof(CkdMetaEntry));
                        entry->enter = setupreport->enter;
                        entry->exit  = setupreport->exit;
                        meta_entry_list = g_list_append (meta_entry_list, entry);
                }
        } else {
                for (gint i = 0; i < n_of_slides; i++) {
                        entry = g_slice_alloc (sizeof(CkdMetaEntry));
                        entry->enter = CKD_SLIDE_AM_FADE_ENTER;
                        entry->exit  = CKD_SLIDE_AM_FADE_EXIT;
                        meta_entry_list = g_list_append (meta_entry_list, entry);
                }
        }
        

        g_node_traverse (script,
                         G_LEVEL_ORDER,
                         G_TRAVERSE_LEAVES,
                         -1,
                         script_output,
                         meta_entry_list);

        return meta_entry_list;
}
