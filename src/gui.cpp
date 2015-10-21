#include "gui.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>

static void *frame_alloc(GuiContext *ctx, int size);

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

#   define FMT_STR sprintf_impl
#   define V_FMT_STR v_sprintf_impl
#else
#   define FMT_STR snprintf
#   define V_FMT_STR vsnprintf
#endif

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(*x))
#define CLAMP(v, a, b) (MIN(MAX((v), (a)), (b)))
#define ABS(x) ((x) < 0 ? (-x) : x)
#define GUI_WINDOW_TITLE_BAR_HEIGHT 25
#define GUI_SCROLL_BAR_WIDTH 15
#define GUI_LAYERS_PER_WINDOW 10000 // Maybe 10k layers inside a window is enough
#define SELECT_COMP(v, h) ((h) ? ((v).x) : ((v).y))

#define GUI_NONE_WINDOW_IX (-2)
#define GUI_BG_WINDOW_IX (-1)

static void *check_ptr(void *ptr)
{
    if (!ptr) {
        abort();
    }
    return ptr;
}

V2i v2i(int x, int y)
{
    V2i v = { x, y };
    return v;
}
V2i operator+(V2i a, V2i b)
{ return v2i(a.x + b.x, a.y + b.y); }
V2i operator-(V2i a, V2i b)
{ return v2i(a.x - b.x, a.y - b.y); }
V2i operator*(V2i a, V2i b)
{ return v2i(a.x * b.x, a.y * b.y); }
V2i operator*(V2i a, int m)
{ return v2i(a.x * m, a.y * m); }
V2i operator/(V2i v, int d)
{ return v2i(v.x / d, v.y / d); }
bool operator==(V2i a, V2i b)
{ return a.x == b.x && a.y == b.y; }
bool operator!=(V2i a, V2i b)
{ return a.x != b.x || a.y != b.y; }
V2i rounded_to_grid(V2i v, int grid)
{ return v2i((v.x + grid / 2) / grid, (v.y + grid / 2) / grid)*grid; }

V2f v2f(float x, float y)
{
    V2f v = { x, y };
    return v;
}
V2f operator+(V2f a, V2f b)
{ return v2f(a.x + b.x, a.y + b.y); }
V2f operator*(V2f a, float m)
{ return v2f(a.x*m, a.y*m); }

V2f v2i_to_v2f(V2i v)
{ return v2f((float)v.x, (float)v.y); }
V2i v2f_to_v2i(V2f v)
{ return v2i((int)v.x, (int)v.y); }
bool v2i_in_rect(V2i v, V2i pos, V2i size)
{ return v.x >= pos.x && v.y >= pos.y && v.x < pos.x + size.x && v.y < pos.y + size.y; }

void destroy_window(GuiContext *ctx, int handle)
{
    bool found = false;
    for (int i = 0; i < ctx->window_count; ++i) {
        if (!found) {
            if (ctx->window_order[i] == handle)
                found = true;
        } else {
            ctx->window_order[i - 1] = ctx->window_order[i];
        }
    }
    --ctx->window_count;

    GuiContext_Window *win = &ctx->windows[handle];

    GuiContext_Window zero_win = {};
    *win = zero_win;
}

int gui_window_order(GuiContext *ctx, int handle)
{
    for (int i = 0; i < ctx->window_count; ++i) {
        if (ctx->window_order[i] == handle)
            return i;
    }
    return -1;
}

V2i pt_to_px(V2i pt, float dpi_scale)
{
    return v2i((int)floorf(pt.x*dpi_scale + 0.5f), (int)floorf(pt.y*dpi_scale + 0.5f));
}

float pt_to_px_f32(float pt, float dpi_scale)
{
    return pt*dpi_scale;
}

V2i px_to_pt(V2i p, float dpi_scale)
{
    return v2i((int)floorf(p.x / dpi_scale + 0.5f), (int)floorf(p.y / dpi_scale + 0.5f));
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
    fprintf(file, "	s.knob_size = v2i(%i, %i);\n", s->knob_size.x, s->knob_size.y);
    fprintf(file, "	s.default_color = color(%ff, %ff, %ff, %ff);\n", s->default_color.r, s->default_color.g, s->default_color.b, s->default_color.a);
    fprintf(file, "\n");

    for (U32 i = 0; i < s->element_offsets.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_offsets.array_data[i];
        if (entry.key == s->element_offsets.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_offsets, %u, v2i(%i, %i));\n",
            entry.key, entry.value.x, entry.value.y);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_sizes.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_sizes.array_data[i];
        if (entry.key == s->element_sizes.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_sizes, %u, v2i(%i, %i));\n",
            entry.key, entry.value.x, entry.value.y);
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
    int size = (int)V_FMT_STR(NULL, 0, fmt, args) + 1;
    text = (char*)frame_alloc(ctx, size);
    V_FMT_STR(text, size, fmt, args_copy);
    va_end(args_copy);
    va_end(args);
    return text;
}

void gui_set_turtle_pos(GuiContext *ctx, V2i pos)
{
    ctx->turtles[ctx->turtle_ix].pos = pos;
    gui_enlarge_bounding(ctx, pos);
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
bool gui_focused(GuiContext *ctx)
{
    if (ctx->window_count == 0)
        return true;
    return gui_turtle(ctx)->window_ix == ctx->focused_win_ix;
}

V2i gui_turtle_pos(GuiContext *ctx) { return gui_turtle(ctx)->pos; }
V2i gui_parent_turtle_start_pos(GuiContext *ctx)
{
    if (ctx->turtle_ix == 0)
        return v2i(0, 0);
    return ctx->turtles[ctx->turtle_ix - 1].start_pos;
}

GuiScissor *gui_scissor(GuiContext *ctx)
{
    GuiScissor *s = &gui_turtle(ctx)->scissor;
    return s->size.x == 0 ? NULL : s;
}

// Returns whether the current turtle with certain size is at least partially visible in the client area of the current window
bool gui_is_inside_window(GuiContext *ctx, V2i size)
{
    // TODO: Take size into account
    V2i pos = gui_turtle_pos(ctx);
    GuiContext_Window *win = gui_window(ctx);
    if (pos.x + size.x <= win->pos.x || pos.y + size.x <= win->pos.y + GUI_WINDOW_TITLE_BAR_HEIGHT)
        return false;
    if (pos.x >= win->pos.x + win->total_size.x || pos.y >= win->pos.y + win->total_size.y)
        return false;
    return true;
}

void gui_start_dragging(GuiContext *ctx, V2f start_value)
{
    assert(!ctx->dragging);
    ctx->dragging = true;
    ctx->drag_start_pos = ctx->cursor_pos;
    ctx->drag_start_value = start_value;
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

// Call _inside_ target element
V2i gui_pos(GuiContext *, const char *, V2i pos)
{
    //V2i override_offset = get_tbl(GuiId, V2i)(&ctx->skin.element_offsets, gui_id(label));
    //if (override_offset == ctx->skin.element_offsets.null_value)
        return pos;
    //return gui_parent_turtle_start_pos(ctx) + override_offset;
}
V2i gui_size(GuiContext *, const char *, V2i size)
{
    //V2i override_size = get_tbl(GuiId, V2i)(&ctx->skin.element_sizes, gui_id(label));
    //if (override_size == ctx->skin.element_sizes.null_value)
        return size;
    //return override_size;
}
//Color gui_color(GuiContext *ctx, const char *label, Color col)
//{
    //Color override_col = get_tbl(GuiId, Color)(&ctx->skin.element_colors, gui_id(label));
    //if (override_col == ctx->skin.element_colors.null_value)
        //return col;
    //return override_col;
//}

static void *frame_alloc(GuiContext *ctx, int size)
{
    assert(ctx->framemem_bucket_count >= 1);
    GuiContext_MemBucket *bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count - 1];
    if (bucket->used + size > bucket->size) {
        // Need a new bucket :(
        int new_bucket_count = ctx->framemem_bucket_count + 1;
        ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)*new_bucket_count));

        int bucket_size = MAX(size, bucket->size * 2);
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
        bool set_hot = false;
        if (ctx->hot_id == 0) {
            set_hot = true;
        } else {
            // Last overlapping element of the topmost window gets to be hot
            if (ctx->hot_layer <= gui_turtle(ctx)->layer)
                set_hot = true;
        }
        if (set_hot) {
            ctx->hot_id = gui_id(label);
            ctx->hot_layer = gui_turtle(ctx)->layer;
        }
    }
}

bool gui_is_hot(GuiContext *ctx, const char *label)
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

bool gui_is_active(GuiContext *ctx, const char *label)
{
    return ctx->active_id == gui_id(label);
}

void gui_button_logic(GuiContext *ctx, const char *label, V2i pos, V2i size, bool *went_up, bool *went_down, bool *down, bool *hover)
{
    if (went_up) *went_up = false;
    if (went_down) *went_down = false;
    if (down) *down = false;
    if (hover) *hover = false;

    bool was_released = false;
    if (gui_is_active(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) {
            if (down) *down = true;
        }
        else if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_RELEASED_BIT) {
            if (went_up) *went_up = true;
            gui_set_inactive(ctx, gui_id(label));
            was_released = true;
        }
    }
    else if (gui_is_hot(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_PRESSED_BIT) {
            if (down) *down = true;
            if (went_down) *went_down = true;
            gui_set_active(ctx, label);
        }
    }

    const V2i c_p = px_to_pt(ctx->cursor_pos, ctx->dpi_scale);
    GuiScissor *s = gui_scissor(ctx);
    if (    v2i_in_rect(c_p, pos, size) &&
            (!s || v2i_in_rect(c_p, s->pos, s->size))) {
        gui_set_hot(ctx, label);
        if (hover && (gui_is_hot(ctx, label) || gui_is_active(ctx, label) || was_released)) *hover = true;
    }
}

//void do_skinning(GuiContext *ctx, const char *label, V2i pos, V2i size, Color col);

void gui_begin(GuiContext *ctx, const char *label, bool detached)
{
    assert(ctx->turtle_ix < MAX_GUI_STACK_SIZE);
    if (ctx->turtle_ix >= MAX_GUI_STACK_SIZE)
        ctx->turtle_ix = 0; // Failsafe

    GuiContext_Turtle *prev = &ctx->turtles[ctx->turtle_ix];
    GuiContext_Turtle *cur = &ctx->turtles[++ctx->turtle_ix];

    GuiContext_Turtle new_turtle = {};
    new_turtle.pos = detached ? v2i(0, 0) : gui_pos(ctx, label, prev->pos);
    new_turtle.start_pos = new_turtle.pos;
    new_turtle.bounding_max = new_turtle.pos;
    new_turtle.last_bounding_max = new_turtle.pos;
    new_turtle.frame_ix = prev->frame_ix;
    new_turtle.window_ix = prev->window_ix;
    new_turtle.layer = prev->layer + 1;
    new_turtle.detached = detached;
    new_turtle.inactive_dragdropdata = prev->inactive_dragdropdata;
    new_turtle.scissor = prev->scissor;
    FMT_STR(new_turtle.label, MAX_GUI_LABEL_SIZE, "%s", label);
    *cur = new_turtle;

    V2i size = gui_size(ctx, label, v2i(-1, -1));
    if (size != v2i(-1, -1)) {
        // This panel has predetermined (minimum) size
        gui_turtle(ctx)->bounding_max = gui_turtle(ctx)->start_pos + size;
    }
}

#define GUI_HANDLE_RIGHT_BIT 0x01
#define GUI_HANDLE_BOTTOM_BIT 0x02

#if 0
// Resize handle for parent element
void do_resize_handle(GuiContext *ctx, bool right_handle)
{
    V2i panel_size = gui_turtle(ctx)->bounding_max - gui_turtle(ctx)->start_pos;
    //GuiId panel_id = gui_id(gui_turtle(ctx)->label);

    char handle_label[MAX_GUI_LABEL_SIZE];
    FMT_STR(handle_label, ARRAY_COUNT(handle_label), "%ihandle_%s", right_handle ? 0 : 1, gui_turtle(ctx)->label);
    { gui_begin(ctx, handle_label);
    V2i handle_pos;
    V2i handle_size;
    if (right_handle) {
        handle_size = gui_size(ctx, handle_label, v2i(5, 20));
        handle_pos = gui_turtle(ctx)->pos + v2i(panel_size.x - handle_size.x / 2, panel_size.y / 2 - handle_size.y / 2);
    }
    else {
        handle_size = gui_size(ctx, handle_label, v2i(20, 5));
        handle_pos = gui_turtle(ctx)->pos + v2i(panel_size.x / 2 - handle_size.x / 2, panel_size.y - handle_size.y / 2);
    }

    bool went_down;
    bool down;
    bool hover;
    gui_button_logic(ctx, handle_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

    if (went_down)
        ctx->drag_start_value = v2i_to_v2f(panel_size);

    if (down && ctx->dragging) {
        V2i new_size = v2f_to_v2i(ctx->drag_start_value) + px_to_pt(ctx->cursor_pos - ctx->drag_start_pos, ctx->dpi_scale);
        new_size.x = MAX(new_size.x, 10);
        new_size.y = MAX(new_size.y, 10);
        if (right_handle)
            new_size.y = panel_size.y;
        else
            new_size.x = panel_size.x;
        // @todo Resizing by user should maybe belong to somewhere else than skin?
        set_tbl(GuiId, V2i)(&ctx->skin.element_sizes, panel_id, new_size);
    }

    // @todo Change cursor when hovering

    //Color col = ctx->skin.default_color;
    //col.a *= 0.5f; // @todo Color to skin
    //draw_rect(gui_rendering(ctx), pt_to_px(handle_pos, ctx->dpi_scale), pt_to_px(handle_size, ctx->dpi_scale), col);
    gui_end_ex(ctx, true, NULL); }
}
#endif

void gui_end(GuiContext *ctx)
{
    gui_end_ex(ctx, false, NULL);
}

void gui_end_droppable(GuiContext *ctx, DragDropData *dropdata)
{
    gui_end_ex(ctx, false, dropdata);
}

void gui_end_ex(GuiContext *ctx, bool make_zero_size, DragDropData *dropdata)
{
    // Initialize outputs
    if (dropdata) {
        DragDropData empty = {};
        *dropdata = empty;
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
        if (c_p.x >= pos.x &&
            c_p.y >= pos.y &&
            c_p.x < pos.x + size.x &&
            c_p.y < pos.y + size.y &&
            !ctx->dragging) {
            draw_rect(gui_rendering(ctx), pt_to_px(pos, ctx->dpi_scale), pt_to_px(size, ctx->dpi_scale), layout_panel_hover_color);
        }
    }
#endif

    { // Dragging stop logic
        if (!ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) {
            if (ctx->dragdropdata.tag && dropdata) {
                V2i pos = gui_turtle(ctx)->start_pos;
                V2i size = gui_turtle(ctx)->bounding_max - pos;
                const V2i c_p = px_to_pt(ctx->cursor_pos, ctx->dpi_scale);
                if (v2i_in_rect(c_p, pos, size)) {
                    // Release dragdropdata
                    *dropdata = ctx->dragdropdata;
                    DragDropData empty = {};
                    ctx->dragdropdata = empty;
                }
            }

            // This doesn't need to be here. Once per frame would be enough.
            ctx->dragging = false;
        }
    }

#if 0
    { // Draggable resize handle for the panel
        GuiId panel_id = gui_id(gui_turtle(ctx)->label);
        U8 handle_bitfield = get_tbl(GuiId, U8)(&ctx->skin.element_resize_handles, panel_id);
        if (handle_bitfield & GUI_HANDLE_RIGHT_BIT)
            do_resize_handle(ctx, true);
        if (handle_bitfield & GUI_HANDLE_BOTTOM_BIT)
            do_resize_handle(ctx, false);
    }
#endif

    bool detached = make_zero_size || gui_turtle(ctx)->detached;

    assert(ctx->turtle_ix > 0);
    --ctx->turtle_ix;

    if (!detached) {
        // Merge bounding boxes
        GuiContext_Turtle *parent = &ctx->turtles[ctx->turtle_ix];
        GuiContext_Turtle *child = &ctx->turtles[ctx->turtle_ix + 1];

        parent->bounding_max.x = MAX(parent->bounding_max.x, child->bounding_max.x);
        parent->bounding_max.y = MAX(parent->bounding_max.y, child->bounding_max.y);
        parent->last_bounding_max = child->bounding_max;
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
            ctx->windows[i].used = false;
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
    FMT_STR(buf, ARRAY_COUNT(buf), "skin_overlay_%s", label);

    ButtonAction action;
    bool down;
    bool hover;
    bool went_down;
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
                ctx->drag_start_value.y = *skinning_mode_to_comp(&col, ctx->skinning_mode);

            if (down && ctx->dragging) {
                float newcomp = ctx->drag_start_value.y - (ctx->cursor_pos.y - ctx->drag_start_pos.y)*0.005f;
                Color newcol = col;
                *skinning_mode_to_comp(&newcol, ctx->skinning_mode) = newcomp;
                char str[128];
                FMT_STR(str, ARRAY_COUNT(str), "(%.2f, %.2f, %.2f, %.2f)", newcol.r, newcol.g, newcol.b, newcol.a);
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

void gui_slider_ex(GuiContext *ctx, const char *label, float *value, float min, float max, float handle_rel_size, bool h, int length)
{
    const int scroll_bar_width = GUI_SCROLL_BAR_WIDTH;

    gui_begin(ctx, label);

    V2i pos = gui_turtle_pos(ctx);
    V2i size = v2i(0, 0);
    SELECT_COMP(size, h) = length;
    SELECT_COMP(size, !h) = scroll_bar_width;

    const int scroll_handle_height = MAX((int)(handle_rel_size*SELECT_COMP(size, h)), 10);

    bool went_down, down, hover;
    gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

    if (went_down)
        gui_start_dragging(ctx, v2f(*value, 0));

    if (down && ctx->dragging) {
        int px_delta = SELECT_COMP(ctx->cursor_pos, h) - SELECT_COMP(ctx->drag_start_pos, h);
        *value = ctx->drag_start_value.x + 1.f*px_delta / (SELECT_COMP(size, h) - scroll_handle_height) *(max - min);
    }
    *value = CLAMP(*value, min, max);

    { // Draw
        { // Bg
            V2i px_pos = pt_to_px(pos, ctx->dpi_scale);
            V2i px_size = pt_to_px(size, ctx->dpi_scale);
            ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y,
                                       false, false, gui_layer(ctx), gui_scissor(ctx));
        }

        { // Handle
            float rel_scroll = (*value - min) / (max - min);
            V2i handle_pos = pos;
            SELECT_COMP(handle_pos, h) += (int)(rel_scroll*(SELECT_COMP(size, h) - scroll_handle_height));
            V2i handle_size = size;
            SELECT_COMP(handle_size, h) = scroll_handle_height;

            V2i px_pos = pt_to_px(handle_pos, ctx->dpi_scale);
            V2i px_size = pt_to_px(handle_size, ctx->dpi_scale);
            ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y,
                                       down, hover, gui_layer(ctx), gui_scissor(ctx));
        }
    }

    gui_enlarge_bounding(ctx, pos + size);

    gui_end(ctx);
}

void gui_begin_frame(GuiContext *ctx, const char *label, V2i pos, V2i size)
{
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
    gui_begin(ctx, label, true);
    gui_turtle(ctx)->pos = pos;
    gui_turtle(ctx)->start_pos = pos;
    gui_turtle(ctx)->frame_ix = frame_handle;

    GuiScissor scissor = {};
    scissor.pos.x = pos.x;
    scissor.pos.y = pos.y;
    scissor.size.x = size.x;
    scissor.size.y = size.y;
    gui_turtle(ctx)->scissor = scissor;

    // Make clicking frame backgound change last active element, so that scrolling works
    gui_button_logic(ctx, label, pos, size, NULL, NULL, NULL, NULL);

    { // Scrolling
        V2i max_scroll = frame->last_bounding_size - size;

        char scroll_panel_label[MAX_GUI_LABEL_SIZE];
        FMT_STR(scroll_panel_label, ARRAY_COUNT(scroll_panel_label), "framescrollpanel_%s", label);
        gui_begin(ctx, scroll_panel_label, true); // Detach so that the scroll doesn't take part in window contents size
        gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window
        for (int h = 0; h < 2; ++h) {
            if (SELECT_COMP(size, h) < SELECT_COMP(frame->last_bounding_size, h)) {
                char scroll_label[MAX_GUI_LABEL_SIZE];
                FMT_STR(scroll_label, ARRAY_COUNT(scroll_label), "framescroll_%i_%s", h, label);
                gui_turtle(ctx)->pos = pos;
                SELECT_COMP(gui_turtle(ctx)->pos, !h) += SELECT_COMP(size, !h) - GUI_SCROLL_BAR_WIDTH;

                if (    h == 0 && // Vertical
                        gui_focused(ctx) &&
                        ctx->mouse_scroll != 0 && // Scrolling mouse wheel
                        !(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)) // Not holding ctrl
                    SELECT_COMP(frame->scroll, h) -= ctx->mouse_scroll*64;

                // Scroll by dragging
                if (    gui_turtle(ctx)->window_ix == ctx->active_win_ix &&
                        (ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT) &&
                        (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT)) {
                    if (!ctx->dragging)
                        gui_start_dragging(ctx, v2i_to_v2f(frame->scroll));
                    else
                        SELECT_COMP(frame->scroll, h) = SELECT_COMP(v2f_to_v2i(ctx->drag_start_value) + ctx->drag_start_pos - ctx->cursor_pos, h);
                }

                float scroll = 1.f*SELECT_COMP(frame->scroll, h);
                float rel_shown_area = 1.f*SELECT_COMP(size, h)/SELECT_COMP(frame->last_bounding_size, h);
                float max_comp_scroll = 1.f*SELECT_COMP(max_scroll, h);
                gui_slider_ex(ctx, scroll_label, &scroll, 0, max_comp_scroll, rel_shown_area, !!h, SELECT_COMP(size, h) - GUI_SCROLL_BAR_WIDTH);
                SELECT_COMP(frame->scroll, h) = (int)scroll;
            }
        }
        gui_end(ctx);

        if (max_scroll.x > 0)
            frame->scroll.x = CLAMP(frame->scroll.x, 0, max_scroll.x);
        if (max_scroll.y > 0)
            frame->scroll.y = CLAMP(frame->scroll.y, 0, max_scroll.y);

        // Scroll client area
        V2i client_start_pos = gui_turtle(ctx)->pos - frame->scroll;
        gui_turtle(ctx)->start_pos = client_start_pos;
        gui_turtle(ctx)->pos = client_start_pos;
    }
}

void gui_end_frame(GuiContext *ctx)
{
    gui_frame(ctx)->last_bounding_size = gui_turtle(ctx)->bounding_max - gui_turtle(ctx)->start_pos;
    gui_end(ctx);
}

void gui_offset_frame(GuiContext *ctx, V2i offset)
{ gui_frame(ctx)->scroll = gui_frame(ctx)->scroll - offset; }

V2i gui_frame_scroll(GuiContext *ctx)
{ return gui_frame(ctx)->scroll; }

void gui_begin_window_ex(GuiContext *ctx, const char *label, V2i default_size)
{
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
            if (win_client_size.x < last_content_size.x || win_client_size.y < last_content_size.y) {
                // Resize window so that content is fully shown
                // @todo Implement minimum and maximum size to gui backend
                default_size.x = MAX(default_size.x, MAX(win_client_size.x, last_content_size.x));
                default_size.y = MAX(default_size.y, MAX(win_client_size.y, last_content_size.y));
                ctx->windows[win_handle].client_size = min_size;
            }
#endif
        } else {
            assert(free_handle >= 0);
            // Create new window
            //bool toplevel_turtle = (ctx->turtle_ix == 0);
            assert(ctx->window_count < MAX_GUI_WINDOW_COUNT);

            ctx->windows[free_handle].id = gui_id(label);
            ctx->windows[free_handle].client_size = default_size;
            ctx->windows[free_handle].pos = ctx->next_window_pos;
            ctx->window_order[ctx->window_count++] = free_handle;

            win_handle = free_handle;
        }
    }
    assert(win_handle >= 0);
    GuiContext_Window *win = &ctx->windows[win_handle];
    assert(!win->used && "Same window used twice in a frame");
    win->used = true;
    if (!win->used_in_last_frame)
        ctx->focused_win_ix = win_handle; // Appearing window will be focused

    gui_begin(ctx, label, true);
    gui_turtle(ctx)->window_ix = win_handle;
    gui_turtle(ctx)->layer = 1337 + gui_window_order(ctx, win_handle) * GUI_LAYERS_PER_WINDOW;

    { // Ordinary gui element logic
        V2i size = win->client_size + v2i(0, GUI_WINDOW_TITLE_BAR_HEIGHT);
        win->total_size = size;

        // Dummy window background element. Makes clicking through window impossible.
        char bg_label[MAX_GUI_LABEL_SIZE];
        FMT_STR(bg_label, ARRAY_COUNT(bg_label), "winbg_%s", label);
        gui_button_logic(ctx, bg_label, win->pos, size, NULL, NULL, NULL, NULL);

        // Title bar logic
        char bar_label[MAX_GUI_LABEL_SIZE];
        FMT_STR(bar_label, ARRAY_COUNT(bar_label), "winbar_%s", label);
        bool went_down, down, hover;
        gui_button_logic(ctx, bar_label, win->pos, v2i(size.x, GUI_WINDOW_TITLE_BAR_HEIGHT), NULL, &went_down, &down, &hover);

        if (ctx->active_win_ix == win_handle) {
            // Lift window to top
            bool found = false;
            for (int i = 0; i < ctx->window_count; ++i) {
                if (!found) {
                    if (ctx->window_order[i] == win_handle)
                        found = true;
                } else {
                    ctx->window_order[i - 1] = ctx->window_order[i];
                }
            }
            ctx->window_order[ctx->window_count - 1] = win_handle;
        }

        if (went_down)
            gui_start_dragging(ctx, v2i_to_v2f(win->pos));

        //V2i prev_value = win->pos;
        if (down && ctx->dragging)
            win->pos = v2f_to_v2i(ctx->drag_start_value) - ctx->drag_start_pos + ctx->cursor_pos;

        win->pos.x = MAX(10 - size.x, win->pos.x);
        win->pos.y = MAX(10 - GUI_WINDOW_TITLE_BAR_HEIGHT, win->pos.y);

        V2i px_pos = pt_to_px(win->pos, ctx->dpi_scale);
        V2i px_size = pt_to_px(size, ctx->dpi_scale);
        ctx->callbacks.draw_window(ctx->callbacks.user_data,
                                   1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y, GUI_WINDOW_TITLE_BAR_HEIGHT,
                                   gui_label_text(label), gui_focused(ctx), gui_layer(ctx));

        // Turtle to content area
        V2i content_pos = win->pos + v2i(0, GUI_WINDOW_TITLE_BAR_HEIGHT);
        gui_turtle(ctx)->start_pos = content_pos;
        gui_turtle(ctx)->pos = content_pos;
    }

    { // Corner resize handle
        char resize_label[MAX_GUI_LABEL_SIZE];
        FMT_STR(resize_label, ARRAY_COUNT(resize_label), "winresize_%s", label);
        gui_begin(ctx, resize_label, true); // Detach so that the handle doesn't take part in window contents size
        gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window

        V2i handle_size = v2i(GUI_SCROLL_BAR_WIDTH, GUI_SCROLL_BAR_WIDTH);
        V2i handle_pos = win->pos + win->total_size - handle_size;
        bool went_down, down, hover;
        gui_button_logic(ctx, resize_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

        if (went_down)
            gui_start_dragging(ctx, v2i_to_v2f(win->client_size));

        if (down)
            win->client_size = v2f_to_v2i(ctx->drag_start_value) + ctx->cursor_pos - ctx->drag_start_pos;
        win->client_size.x = MAX(win->client_size.x, 40);
        win->client_size.y = MAX(win->client_size.y, 40);

        V2i px_pos = pt_to_px(handle_pos, ctx->dpi_scale);
        V2i px_size = pt_to_px(handle_size, ctx->dpi_scale);
        ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y,
                                   down, hover, gui_layer(ctx), gui_scissor(ctx));

        gui_end(ctx);
    }

    gui_begin_frame(ctx, label, win->pos + v2i(0, GUI_WINDOW_TITLE_BAR_HEIGHT), win->client_size);

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

void gui_end_window_ex(GuiContext *ctx, bool * /*open*/)
{
    gui_end_frame(ctx);
    gui_end(ctx);
}

void gui_begin_window(GuiContext *ctx, const char *label, V2i default_size)
{
    gui_begin_window_ex(ctx, label, default_size);
}

void gui_end_window(GuiContext *ctx, bool *open)
{
    gui_end_window_ex(ctx, open);
}

V2i gui_window_client_size(GuiContext *ctx)
{ return gui_window(ctx)->client_size; }

void gui_begin_contextmenu(GuiContext *ctx, const char *label)
{
    gui_begin_window_ex(ctx, label, v2i(10, 10));
}

void gui_end_contextmenu(GuiContext *ctx, bool *open)
{
    gui_end_window_ex(ctx, open);
}

bool gui_contextmenu_item(GuiContext *ctx, const char *label)
{
    bool ret = gui_button(ctx, label);
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
    DragDropData empty = {};
    gui_turtle(ctx)->inactive_dragdropdata = empty;
}

bool gui_knob(GuiContext *ctx, const char *label, float min, float max, float *value, const char *value_str)
{
    bool ret = false;
    gui_begin(ctx, label);

    V2i pos = gui_turtle_pos(ctx);
    V2i size = gui_size(ctx, label, v2i(32, 32));


    //if (ctx->skinning_mode == SkinningMode_none) {
        bool went_down;
        bool down;
        bool hover;
        gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

        if (went_down)
            ctx->drag_start_value.x = *value;

        if (down && ctx->dragging) {
            *value = ctx->drag_start_value.x + (max - min) / 100.0f*(ctx->drag_start_pos.y - ctx->cursor_pos.y);
            *value = CLAMP(*value, min, max);

            { // Show value
                const char *str = value_str;
                char buf[128];
                if (!str) {
                    FMT_STR(buf, ARRAY_COUNT(buf), "%.3f", *value);
                    str = buf;
                }
            }
        }
        ret = down;
    //}

    //float d = ctx->dpi_scale;
    //V2i center = pos + size / 2;
    //V2i knob_center = v2i(center.x, pos.y + size.x / 2);
    //float rad = size.x / 2.0f;
    //Color auxillary = ctx->skin.default_color;
    //auxillary.a *= 0.4f;
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad - ctx->skin.knob_arc_width, d) - 3.0f, 2, 4, auxillary);
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad, d),
    //    pt_to_px_f32(ctx->skin.knob_arc_width, d), pt_to_px_f32(ctx->skin.knob_bloom_width, d),
    //    gui_color(ctx, label, ctx->skin.default_color),
    //    TAU * 3 / 8, TAU * 3 / 4 * phase_f32(*value, min, max));
    //draw_text(gui_rendering(ctx), pt_to_px(center + v2i(0, size.y / 2 - 14), d), TextAlign_center, gui_label_text(label), ctx->skin.default_color);
    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);

    return ret;
}

void gui_label(GuiContext *ctx, const char *label)
{
    gui_begin(ctx, label);
    V2i pos = gui_turtle_pos(ctx);
    //V2i size = draw_text(gui_rendering(ctx),
    //    pt_to_px(pos, ctx->dpi_scale),
    //    TextAlign_left,
    //    gui_label_text(label),
    //    gui_color(ctx, label, ctx->skin.default_color));
    V2i size = v2i(30, 30); // @todo
    gui_set_turtle_pos(ctx, pos);
    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);
}

bool gui_button_ex(GuiContext *ctx, const char *label, bool force_down)
{
    gui_begin(ctx, label);
    V2i margin = v2i(5, 3);
    V2i pos = gui_turtle(ctx)->pos;
    V2i size = gui_size(ctx, label, v2i(50, 21)); // @todo Minimum size to skin

    bool went_up = false, hover = false, down = false;
    if (gui_is_inside_window(ctx, size))
    {
        // @todo Recalc size when text changes regardless if is in a window or not. Disabled size.y change because caused shaking with long lists.
        float text_size[2];
        ctx->callbacks.calc_text_size(text_size, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
        size.x = MAX((int)text_size[0] + margin.x * 2, size.x);
        //size.y = MAX((int)text_size[1] + margin.y * 2, size.y);

        gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

        V2i px_pos = pt_to_px(pos, ctx->dpi_scale);
        V2i px_size = pt_to_px(size, ctx->dpi_scale);
        ctx->callbacks.draw_button(ctx->callbacks.user_data, 1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y, down || force_down, hover, gui_layer(ctx), gui_scissor(ctx));

        V2i px_margin = pt_to_px(margin, ctx->dpi_scale);
        ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos.x + px_margin.x, 1.f*px_pos.y + px_margin.y, gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
    }

    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);

    gui_next_row(ctx);

    return went_up && hover;
}

bool gui_button(GuiContext *ctx, const char *label)
{ return gui_button_ex(ctx, label, false); }

bool gui_selectable(GuiContext *ctx, const char *label, bool selected)
{ return gui_button_ex(ctx, label, selected); }

bool gui_checkbox_ex(GuiContext *ctx, const char *label, bool *value, bool radio_button_visual)
{
    gui_begin(ctx, label);
    V2i margin = v2i(5, 3);
    V2i pos = gui_turtle(ctx)->pos;
    V2i size = gui_size(ctx, label, v2i(20, 20)); // @todo Minimum size to skin

    bool went_up = false, hover = false, down = false;
    if (gui_is_inside_window(ctx, size))
    {
        float text_size[2];
        ctx->callbacks.calc_text_size(text_size, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
        size.x = MAX((int)text_size[0] + margin.x * 2, size.x);
        size.y = MAX((int)text_size[1] + margin.y * 2, size.y);

        V2i px_margin = pt_to_px(margin, ctx->dpi_scale);
        int box_width = size.y;
        float px_box_width = pt_to_px(v2i(0, box_width), ctx->dpi_scale).y - px_margin.y*2.f;

        size.x += box_width + margin.x;

        gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

        V2i px_pos = pt_to_px(pos, ctx->dpi_scale);
        //V2i px_size = pt_to_px(size, ctx->dpi_scale);

        float x = 1.f*px_pos.x + px_margin.x;
        float y = 1.f*px_pos.y + px_margin.x;
        float w = 1.f*px_box_width;
        if (radio_button_visual)
            ctx->callbacks.draw_radiobutton(ctx->callbacks.user_data, x, y, w, *value, down, hover, gui_layer(ctx), gui_scissor(ctx));
        else
            ctx->callbacks.draw_checkbox(ctx->callbacks.user_data, x, y, w, *value, down, hover, gui_layer(ctx), gui_scissor(ctx));
        ctx->callbacks.draw_text(ctx->callbacks.user_data, px_pos.x + px_box_width + 2.f*px_margin.x, 1.f*px_pos.y + px_margin.y,
                                 gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
    }

    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);

    gui_next_row(ctx); // @todo Layouting

    if (value && went_up && hover)
        *value = !*value;
    return went_up && hover;
}

bool gui_checkbox(GuiContext *ctx, const char *label, bool *value)
{ return gui_checkbox_ex(ctx, label, value, false); }

bool gui_radiobutton(GuiContext *ctx, const char *label, bool value)
{
    bool v = value;
    return gui_checkbox_ex(ctx, label, &v, true); // @todo Proper radiobutton
}

void gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max)
{
    gui_slider_ex(ctx, label, value, min, max, 0.1f, true, gui_window(ctx)->client_size.x);
    gui_next_row(ctx); // @todo Layouting
}

bool gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size)
{
    bool content_changed = false;

    gui_begin(ctx, label);
    V2i margin = v2i(5, 3);
    V2i pos = gui_turtle(ctx)->pos;
    V2i size = gui_size(ctx, label, v2i(300, 21)); // @todo Minimum size to skin
    V2i box_size = size;
    V2i label_size = v2i(0, 0);
    bool has_label = (strlen(gui_label_text(label)) > 0);
    if (has_label)
    {
        float label_size_f[2];
        ctx->callbacks.calc_text_size(label_size_f, ctx->callbacks.user_data, gui_label_text(label), gui_layer(ctx));
        label_size.x = (int)label_size_f[0] + margin.x*2;
        label_size.y = (int)label_size_f[1] + margin.y*2;

        size.x += label_size.x;
        box_size.x -= label_size.x;
    }

    bool went_down = false, hover = false;
    if (gui_is_inside_window(ctx, size))
    {
        gui_button_logic(ctx, label, pos, size, NULL, &went_down, NULL, &hover);
        bool active = (ctx->last_active_id == gui_id(label));

        if (active) {
            assert(buf && buf_size > 0);
            int char_count = static_cast<int>(strlen(buf));
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
                content_changed = true;
            }
            char_count = MIN(char_count, buf_size - 1);
            buf[char_count] = '\0';
        }

        V2i px_margin = pt_to_px(margin, ctx->dpi_scale);
        if (has_label) { // Draw label
            V2i px_pos = pt_to_px(pos, ctx->dpi_scale);
            ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos.x + px_margin.x, 1.f*px_pos.y, gui_label_text(label), gui_layer(ctx), gui_scissor(ctx));
        }

        { // Draw textbox
            V2i px_pos = pt_to_px(pos + v2i(label_size.x, 0), ctx->dpi_scale);
            V2i px_size = pt_to_px(box_size, ctx->dpi_scale);
            // @todo down --> active
            ctx->callbacks.draw_textbox(ctx->callbacks.user_data, 1.f*px_pos.x, 1.f*px_pos.y, 1.f*px_size.x, 1.f*px_size.y, active, hover, gui_layer(ctx), gui_scissor(ctx));

            ctx->callbacks.draw_text(ctx->callbacks.user_data, 1.f*px_pos.x + px_margin.x, 1.f*px_pos.y + px_margin.y, buf, gui_layer(ctx), gui_scissor(ctx));
        }

    }

    gui_enlarge_bounding(ctx, pos + size);
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
    gui_set_turtle_pos(ctx, v2i(gui_turtle(ctx)->pos.x, gui_turtle(ctx)->last_bounding_max.y));
}

void gui_next_col(GuiContext *ctx)
{
    gui_set_turtle_pos(ctx, v2i(gui_turtle(ctx)->last_bounding_max.x, gui_turtle(ctx)->pos.y));
}

void gui_enlarge_bounding(GuiContext *ctx, V2i pos)
{
    GuiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_ix];

    turtle->bounding_max.x = MAX(turtle->bounding_max.x, pos.x);
    turtle->bounding_max.y = MAX(turtle->bounding_max.y, pos.y);

    turtle->last_bounding_max = pos;
}

void gui_ver_space(GuiContext *ctx)
{
    V2i pos = gui_turtle(ctx)->pos;
    gui_enlarge_bounding(ctx, pos + v2i(25, 0));
    gui_next_col(ctx);
}

void gui_hor_space(GuiContext *ctx)
{
    V2i pos = gui_turtle(ctx)->pos;
    gui_enlarge_bounding(ctx, pos + v2i(0, 25));
    gui_next_row(ctx);
}

