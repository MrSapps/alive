#include "gui.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>

static void *gui_frame_alloc(GuiContext *ctx, int size);

const int gui_zero_v2i[2] = {0};

#if defined(_MSC_VER) && _MSC_VER <= 1800 // MSVC 2013
size_t v_sprintf_impl(char *buf, size_t count, const char *fmt, va_list args)
{
    size_t ret = _vsnprintf(buf, count, fmt, args);
    // Fix unsafeness of msvc _vsnprintf
    if (buf && count > 0)
        buf[count - 1] = '\0';
    return ret;
}

void sprintf_impl(char *buf, size_t count, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    v_sprintf_impl(buf, count, fmt, args);
    va_end(args);
}

#   define GUI_FMT_STR sprintf_impl
#   define GUI_V_FMT_STR v_sprintf_impl
#else
#   define GUI_FMT_STR snprintf
#   define GUI_V_FMT_STR vsnprintf
#endif

#define GUI_MAX(a, b) ((a > b) ? (a) : (b))
#define GUI_MIN(a, b) ((a < b) ? (a) : (b))
#define GUI_CLAMP(v, a, b) (GUI_MIN(GUI_MAX((v), (a)), (b)))
#define GUI_ABS(x) ((x) < 0 ? (-x) : x)
#define GUI_WINDOW_TITLE_BAR_HEIGHT 25
#define GUI_SCROLL_BAR_WIDTH 15
#define GUI_LAYERS_PER_WINDOW 10000 // Maybe 10k layers inside a window is enough
#define GUI_UNUSED(x) (void)(x) // For unused params. C doesn't allow omitting param name.
#define GUI_TRUE 1
#define GUI_FALSE 0

#define GUI_NONE_WINDOW_IX (-2)
#define GUI_BG_WINDOW_IX (-1)

static void *check_ptr(void *ptr)
{
    if (!ptr) {
        abort();
    }
    return ptr;
}

#define GUI_DECL_V2(type, name, x, y) type name[2]; name[0] = x; name[1] = y;
#define GUI_V2(stmt) do { int c = 0; stmt; c = 1; stmt; } while(0)
#define GUI_ASSIGN_V2(a, b) GUI_V2((a)[c] = (b)[c])
#define GUI_EQUALS_V2(a, b) ((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define GUI_ZERO(var) memset(&var, 0, sizeof(var))

GUI_BOOL v2i_in_rect(int v[2], int pos[2], int size[2])
{ return v[0] >= pos[0] && v[1] >= pos[1] && v[0] < pos[0] + size[0] && v[1] < pos[1] + size[1]; }

void destroy_window(GuiContext *ctx, int handle)
{
    GUI_BOOL found = GUI_FALSE;
    for (int i = 0; i < ctx->window_count; ++i) {
        if (!found) {
            if (ctx->window_order[i] == handle)
                found = GUI_TRUE;
        } else {
            ctx->window_order[i - 1] = ctx->window_order[i];
        }
    }
    --ctx->window_count;

    GuiContext_Window *win = &ctx->windows[handle];

    GUI_ZERO(*win);
}

int gui_window_order(GuiContext *ctx, int handle)
{
    for (int i = 0; i < ctx->window_count; ++i) {
        if (ctx->window_order[i] == handle)
            return i;
    }
    return -1;
}

void pt_to_px(int px[2], int pt[2], float dpi_scale)
{
    px[0] = (int)floorf(pt[0]*dpi_scale + 0.5f);
    px[1] = (int)floorf(pt[1]*dpi_scale + 0.5f);
}

float pt_to_px_f32(float pt, float dpi_scale)
{
    return pt*dpi_scale;
}

void px_to_pt(int pt[2], int px[2], float dpi_scale)
{
    pt[0] = (int)floorf(px[0] / dpi_scale + 0.5f);
    pt[1] = (int)floorf(px[1] / dpi_scale + 0.5f);
}

#if 0
Skin create_skin()
{
    Skin skin;
    skin.knob_arc_width = 5;
    skin.knob_bloom_width = 5;
    skin.knob_size = v2i(32, 32);
    skin.element_offsets = create_tbl(GuiId, V2i)(0, v2i(100000, 100000), MAX_GUI_ELEMENT_COUNT);
    skin.element_sizes = create_tbl(GuiId, V2i)(0, v2i(100000, 100000), MAX_GUI_ELEMENT_COUNT);
    skin.element_grids = create_tbl(GuiId, U8)(0, 1, MAX_GUI_ELEMENT_COUNT);
    skin.element_colors = create_tbl(GuiId, Color)(0, color_white, MAX_GUI_ELEMENT_COUNT);
    skin.element_resize_handles = create_tbl(GuiId, U8)(0, 0, MAX_GUI_ELEMENT_COUNT);
    return skin;
}

void destroy_skin(Skin *skin)
{
    destroy_tbl(GuiId, V2i)(&skin->element_offsets);
    destroy_tbl(GuiId, V2i)(&skin->element_sizes);
    destroy_tbl(GuiId, U8)(&skin->element_grids);
    destroy_tbl(GuiId, Color)(&skin->element_colors);
    destroy_tbl(GuiId, U8)(&skin->element_resize_handles);
}

void save_skin(const Skin *s, const char *path)
{
    const char * top =
        "// This file is generated in save_skin()\n"
        "\n"
        "#include <gui/gui.h>\n"
        "\n"
        "Skin create_saved_skin()\n"
        "{\n"
        "	Skin s = create_skin();\n";
    const char *bottom =
        "	return s;\n"
        "}\n";

    FILE *file = fopen(path, "wb");
    assert(file);

    fprintf(file, "%s", top);
    fprintf(file, "	s.knob_arc_width = %ff;\n", s->knob_arc_width);
    fprintf(file, "	s.knob_bloom_width = %ff;\n", s->knob_bloom_width);
    fprintf(file, "	s.knob_size = v2i(%i, %i);\n", s->knob_size[0], s->knob_size[1]);
    fprintf(file, "	s.default_color = color(%ff, %ff, %ff, %ff);\n", s->default_color.r, s->default_color.g, s->default_color.b, s->default_color.a);
    fprintf(file, "\n");

    for (U32 i = 0; i < s->element_offsets.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_offsets.array_data[i];
        if (entry.key == s->element_offsets.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_offsets, %u, v2i(%i, %i));\n",
            entry.key, entry.value[0], entry.value[1]);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_sizes.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_sizes.array_data[i];
        if (entry.key == s->element_sizes.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_sizes, %u, v2i(%i, %i));\n",
            entry.key, entry.value[0], entry.value[1]);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_grids.array_size; ++i) {
        const HashTbl_Entry(GuiId, U8) entry = s->element_grids.array_data[i];
        if (entry.key == s->element_grids.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, U8)(&s.element_grids, %u, %u);\n",
            entry.key, entry.value);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_colors.array_size; ++i) {
        const HashTbl_Entry(GuiId, Color) entry = s->element_colors.array_data[i];
        if (entry.key == s->element_colors.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, Color)(&s.element_colors, %u, color(%ff, %ff, %ff, %ff));\n",
            entry.key, entry.value.r, entry.value.g, entry.value.b, entry.value.a);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_resize_handles.array_size; ++i) {
        const HashTbl_Entry(GuiId, U8) entry = s->element_resize_handles.array_data[i];
        if (entry.key == s->element_resize_handles.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, U8)(&s.element_resize_handles, %u, %u);\n",
            entry.key, entry.value);
    }
    fprintf(file, "\n");

    fprintf(file, "%s", bottom);

    fclose(file);

    PRINTF("save_skin(%s): completed", path);
}
#endif

// Labels can contain prefixed stuff (to keep them unique for example).
// "hoopoa|asdfg" -> "asdfg".
const char *gui_label_text(const char *label)
{
    int len = (int)strlen(label);
    for (int i = len - 1; i >= 0; --i) {
        if (label[i] == '|')
            return label + i + 1;
    }
    return label;
}

const char *gui_str(GuiContext *ctx, const char *fmt, ...)
{
    char *text = NULL;
    va_list args;
    va_list args_copy;

    va_start(args, fmt);
    va_copy(args_copy, args);
    int size = (int)GUI_V_FMT_STR(NULL, 0, fmt, args) + 1;
    text = (char*)gui_frame_alloc(ctx, size);
    GUI_V_FMT_STR(text, size, fmt, args_copy);
    va_end(args_copy);
    va_end(args);
    return text;
}

void gui_set_next_window_pos(GuiContext *ctx, int x, int y)
{
    ctx->next_window_pos[0] = x;
    ctx->next_window_pos[1] = y;
}

void gui_set_turtle_pos(GuiContext *ctx, int x, int y)
{
    ctx->turtles[ctx->turtle_ix].pos[0] = x;
    ctx->turtles[ctx->turtle_ix].pos[1] = y;
    gui_enlarge_bounding(ctx, x, y);
}

GuiContext_Turtle *gui_turtle(GuiContext *ctx)
{
    return &ctx->turtles[ctx->turtle_ix];
}

GuiContext_Frame *gui_frame(GuiContext *ctx)
{
    assert(gui_turtle(ctx)->frame_ix < ctx->frame_count);
    return gui_turtle(ctx)->frame_ix >= 0 ? &ctx->frames[gui_turtle(ctx)->frame_ix] : NULL;
}

GuiContext_Window *gui_window(GuiContext *ctx)
{
    assert(gui_turtle(ctx)->window_ix < ctx->window_count);
    return gui_turtle(ctx)->window_ix >= 0 ? &ctx->windows[gui_turtle(ctx)->window_ix] : NULL;
}

// @note Change of focus is delayed by one frame (similar to activation)
GUI_BOOL gui_focused(GuiContext *ctx)
{
    if (ctx->window_count == 0)
        return GUI_TRUE;
    return gui_turtle(ctx)->window_ix == ctx->focused_win_ix;
}

void gui_turtle_pos(GuiContext *ctx, int *x, int *y)
{
    *x = gui_turtle(ctx)->pos[0];
    *y = gui_turtle(ctx)->pos[1];
}

void gui_parent_turtle_start_pos(GuiContext *ctx, int pos[2])
{
    if (ctx->turtle_ix == 0) {
        pos[0] = pos[1] = 0;
    } else {
        GUI_ASSIGN_V2(pos, ctx->turtles[ctx->turtle_ix - 1].start_pos);
    }
}

GuiScissor *gui_scissor(GuiContext *ctx)
{
    GuiScissor *s = &gui_turtle(ctx)->scissor;
    return s->size[0] == 0 ? NULL : s;
}

// Returns whether the current turtle with certain size is at least partially visible in the client area of the current window
GUI_BOOL gui_is_inside_window(GuiContext *ctx, int size[2])
{
    // TODO: Take size into account
    int pos[2];
    gui_turtle_pos(ctx, &pos[0], &pos[1]);
    GuiContext_Window *win = gui_window(ctx);
    if (pos[0] + size[0] <= win->pos[0] || pos[1] + size[1] <= win->pos[1] + GUI_WINDOW_TITLE_BAR_HEIGHT)
        return GUI_FALSE;
    if (pos[0] >= win->pos[0] + win->total_size[0] || pos[1] >= win->pos[1] + win->total_size[1])
        return GUI_FALSE;
    return GUI_TRUE;
}

void gui_start_dragging(GuiContext *ctx, float start_value[2])
{
    assert(!ctx->dragging);
    ctx->dragging = GUI_TRUE;
    GUI_ASSIGN_V2(ctx->drag_start_pos, ctx->cursor_pos);
    GUI_ASSIGN_V2(ctx->drag_start_value, start_value);
    // Store dragdropdata
    ctx->dragdropdata = gui_turtle(ctx)->inactive_dragdropdata;
}

GuiId gui_id(const char *label)
{
    int size = 0;
    while (label[size] && label[size] != '|') // gui_id("foo_button|Press this") == gui_id("foo_button|Don't press this")
        ++size;

    // Modified FNV-1a
    uint32_t hash = 2166136261;
    for (int i = 0; i < size; ++i)
        hash = ((hash ^ label[i]) + 379721) * 16777619;
    return hash;
}

static void *gui_frame_alloc(GuiContext *ctx, int size)
{
    assert(ctx->framemem_bucket_count >= 1);
    GuiContext_MemBucket *bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count - 1];
    if (bucket->used + size > bucket->size) {
        // Need a new bucket :(
        int new_bucket_count = ctx->framemem_bucket_count + 1;
        ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)*new_bucket_count));

        int bucket_size = GUI_MAX(size, bucket->size * 2);
        bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count++];
        bucket->data = check_ptr(malloc(bucket_size));
        bucket->size = bucket_size;
        bucket->used = 0;
    }

    char *mem = (char *)bucket->data + bucket->used; // @todo Alignment
    bucket->used += size;
    assert(bucket->used <= bucket->size);
    return (void*)mem;
}

// Resize and clean frame memory
static void refresh_framemem(GuiContext *ctx)
{
    if (ctx->framemem_bucket_count > 1) { // Merge buckets to one for next frame
        int memory_size = ctx->framemem_buckets[0].size;
        for (int i = 1; i < ctx->framemem_bucket_count; ++i) {
            memory_size += ctx->framemem_buckets[i].size;
            free(ctx->framemem_buckets[i].data);
        }

        ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)));
        ctx->framemem_buckets[0].data = check_ptr(realloc(ctx->framemem_buckets[0].data, memory_size));
        ctx->framemem_buckets[0].size = memory_size;

        ctx->framemem_bucket_count = 1;
    }

    ctx->framemem_buckets[0].used = 0;
}

GuiContext *create_gui(GuiCallbacks callbacks)
{
    GuiContext *ctx = (GuiContext*)calloc(1, sizeof(*ctx));
    ctx->dpi_scale = 1.0f;
    ctx->callbacks = callbacks;
    ctx->hot_layer = -1;
    ctx->active_win_ix = GUI_NONE_WINDOW_IX;
    ctx->focused_win_ix = -1;
    ctx->host_win_size[0] = 800;
    ctx->host_win_size[1] = 600;

    // "Null" turtle
    gui_turtle(ctx)->window_ix = GUI_BG_WINDOW_IX;
    gui_turtle(ctx)->frame_ix = -1;

    ctx->framemem_bucket_count = 1;
    ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(malloc(sizeof(*ctx->framemem_buckets)));
    ctx->framemem_buckets[0].data = check_ptr(malloc(GUI_DEFAULT_MAX_FRAME_MEMORY));
    ctx->framemem_buckets[0].size = GUI_DEFAULT_MAX_FRAME_MEMORY;
    ctx->framemem_buckets[0].used = 0;

    return ctx;
}

void destroy_gui(GuiContext *ctx)
{
    if (ctx)
    {
        for (int i = 0; i < ctx->framemem_bucket_count; ++i)
            free(ctx->framemem_buckets[i].data);
        free(ctx->framemem_buckets);

        //destroy_skin(&ctx->skin);

        for (int i = MAX_GUI_WINDOW_COUNT - 1; i >= 0; --i) {
            if (ctx->windows[i].id)
                destroy_window(ctx, i);
        }

        free(ctx);
    }
}

void gui_write_char(GuiContext *ctx, char ch)
{
    if (ctx->written_char_count >= (int)sizeof(ctx->written_text_buf))
        return;
    ctx->written_text_buf[ctx->written_char_count++] = ch;
}

int gui_layer(GuiContext *ctx) { return gui_turtle(ctx)->layer; }

void gui_set_hot(GuiContext *ctx, const char *label)
{
    if (ctx->active_id == 0) {
        GUI_BOOL set_hot = GUI_FALSE;
        if (ctx->hot_id == 0) {
            set_hot = GUI_TRUE;
        } else {
            // Last overlapping element of the topmost window gets to be hot
            if (ctx->hot_layer <= gui_turtle(ctx)->layer)
                set_hot = GUI_TRUE;
        }
        if (set_hot) {
            ctx->hot_id = gui_id(label);
            ctx->hot_layer = gui_turtle(ctx)->layer;
        }
    }
}

GUI_BOOL gui_is_hot(GuiContext *ctx, const char *label)
{
    return ctx->last_hot_id == gui_id(label);
}

void gui_set_active(GuiContext *ctx, const char *label)
{
    ctx->active_id = gui_id(label);
    ctx->last_active_id = ctx->active_id;
    ctx->active_win_ix = gui_turtle(ctx)->window_ix;
    ctx->focused_win_ix = gui_turtle(ctx)->window_ix;
    ctx->hot_id = 0; // Prevent the case where hot becomes assigned some different (overlapping) element than active
}

void gui_set_inactive(GuiContext *ctx, GuiId id)
{
    if (ctx->active_id == id) {
        ctx->active_id = 0;
        ctx->active_win_ix = GUI_NONE_WINDOW_IX;
    }
}

GUI_BOOL gui_is_active(GuiContext *ctx, const char *label)
{
    return ctx->active_id == gui_id(label);
}

void gui_button_logic(GuiContext *ctx, const char *label, int pos[2], int size[2], GUI_BOOL *went_up, GUI_BOOL *went_down, GUI_BOOL *down, GUI_BOOL *hover)
{
    if (went_up) *went_up = GUI_FALSE;
    if (went_down) *went_down = GUI_FALSE;
    if (down) *down = GUI_FALSE;
    if (hover) *hover = GUI_FALSE;

    GUI_BOOL was_released = GUI_FALSE;
    if (gui_is_active(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) {
            if (down) *down = GUI_TRUE;
        }
        else if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_RELEASED_BIT) {
            if (went_up) *went_up = GUI_TRUE;
            gui_set_inactive(ctx, gui_id(label));
            was_released = GUI_TRUE;
        }
    }
    else if (gui_is_hot(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_PRESSED_BIT) {
            if (down) *down = GUI_TRUE;
            if (went_down) *went_down = GUI_TRUE;
            gui_set_active(ctx, label);
        }
    }

    int c_p[2];
    px_to_pt(c_p, ctx->cursor_pos, ctx->dpi_scale);
    GuiScissor *s = gui_scissor(ctx);
    if (    v2i_in_rect(c_p, pos, size) &&
            (!s || v2i_in_rect(c_p, s->pos, s->size))) {
        gui_set_hot(ctx, label);
        if (hover && (gui_is_hot(ctx, label) || gui_is_active(ctx, label) || was_released)) *hover = GUI_TRUE;
    }
}

void gui_pos(GuiContext *ctx, int ret[2], const char *label, int p[2])
{
	GUI_UNUSED(ctx);
	GUI_UNUSED(label);
    GUI_ASSIGN_V2(ret, p);
}

void gui_size(GuiContext *ctx, int ret[2], const char *label, int p[2])
{
	GUI_UNUSED(ctx);
	GUI_UNUSED(label);
    GUI_ASSIGN_V2(ret, p);
}

void gui_begin(GuiContext *ctx, const char *label)
{
    assert(ctx->turtle_ix < MAX_GUI_STACK_SIZE);
    if (ctx->turtle_ix >= MAX_GUI_STACK_SIZE)
        ctx->turtle_ix = 0; // Failsafe

    GuiContext_Turtle *prev = &ctx->turtles[ctx->turtle_ix];
    GuiContext_Turtle *cur = &ctx->turtles[++ctx->turtle_ix];

	GuiContext_Turtle new_turtle;
    GUI_ZERO(new_turtle);
    gui_pos(ctx, new_turtle.pos, label, prev->pos);
    GUI_ASSIGN_V2(new_turtle.start_pos, new_turtle.pos);
    GUI_ASSIGN_V2(new_turtle.bounding_max, new_turtle.pos);
    GUI_ASSIGN_V2(new_turtle.last_bounding_max, new_turtle.pos);
    new_turtle.frame_ix = prev->frame_ix;
    new_turtle.window_ix = prev->window_ix;
    new_turtle.layer = prev->layer + 1;
    new_turtle.detached = GUI_FALSE;
    new_turtle.inactive_dragdropdata = prev->inactive_dragdropdata;
    new_turtle.scissor = prev->scissor;
    GUI_FMT_STR(new_turtle.label, MAX_GUI_LABEL_SIZE, "%s", label);
    *cur = new_turtle;

    int size[2];
    int def[2] = {-1, -1};
    gui_size(ctx, size, label, def);
    if (!GUI_EQUALS_V2(size, def)) {
        // This panel has predetermined (minimum) size
        GUI_V2(gui_turtle(ctx)->bounding_max[c] = gui_turtle(ctx)->start_pos[c] + size[c]);
    }
}

#define GUI_HANDLE_RIGHT_BIT 0x01
#define GUI_HANDLE_BOTTOM_BIT 0x02

#if 0
// Resize handle for parent element
void do_resize_handle(GuiContext *ctx, GUI_BOOL right_handle)
{
    V2i panel_size = gui_turtle(ctx)->bounding_max - gui_turtle(ctx)->start_pos;
    //GuiId panel_id = gui_id(gui_turtle(ctx)->label);

    char handle_label[MAX_GUI_LABEL_SIZE];
    GUI_FMT_STR(handle_label, ARRAY_COUNT(handle_label), "%ihandle_%s", right_handle ? 0 : 1, gui_turtle(ctx)->label);
    { gui_begin(ctx, handle_label);
    V2i handle_pos;
    V2i handle_size;
    if (right_handle) {
        handle_size = gui_size(ctx, handle_label, v2i(5, 20));
        handle_pos = gui_turtle(ctx)->pos + v2i(panel_size[0] - handle_size[0] / 2, panel_size[1] / 2 - handle_size[1] / 2);
    }
    else {
        handle_size = gui_size(ctx, handle_label, v2i(20, 5));
        handle_pos = gui_turtle(ctx)->pos + v2i(panel_size[0] / 2 - handle_size[0] / 2, panel_size[1] - handle_size[1] / 2);
    }

    GUI_BOOL went_down;
    GUI_BOOL down;
    GUI_BOOL hover;
    gui_button_logic(ctx, handle_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

    if (went_down)
        ctx->drag_start_value = v2i_to_v2f(panel_size);

    if (down && ctx->dragging) {
        V2i new_size = v2f_to_v2i(ctx->drag_start_value) + px_to_pt(ctx->cursor_pos - ctx->drag_start_pos, ctx->dpi_scale);
        new_size[0] = GUI_MAX(new_size[0], 10);
        new_size[1] = GUI_MAX(new_size[1], 10);
        if (right_handle)
            new_size[1] = panel_size[1];
        else
            new_size[0] = panel_size[0];
        // @todo Resizing by user should maybe belong to somewhere else than skin?
        set_tbl(GuiId, V2i)(&ctx->skin.element_sizes, panel_id, new_size);
    }

    // @todo Change cursor when hovering

    //Color col = ctx->skin.default_color;
    //col.a *= 0.5f; // @todo Color to skin
    //draw_rect(gui_rendering(ctx), pt_to_px(handle_pos, ctx->dpi_scale), pt_to_px(handle_size, ctx->dpi_scale), col);
    gui_end_ex(ctx, GUI_TRUE, NULL); }
}
#endif

void gui_end(GuiContext *ctx)
{
    gui_end_ex(ctx, GUI_FALSE, NULL);
}

void gui_end_droppable(GuiContext *ctx, DragDropData *dropdata)
{
    gui_end_ex(ctx, GUI_FALSE, dropdata);
}

void gui_end_ex(GuiContext *ctx, GUI_BOOL make_zero_size, DragDropData *dropdata)
{
    // Initialize outputs
    if (dropdata) {
		GUI_ZERO(*dropdata);
    }

#if 0
    if (ctx->skinning_mode != SkinningMode_none) {
        V2i pos = gui_turtle(ctx)->start_pos;
        V2i size = gui_turtle(ctx)->bounding_max - pos;

        GuiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_ix];
        do_skinning(ctx, turtle->label,
            turtle->start_pos,
            turtle->bounding_max - turtle->start_pos,
            gui_color(ctx, turtle->label, ctx->skin.default_color));

        const V2i c_p = px_to_pt(screen_to_client_pos_impl(ctx->backend, turtle->window_ix, ctx->cursor_pos), ctx->dpi_scale);
        if (c_p[0] >= pos[0] &&
            c_p[1] >= pos[1] &&
            c_p[0] < pos[0] + size[0] &&
            c_p[1] < pos[1] + size[1] &&
            !ctx->dragging) {
            draw_rect(gui_rendering(ctx), pt_to_px(pos, ctx->dpi_scale), pt_to_px(size, ctx->dpi_scale), layout_panel_hover_color);
        }
    }
#endif

    { // Dragging stop logic
        if (    !ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT || // Mouse released
                (ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT && ctx->mouse_scroll)) { // Scrolling when xy dragging
            if (ctx->dragdropdata.tag && dropdata) {
                int pos[2], size[2], c_p[2];
                GUI_ASSIGN_V2(pos, gui_turtle(ctx)->start_pos);
                GUI_V2(size[c] = gui_turtle(ctx)->bounding_max[c] - pos[c]);
                px_to_pt(c_p, ctx->cursor_pos, ctx->dpi_scale);
                if (v2i_in_rect(c_p, pos, size)) {
                    // Release dragdropdata
                    *dropdata = ctx->dragdropdata;
                    GUI_ZERO(ctx->dragdropdata);
                }
            }

            // This doesn't need to be here. Once per frame would be enough.
            ctx->dragging = GUI_FALSE;
        }
    }

#if 0
    { // Draggable resize handle for the panel
        GuiId panel_id = gui_id(gui_turtle(ctx)->label);
        U8 handle_bitfield = get_tbl(GuiId, U8)(&ctx->skin.element_resize_handles, panel_id);
        if (handle_bitfield & GUI_HANDLE_RIGHT_BIT)
            do_resize_handle(ctx, GUI_TRUE);
        if (handle_bitfield & GUI_HANDLE_BOTTOM_BIT)
            do_resize_handle(ctx, GUI_FALSE);
    }
#endif

    GUI_BOOL detached = make_zero_size || gui_turtle(ctx)->detached;

    assert(ctx->turtle_ix > 0);
    --ctx->turtle_ix;

    if (!detached) {
        // Merge bounding boxes
        GuiContext_Turtle *parent = &ctx->turtles[ctx->turtle_ix];
        GuiContext_Turtle *child = &ctx->turtles[ctx->turtle_ix + 1];

        parent->bounding_max[0] = GUI_MAX(parent->bounding_max[0], child->bounding_max[0]);
        parent->bounding_max[1] = GUI_MAX(parent->bounding_max[1], child->bounding_max[1]);
        GUI_ASSIGN_V2(parent->last_bounding_max, child->bounding_max);
    }

    if (ctx->turtle_ix == 0) { // Root panel
        for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
            if (!ctx->windows[i].id)
                continue;
            
            if (!ctx->windows[i].used) {
                // Hide closed windows - don't destroy. Position etc. must be preserved.
                //destroy_window(ctx, i);

                if (ctx->active_win_ix == i) {
                    // Stop interacting with an element in hidden window
                    gui_set_inactive(ctx, ctx->active_id);
                }
            }

            ctx->windows[i].used_in_last_frame = ctx->windows[i].used;
            ctx->windows[i].used = GUI_FALSE;
        }

        ctx->mouse_scroll = 0;
        ctx->last_hot_id = ctx->hot_id;
        ctx->hot_id = 0;
        ctx->written_char_count = 0;
        refresh_framemem(ctx);
    }
}

#if 0
float *skinning_mode_to_comp(Color *col, SkinningMode mode)
{
    switch (mode) {
    case SkinningMode_r: return &col->r;
    case SkinningMode_g: return &col->g;
    case SkinningMode_b: return &col->b;
    case SkinningMode_a: return &col->a;
    default: assert(0);
    }
    return NULL;
}

// Modify skin (do outside of target element)
void do_skinning(GuiContext *ctx, const char *label, V2i pos, V2i size, Color col)
{
    if (ctx->skinning_mode == SkinningMode_none)
        return;

    char buf[1024];
    GUI_FMT_STR(buf, ARRAY_COUNT(buf), "skin_overlay_%s", label);

    ButtonAction action;
    GUI_BOOL down;
    GUI_BOOL hover;
    GUI_BOOL went_down;
    gui_button_logic(ctx, buf, pos, size, &action, &went_down, &down, &hover);

    //if (hover || down)
    //	draw_rect(ctx->rendering, pos, size, layout_hover_color);

    U8 grid_size = get_tbl(GuiId, U8)(&ctx->skin.element_grids, gui_id(label));
    if (ctx->skinning_mode == SkinningMode_pos) {
        if (down && ctx->key_down[KEY_SPACE]) {
            remove_tbl(GuiId, V2i)(&ctx->skin.element_offsets, gui_id(label));
        }
        else {
            if (went_down)
                ctx->drag_start_value = v2i_to_v2f(pos - gui_parent_turtle_start_pos(ctx));

            if (down && ctx->dragging) {
                V2i newpos = v2f_to_v2i(ctx->drag_start_value) + ctx->cursor_pos - ctx->drag_start_pos;
                newpos = rounded_to_grid(newpos, grid_size);
                set_tbl(GuiId, V2i)(&ctx->skin.element_offsets, gui_id(label), newpos);
            }
        }
    }
    if (ctx->skinning_mode == SkinningMode_size) {
        if (down && ctx->key_down[KEY_SPACE]) {
            remove_tbl(GuiId, V2i)(&ctx->skin.element_sizes, gui_id(label));
        }
        else {
            if (went_down)
                ctx->drag_start_value = v2i_to_v2f(size);

            if (down && ctx->dragging) {
                V2i newsize = v2f_to_v2i(ctx->drag_start_value) + ctx->cursor_pos - ctx->drag_start_pos;
                newsize = rounded_to_grid(newsize, grid_size);
                set_tbl(GuiId, V2i)(&ctx->skin.element_sizes, gui_id(label), newsize);
            }
        }
    }
    if (ctx->skinning_mode == SkinningMode_r || ctx->skinning_mode == SkinningMode_g ||
        ctx->skinning_mode == SkinningMode_b || ctx->skinning_mode == SkinningMode_a) {
        if (down && ctx->key_down[KEY_SPACE]) {
            remove_tbl(GuiId, Color)(&ctx->skin.element_colors, gui_id(label));
        }
        else {
            if (went_down)
                ctx->drag_start_value[1] = *skinning_mode_to_comp(&col, ctx->skinning_mode);

            if (down && ctx->dragging) {
                float newcomp = ctx->drag_start_value[1] - (ctx->cursor_pos[1] - ctx->drag_start_pos[1])*0.005f;
                Color newcol = col;
                *skinning_mode_to_comp(&newcol, ctx->skinning_mode) = newcomp;
                char str[128];
                GUI_FMT_STR(str, ARRAY_COUNT(str), "(%.2f, %.2f, %.2f, %.2f)", newcol.r, newcol.g, newcol.b, newcol.a);
                draw_text(gui_rendering(ctx), ctx->cursor_pos + v2i(0, -15), TextAlign_left, str);
                set_tbl(GuiId, Color)(&ctx->skin.element_colors, gui_id(label), newcol);
            }
        }
    }
    if (ctx->skinning_mode >= SkinningMode_grid_1 && ctx->skinning_mode < SkinningMode_grid_last) {
        if (down && ctx->key_down[KEY_SPACE]) {
            remove_tbl(GuiId, U8)(&ctx->skin.element_grids, gui_id(label));
        }
        else {
            U8 grid_sizes[SkinningMode_grid_last - SkinningMode_grid_1] = {
                1, 5, 10, 15, 20
            };
            U8 grid_size = grid_sizes[ctx->skinning_mode - SkinningMode_grid_1];
            if (went_down)
                set_tbl(GuiId, U8)(&ctx->skin.element_grids, gui_id(label), grid_size);
        }
    }
    if (ctx->skinning_mode == SkinningMode_right_handle || ctx->skinning_mode == SkinningMode_bottom_handle) {
        if (down && ctx->key_down[KEY_SPACE]) {
            remove_tbl(GuiId, U8)(&ctx->skin.element_resize_handles, gui_id(label));
        }
        else {
            if (went_down) {
                U8 old = get_tbl(GuiId, U8)(&ctx->skin.element_resize_handles, gui_id(label));
                U8 mask = (ctx->skinning_mode == SkinningMode_right_handle) ? GUI_HANDLE_RIGHT_BIT : GUI_HANDLE_BOTTOM_BIT;
                set_tbl(GuiId, U8)(&ctx->skin.element_resize_handles, gui_id(label), old ^ mask);
            }
        }
    }
}
#endif

void gui_slider_ex(GuiContext *ctx, const char *label, float *value, float min, float max, float handle_rel_size, GUI_BOOL v, int length)
{
    const int scroll_bar_width = GUI_SCROLL_BAR_WIDTH;

    gui_begin(ctx, label);

    int pos[2], size[2];
    gui_turtle_pos(ctx, &pos[0], &pos[1]);
    GUI_ASSIGN_V2(size, gui_zero_v2i);
    size[v] = length;
    size[!v] = scroll_bar_width;

    const int scroll_handle_height = GUI_MAX((int)(handle_rel_size*size[v]), 10);

    GUI_BOOL went_down, down, hover;
    gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

    if (went_down) {
        GUI_DECL_V2(float, tmp, *value, 0);
        gui_start_dragging(ctx, tmp);
    }

    if (down && ctx->dragging) {
        int px_delta = ctx->cursor_pos[v] - ctx->drag_start_pos[v];
        *value = ctx->drag_start_value[0] + 1.f*px_delta / (size[v] - scroll_handle_height) *(max - min);
    }
    *value = GUI_CLAMP(*value, min, max);

    { // Draw
        { // Bg
            int px_pos[2], px_size[2];
            pt_to_px(px_pos, pos, ctx->dpi_scale);
            pt_to_px(px_size, size, ctx->dpi_scale);
            ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1],
                                       GUI_FALSE, GUI_FALSE, gui_layer(ctx), gui_scissor(ctx));
        }

        { // Handle
            float rel_scroll = (*value - min) / (max - min);
            int handle_pos[2];
            GUI_ASSIGN_V2(handle_pos, pos);
            handle_pos[v] += (int)(rel_scroll*(size[v] - scroll_handle_height));
            int handle_size[2];
            GUI_ASSIGN_V2(handle_size, size);
            handle_size[v] = scroll_handle_height;

            int px_pos[2], px_size[2];
            pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
            pt_to_px(px_size, handle_size, ctx->dpi_scale);
            ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1],
                                       down, hover, gui_layer(ctx), gui_scissor(ctx));
        }
    }

    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);

    gui_end(ctx);
}

void gui_begin_frame(GuiContext *ctx, const char *label, int x, int y, int w, int height)
{
    GUI_DECL_V2(int, pos, x, y);
    GUI_DECL_V2(int, size, w, height);

    int frame_handle = -1;
    { // Find/create frame
        int free_handle = -1;
        for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
            if (ctx->frames[i].id == 0 && free_handle == -1)
                free_handle = i;
            if (ctx->frames[i].id == gui_id(label)) {
                frame_handle = i;
                break;
            }
        }
        if (frame_handle < 0) {
            // Create new frame
            assert(free_handle >= 0);
            assert(ctx->frame_count < MAX_GUI_FRAME_COUNT);

            ctx->frames[free_handle].id = gui_id(label);
            ++ctx->frame_count;
            frame_handle = free_handle;
        }
    }
    assert(frame_handle >= 0);
    GuiContext_Frame *frame = &ctx->frames[frame_handle];
    gui_begin(ctx, label);
	gui_turtle(ctx)->detached = GUI_TRUE;
    GUI_ASSIGN_V2(gui_turtle(ctx)->pos, pos);
    GUI_ASSIGN_V2(gui_turtle(ctx)->start_pos, pos);
    gui_turtle(ctx)->frame_ix = frame_handle;

    GuiScissor scissor;
	GUI_ZERO(scissor);
    GUI_ASSIGN_V2(scissor.pos, pos);
    GUI_ASSIGN_V2(scissor.size, size);
    gui_turtle(ctx)->scissor = scissor;

    // Make clicking frame backgound change last active element, so that scrolling works
    gui_button_logic(ctx, label, pos, size, NULL, NULL, NULL, NULL);

    { // Scrolling
        int max_scroll[2];
        GUI_V2(max_scroll[c] = frame->last_bounding_size[c] - size[c]);
        GUI_V2(max_scroll[c] = GUI_MAX(max_scroll[c], 0));

        char scroll_panel_label[MAX_GUI_LABEL_SIZE];
        GUI_FMT_STR(scroll_panel_label, sizeof(scroll_panel_label), "framescrollpanel_%s", label);
        gui_begin(ctx, scroll_panel_label);
		gui_turtle(ctx)->detached = GUI_TRUE; // Detach so that the scroll doesn't take part in window contents size
        gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window
        for (int d = 0; d < 2; ++d) {
            if (size[d] < frame->last_bounding_size[d]) {
                char scroll_label[MAX_GUI_LABEL_SIZE];
                GUI_FMT_STR(scroll_label, sizeof(scroll_label), "framescroll_%i_%s", d, label);
                GUI_ASSIGN_V2(gui_turtle(ctx)->pos, pos);
                gui_turtle(ctx)->pos[!d] += size[!d] - GUI_SCROLL_BAR_WIDTH;

                if (    d == 0 && // Vertical
                        gui_focused(ctx) &&
                        ctx->mouse_scroll != 0 && // Scrolling mouse wheel
                        !(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)) // Not holding ctrl
                    frame->scroll[d] -= ctx->mouse_scroll*64;

                // Scroll by dragging
                if (    gui_turtle(ctx)->window_ix == ctx->active_win_ix &&
                        (ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT) &&
                        (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT)) {
                    if (!ctx->dragging) {
                        float v[2];
                        GUI_V2(v[c] = (float)frame->scroll[c]);
                        gui_start_dragging(ctx, v);
                    } else {
                        int v[2];
                        GUI_V2(v[c] = (int)ctx->drag_start_value[c]);
                        frame->scroll[d] = v[d] + ctx->drag_start_pos[d] - ctx->cursor_pos[d];
                    }
                }

                float scroll = 1.f*frame->scroll[d];
                float rel_shown_area = 1.f*size[d]/frame->last_bounding_size[d];
                float max_comp_scroll = 1.f*max_scroll[d];
                gui_slider_ex(ctx, scroll_label, &scroll, 0, max_comp_scroll, rel_shown_area, !!d, size[d] - GUI_SCROLL_BAR_WIDTH);
                frame->scroll[d] = (int)scroll;
            }
        }
        gui_end(ctx);

        frame->scroll[0] = GUI_CLAMP(frame->scroll[0], 0, max_scroll[0]);
        frame->scroll[1] = GUI_CLAMP(frame->scroll[1], 0, max_scroll[1]);

        // Scroll client area
        int client_start_pos[2];
        GUI_V2(client_start_pos[c] = gui_turtle(ctx)->pos[c] - frame->scroll[c]);
        GUI_ASSIGN_V2(gui_turtle(ctx)->start_pos, client_start_pos);
        GUI_ASSIGN_V2(gui_turtle(ctx)->pos, client_start_pos);
    }
}

void gui_end_frame(GuiContext *ctx)
{
    GUI_V2(gui_frame(ctx)->last_bounding_size[c] = gui_turtle(ctx)->bounding_max[c] - gui_turtle(ctx)->start_pos[c]);
    gui_end(ctx);
}

void gui_set_frame_scroll(GuiContext *ctx, int scroll_x, int scroll_y)
{
    gui_frame(ctx)->scroll[0] = scroll_x;
    gui_frame(ctx)->scroll[1] = scroll_y;
}

void gui_frame_scroll(GuiContext *ctx, int *x, int *y)
{
    *x = gui_frame(ctx)->scroll[0];
    *y = gui_frame(ctx)->scroll[1];
}

void gui_begin_window_ex(GuiContext *ctx, const char *label, int default_size_x, int default_size_y)
{
    GUI_DECL_V2(int, default_size, default_size_x, default_size_y);
    int win_handle = -1;
    { // Find/create window
        int free_handle = -1;
        for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
            if (ctx->windows[i].id == 0 && free_handle == -1)
                free_handle = i;
            if (ctx->windows[i].id == gui_id(label)) {
                win_handle = i;
                break;
            }
        }
        if (win_handle >= 0) {
#if 0
            V2i last_content_size = ctx->windows[win_handle].last_frame_bounding_max;
            V2i win_client_size = ctx->windows[win_handle].client_size;
            if (win_client_size[0] < last_content_size[0] || win_client_size[1] < last_content_size[1]) {
                // Resize window so that content is fully shown
                // @todo Implement minimum and maximum size to gui backend
                default_size[0] = GUI_MAX(default_size[0], MAX(win_client_size[0], last_content_size[0]));
                default_size[1] = GUI_MAX(default_size[1], MAX(win_client_size[1], last_content_size[1]));
                ctx->windows[win_handle].client_size = min_size;
            }
#endif
        } else {
            assert(free_handle >= 0);
            // Create new window
            //GUI_BOOL toplevel_turtle = (ctx->turtle_ix == 0);
            assert(ctx->window_count < MAX_GUI_WINDOW_COUNT);

            ctx->windows[free_handle].id = gui_id(label);
            GUI_ASSIGN_V2(ctx->windows[free_handle].client_size, default_size);
            GUI_ASSIGN_V2(ctx->windows[free_handle].pos, ctx->next_window_pos);
            ctx->window_order[ctx->window_count++] = free_handle;

            win_handle = free_handle;
        }
    }
    assert(win_handle >= 0);
    GuiContext_Window *win = &ctx->windows[win_handle];
    assert(!win->used && "Same window used twice in a frame");
    win->used = GUI_TRUE;
    if (!win->used_in_last_frame)
        ctx->focused_win_ix = win_handle; // Appearing window will be focused

    gui_begin(ctx, label);
	gui_turtle(ctx)->detached = GUI_TRUE;
    gui_turtle(ctx)->window_ix = win_handle;
    gui_turtle(ctx)->layer = 1337 + gui_window_order(ctx, win_handle) * GUI_LAYERS_PER_WINDOW;

    { // Ordinary gui element logic
        int size[2];
        GUI_V2(size[c] = win->client_size[c] + c*GUI_WINDOW_TITLE_BAR_HEIGHT);
        GUI_ASSIGN_V2(win->total_size, size);

        // Dummy window background element. Makes clicking through window impossible.
        char bg_label[MAX_GUI_LABEL_SIZE];
        GUI_FMT_STR(bg_label, sizeof(bg_label), "winbg_%s", label);
        gui_button_logic(ctx, bg_label, win->pos, size, NULL, NULL, NULL, NULL);

        // Title bar logic
        char bar_label[MAX_GUI_LABEL_SIZE];
        GUI_FMT_STR(bar_label, sizeof(bar_label), "winbar_%s", label);
        GUI_BOOL went_down, down, hover;
        GUI_DECL_V2(int, btn_size, size[0], GUI_WINDOW_TITLE_BAR_HEIGHT);
        gui_button_logic(ctx, bar_label, win->pos, btn_size, NULL, &went_down, &down, &hover);

        if (ctx->active_win_ix == win_handle) {
            // Lift window to top
            GUI_BOOL found = GUI_FALSE;
            for (int i = 0; i < ctx->window_count; ++i) {
                if (!found) {
                    if (ctx->window_order[i] == win_handle)
                        found = GUI_TRUE;
                } else {
                    ctx->window_order[i - 1] = ctx->window_order[i];
                }
            }
            ctx->window_order[ctx->window_count - 1] = win_handle;
        }

        if (went_down) {
            float v[2];
            GUI_V2(v[c] = (float)win->pos[c]);
            gui_start_dragging(ctx, v);
        }

        //V2i prev_value = win->pos;
        if (down && ctx->dragging)
            GUI_V2(win->pos[c] = (int)ctx->drag_start_value[c] - ctx->drag_start_pos[c] + ctx->cursor_pos[c]);

        const int margin = 20;
        win->pos[0] = GUI_CLAMP(win->pos[0], margin - size[0], ctx->host_win_size[0] - margin);
        win->pos[1] = GUI_CLAMP(win->pos[1], margin - GUI_WINDOW_TITLE_BAR_HEIGHT, ctx->host_win_size[1] - margin);

        int px_pos[2], px_size[2];
        pt_to_px(px_pos, win->pos, ctx->dpi_scale);
        pt_to_px(px_size, size, ctx->dpi_scale);
        ctx->callbacks.draw_window(ctx->callbacks.user_data,
                                   1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1], GUI_WINDOW_TITLE_BAR_HEIGHT,
                                   gui_label_text(label), gui_focused(ctx), gui_layer(ctx));

        // Turtle to content area
        int content_pos[2];
        GUI_V2(content_pos[c] = win->pos[c] + c*GUI_WINDOW_TITLE_BAR_HEIGHT);
        GUI_ASSIGN_V2(gui_turtle(ctx)->start_pos, content_pos);
        GUI_ASSIGN_V2(gui_turtle(ctx)->pos, content_pos);
    }

    { // Corner resize handle
        char resize_label[MAX_GUI_LABEL_SIZE];
        GUI_FMT_STR(resize_label, sizeof(resize_label), "winresize_%s", label);
        gui_begin(ctx, resize_label);
		gui_turtle(ctx)->detached = GUI_TRUE; // Detach so that the handle doesn't take part in window contents size
        gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window

        int handle_size[2] = {GUI_SCROLL_BAR_WIDTH, GUI_SCROLL_BAR_WIDTH};
        int handle_pos[2];
        GUI_V2(handle_pos[c] = win->pos[c] + win->total_size[c] - handle_size[c]);
        GUI_BOOL went_down, down, hover;
        gui_button_logic(ctx, resize_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

        if (went_down) {
            float v[2];
            GUI_V2(v[c] = (float)win->client_size[c]);
            gui_start_dragging(ctx, v);
        }

        if (down)
            GUI_V2(win->client_size[c] = (int)ctx->drag_start_value[c] + ctx->cursor_pos[c] - ctx->drag_start_pos[c]);
        GUI_V2(win->client_size[c] = GUI_MAX(win->client_size[c], 40));

        int px_pos[2], px_size[2];
        pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
        pt_to_px(px_size, handle_size, ctx->dpi_scale);
        ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1],
                                   down, hover, gui_layer(ctx), gui_scissor(ctx));

        gui_end(ctx);
    }

    gui_begin_frame(ctx, label, win->pos[0], win->pos[1] + GUI_WINDOW_TITLE_BAR_HEIGHT, win->client_size[0], win->client_size[1]);

#if 0
    begin_frame(gui_rendering(ctx), win_client_size_impl(ctx->backend, win_handle), ctx->dpi_scale);
    if (BUILD == BUILD_DEBUG || BUILD == BUILD_DEV) {
        ctx->skinning_mode = SkinningMode_none;

        if (ctx->key_down['a'])
            ctx->skinning_mode = SkinningMode_pos;
        else if (ctx->key_down['s'])
            ctx->skinning_mode = SkinningMode_size;
        else if (ctx->key_down['z'])
            ctx->skinning_mode = SkinningMode_r;
        else if (ctx->key_down['x'])
            ctx->skinning_mode = SkinningMode_g;
        else if (ctx->key_down['c'])
            ctx->skinning_mode = SkinningMode_b;
        else if (ctx->key_down['v'])
            ctx->skinning_mode = SkinningMode_a;
        else if (ctx->key_down['n'])
            ctx->skinning_mode = SkinningMode_right_handle;
        else if (ctx->key_down['m'])
            ctx->skinning_mode = SkinningMode_bottom_handle;

        if (ctx->key_pressed['p'])
            save_skin(&ctx->skin, ctx->skin_source_path);

        for (int i = 0; i < SkinningMode_grid_last - SkinningMode_grid_1; ++i) {
            U32 key_code = KEY_1 + i;
            if (ctx->key_down[key_code])
                ctx->skinning_mode = (SkinningMode)((int)SkinningMode_grid_1 + i);
        }

    }
#endif
}

void gui_end_window_ex(GuiContext *ctx)
{
    gui_end_frame(ctx);
    gui_end(ctx);
}

void gui_begin_window(GuiContext *ctx, const char *label, int default_size_x, int default_size_y)
{
    gui_begin_window_ex(ctx, label, default_size_x, default_size_y);
}

void gui_end_window(GuiContext *ctx)
{
    gui_end_window_ex(ctx);
}

void gui_window_client_size(GuiContext *ctx, int *w, int *h)
{
    *w = gui_window(ctx)->client_size[0];
    *h = gui_window(ctx)->client_size[1];
}

void gui_begin_contextmenu(GuiContext *ctx, const char *label)
{
    gui_begin_window_ex(ctx, label, 10, 10);
}

void gui_end_contextmenu(GuiContext *ctx)
{
    gui_end_window_ex(ctx);
}

GUI_BOOL gui_contextmenu_item(GuiContext *ctx, const char *label)
{
    GUI_BOOL ret = gui_button(ctx, label);
    gui_next_row(ctx);
    return ret;
}

void gui_begin_dragdrop_src(GuiContext *ctx, DragDropData data)
{
    assert(gui_turtle(ctx)->inactive_dragdropdata.tag == NULL && "Nested dragdrop areas in single gui element not supported");
    gui_turtle(ctx)->inactive_dragdropdata = data;
}

void gui_end_dragdrop_src(GuiContext *ctx)
{
    assert(gui_turtle(ctx)->inactive_dragdropdata.tag);
    GUI_ZERO(gui_turtle(ctx)->inactive_dragdropdata);
}

GUI_BOOL gui_knob(GuiContext *ctx, const char *label, float min, float max, float *value, const char *value_str)
{
    GUI_BOOL ret = GUI_FALSE;
    gui_begin(ctx, label);

    int pos[2], size[2];
    gui_turtle_pos(ctx, &pos[0], &pos[1]);
    int def_size[2] = {32, 32};
    gui_size(ctx, size, label, def_size);


    //if (ctx->skinning_mode == SkinningMode_none) {
        GUI_BOOL went_down;
        GUI_BOOL down;
        GUI_BOOL hover;
        gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

        if (went_down)
            ctx->drag_start_value[0] = *value;

        if (down && ctx->dragging) {
            *value = ctx->drag_start_value[0] + (max - min) / 100.0f*(ctx->drag_start_pos[1] - ctx->cursor_pos[1]);
            *value = GUI_CLAMP(*value, min, max);

            { // Show value
                const char *str = value_str;
                char buf[128];
                if (!str) {
                    GUI_FMT_STR(buf, sizeof(buf), "%.3f", *value);
                    str = buf;
                }
            }
        }
        ret = down;
    //}

    //float d = ctx->dpi_scale;
    //V2i center = pos + size / 2;
    //V2i knob_center = v2i(center[0], pos[1] + size[0] / 2);
    //float rad = size[0] / 2.0f;
    //Color auxillary = ctx->skin.default_color;
    //auxillary.a *= 0.4f;
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad - ctx->skin.knob_arc_width, d) - 3.0f, 2, 4, auxillary);
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad, d),
    //    pt_to_px_f32(ctx->skin.knob_arc_width, d), pt_to_px_f32(ctx->skin.knob_bloom_width, d),
    //    gui_color(ctx, label, ctx->skin.default_color),
    //    TAU * 3 / 8, TAU * 3 / 4 * phase_f32(*value, min, max));
    //draw_text(gui_rendering(ctx), pt_to_px(center + v2i(0, size[1] / 2 - 14), d), TextAlign_center, gui_label_text(label), ctx->skin.default_color);
    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
    gui_end(ctx);

    return ret;
}

void gui_label(GuiContext *ctx, const char *label)
{
    gui_begin(ctx, label);
    int pos[2];
    gui_turtle_pos(ctx, &pos[0], &pos[1]);
    //V2i size = draw_text(gui_rendering(ctx),
    //    pt_to_px(pos, ctx->dpi_scale),
    //    TextAlign_left,
    //    gui_label_text(label),
    //    gui_color(ctx, label, ctx->skin.default_color));
    int size[2] = {30, 30}; // @todo
    gui_set_turtle_pos(ctx, pos[0], pos[1]);
    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
    gui_end(ctx);
}

GUI_BOOL gui_button_ex(GuiContext *ctx, const char *label, GUI_BOOL force_down)
{
    gui_begin(ctx, label);
    int margin[2] = {5, 3};
    int pos[2], size[2];
    GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
    int def_size[2] = {50, 21};
    gui_size(ctx, size, label, def_size); // @todo Minimum size to skin

    // @todo Recalc size only when text changes
    float text_size[2];
    ctx->callbacks.calc_text_size(text_size, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
    GUI_V2(size[c] = GUI_MAX((int)text_size[c] + margin[c] * 2, size[c]));

    GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
    if (gui_is_inside_window(ctx, size))
    {
        gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

        int px_pos[2], px_size[2];
        pt_to_px(px_pos, pos, ctx->dpi_scale);
        pt_to_px(px_size, size, ctx->dpi_scale);
        ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1], down || force_down, hover, gui_layer(ctx), gui_scissor(ctx));

        int px_margin[2];
        pt_to_px(px_margin, margin, ctx->dpi_scale);
        ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos[0] + px_margin[0], 1.f*px_pos[1] + px_margin[1], gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
    }

    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
    gui_end(ctx);

    gui_next_row(ctx);

    return went_up && hover;
}

GUI_BOOL gui_button(GuiContext *ctx, const char *label)
{ return gui_button_ex(ctx, label, GUI_FALSE); }

GUI_BOOL gui_selectable(GuiContext *ctx, const char *label, GUI_BOOL selected)
{ return gui_button_ex(ctx, label, selected); }

GUI_BOOL gui_checkbox_ex(GuiContext *ctx, const char *label, GUI_BOOL *value, GUI_BOOL radio_button_visual)
{
    gui_begin(ctx, label);
    int margin[2] = {5, 3};
    int pos[2], size[2];
    GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
    int def_size[2] = {20, 20};
    gui_size(ctx, size, label, def_size); // @todo Minimum size to skin

    float text_size[2];
    ctx->callbacks.calc_text_size(text_size, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
    GUI_V2(size[c] = GUI_MAX((int)text_size[c] + margin[c] * 2, size[c]));

    GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
    if (gui_is_inside_window(ctx, size))
    {
        int px_margin[2];
        pt_to_px(px_margin, margin, ctx->dpi_scale);
        int box_width = size[1];
        GUI_DECL_V2(int, tmp, 0, box_width);
        pt_to_px(tmp, tmp, ctx->dpi_scale);
        float px_box_width = tmp[1] - px_margin[1]*2.f;

        size[0] += box_width + margin[0];

        gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

        int px_pos[2];
        pt_to_px(px_pos, pos, ctx->dpi_scale);
        //V2i px_size = pt_to_px(size, ctx->dpi_scale);

        float x = 1.f*px_pos[0] + px_margin[0];
        float y = 1.f*px_pos[1] + px_margin[0];
        float w = 1.f*px_box_width;
        if (radio_button_visual)
            ctx->callbacks.draw_radiobutton(ctx->callbacks.user_data, x, y, w, *value, down, hover, gui_layer(ctx), gui_scissor(ctx));
        else
            ctx->callbacks.draw_checkbox(ctx->callbacks.user_data, x, y, w, *value, down, hover, gui_layer(ctx), gui_scissor(ctx));
        ctx->callbacks.draw_text(ctx->callbacks.user_data, px_pos[0] + px_box_width + 2.f*px_margin[0], 1.f*px_pos[1] + px_margin[1],
                                 gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
    }

    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
    gui_end(ctx);

    gui_next_row(ctx); // @todo Layouting

    if (value && went_up && hover)
        *value = !*value;
    return went_up && hover;
}

GUI_BOOL gui_checkbox(GuiContext *ctx, const char *label, GUI_BOOL *value)
{ return gui_checkbox_ex(ctx, label, value, GUI_FALSE); }

GUI_BOOL gui_radiobutton(GuiContext *ctx, const char *label, GUI_BOOL value)
{
    GUI_BOOL v = value;
    return gui_checkbox_ex(ctx, label, &v, GUI_TRUE); // @todo Proper radiobutton
}

void gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max)
{
    gui_slider_ex(ctx, label, value, min, max, 0.1f, GUI_FALSE, GUI_MAX(100, gui_frame(ctx)->last_bounding_size[0]));
    gui_next_row(ctx); // @todo Layouting
}

GUI_BOOL gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size)
{
    GUI_BOOL content_changed = GUI_FALSE;

    gui_begin(ctx, label);
    int margin[2] = {5, 3};
    int pos[2], size[2];
    GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
    int def_size[2] = {300, 21};
    gui_size(ctx, size, label, def_size); // @todo Minimum size to skin
    int box_size[2];
    GUI_ASSIGN_V2(box_size, size);
    int label_size[2] = {0, 0};
    GUI_BOOL has_label = (strlen(gui_label_text(label)) > 0);
    if (has_label) {
        float label_size_f[2];
        ctx->callbacks.calc_text_size(label_size_f, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
        GUI_V2(label_size[c] = (int)label_size_f[c] + margin[c]*2);

        size[0] += label_size[0];
        box_size[0] -= label_size[0];
    }

    GUI_BOOL went_down = GUI_FALSE, hover = GUI_FALSE;
    if (gui_is_inside_window(ctx, size)) {
        gui_button_logic(ctx, label, pos, size, NULL, &went_down, NULL, &hover);
        GUI_BOOL active = (ctx->last_active_id == gui_id(label));

        if (active) {
            assert(buf && buf_size > 0);
            int char_count = (int)strlen(buf);
            for (int i = 0; i < ctx->written_char_count; ++i) {
                if (char_count >= buf_size)
                    break;
                char ch = ctx->written_text_buf[i];
                if (ch == '\b') {
                    if (char_count > 0)
                        buf[--char_count] = '\0';
                } else {
                    buf[char_count++] = ch;
                }
                content_changed = GUI_TRUE;
            }
            char_count = GUI_MIN(char_count, buf_size - 1);
            buf[char_count] = '\0';
        }

        int px_margin[2];
        pt_to_px(px_margin, margin, ctx->dpi_scale);
        if (has_label) { // Draw label
            int px_pos[2];
            pt_to_px(px_pos, pos, ctx->dpi_scale);
            ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos[0] + px_margin[0], 1.f*px_pos[1], gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
        }

        { // Draw textbox
            GUI_DECL_V2(int, pt_pos, pos[0] + label_size[0], pos[1]);
            int px_pos[2], px_size[2];
            pt_to_px(px_pos, pt_pos, ctx->dpi_scale);
            pt_to_px(px_size, box_size, ctx->dpi_scale);
            // @todo down --> active
            ctx->callbacks.draw_textbox(ctx->callbacks.user_data, 1.f*px_pos[0], 1.f*px_pos[1], 1.f*px_size[0], 1.f*px_size[1], active, hover, gui_layer(ctx), gui_scissor(ctx));

            ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos[0] + px_margin[0], 1.f*px_pos[1] + px_margin[1], buf, gui_layer(ctx), gui_scissor(ctx));
        }

    }

    gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
    gui_end(ctx);

    gui_next_row(ctx); // @todo Layouting

    return content_changed;
}

void gui_begin_listbox(GuiContext *ctx, const char *label)
{
    gui_begin(ctx, label);
    // @todo Clipping and scrollbar
}

void gui_end_listbox(GuiContext *ctx)
{
    gui_end(ctx);

    gui_next_row(ctx); // @todo Layouting
}

void gui_next_row(GuiContext *ctx)
{
    gui_set_turtle_pos(ctx, gui_turtle(ctx)->pos[0], gui_turtle(ctx)->last_bounding_max[1]);
}

void gui_next_col(GuiContext *ctx)
{
    gui_set_turtle_pos(ctx, gui_turtle(ctx)->last_bounding_max[0], gui_turtle(ctx)->pos[1]);
}

void gui_enlarge_bounding(GuiContext *ctx, int x, int y)
{
	GUI_DECL_V2(int, pos, x, y);

    GuiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_ix];

    GUI_V2(turtle->bounding_max[c] = GUI_MAX(turtle->bounding_max[c], pos[c]));
    GUI_ASSIGN_V2(turtle->last_bounding_max, pos);
}

void gui_ver_space(GuiContext *ctx)
{
    int pos[2];
    GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
    gui_enlarge_bounding(ctx, pos[0] + 25, pos[1]);
    gui_next_col(ctx);
}

void gui_hor_space(GuiContext *ctx)
{
    int pos[2];
    GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
    gui_enlarge_bounding(ctx, pos[0], pos[1] + 25);
    gui_next_row(ctx);
}

