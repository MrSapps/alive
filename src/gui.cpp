#include "gui.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(*x))
#define CLAMP(v, a, b) (MIN(MAX((v), (a)), (b)))
#define ABS(x) ((x) < 0 ? (-x) : x)

void destroy_window(GuiContext *ctx, int handle)
{
    GuiContext_Window *win = &ctx->windows[handle];

    GuiContext_Window zero_win = {};
    *win = zero_win;
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
    skin.knob_size = V2i(32, 32);
    skin.element_offsets = create_tbl(GuiId, V2i)(0, V2i(100000, 100000), MAX_GUI_ELEMENT_COUNT);
    skin.element_sizes = create_tbl(GuiId, V2i)(0, V2i(100000, 100000), MAX_GUI_ELEMENT_COUNT);
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
    fprintf(file, "	s.knob_size = V2i(%i, %i);\n", s->knob_size.x, s->knob_size.y);
    fprintf(file, "	s.default_color = color(%ff, %ff, %ff, %ff);\n", s->default_color.r, s->default_color.g, s->default_color.b, s->default_color.a);
    fprintf(file, "\n");

    for (U32 i = 0; i < s->element_offsets.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_offsets.array_data[i];
        if (entry.key == s->element_offsets.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_offsets, %u, V2i(%i, %i));\n",
            entry.key, entry.value.x, entry.value.y);
    }
    fprintf(file, "\n");
    for (U32 i = 0; i < s->element_sizes.array_size; ++i) {
        const HashTbl_Entry(GuiId, V2i) entry = s->element_sizes.array_data[i];
        if (entry.key == s->element_sizes.null_key)
            continue;
        fprintf(file, "	set_tbl(GuiId, V2i)(&s.element_sizes, %u, V2i(%i, %i));\n",
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

void gui_enlarge_bounding(GuiContext *ctx, V2i pos)
{
    GuiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_ix];

    turtle->bounding_max.x = MAX(turtle->bounding_max.x, pos.x);
    turtle->bounding_max.y = MAX(turtle->bounding_max.y, pos.y);

    turtle->last_bounding_max = pos;
}

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

void gui_set_turtle_pos(GuiContext *ctx, V2i pos)
{
    ctx->turtles[ctx->turtle_ix].pos = pos;
    gui_enlarge_bounding(ctx, pos);
}

GuiContext_Turtle *gui_turtle(GuiContext *ctx)
{
    return &ctx->turtles[ctx->turtle_ix];
}

GuiContext_Window *gui_window(GuiContext *ctx)
{
    return &ctx->windows[gui_turtle(ctx)->window_ix];
}

bool gui_focused(GuiContext *ctx)
{
    return gui_turtle(ctx)->window_ix == ctx->focused_window_ix;
}

bool gui_hovered(GuiContext *ctx)
{
    return gui_turtle(ctx)->window_ix == ctx->hovered_window_ix;
}

V2i gui_turtle_pos(GuiContext *ctx) { return gui_turtle(ctx)->pos; }
V2i gui_parent_turtle_start_pos(GuiContext *ctx)
{
    if (ctx->turtle_ix == 0)
        return V2i(0, 0);
    return ctx->turtles[ctx->turtle_ix - 1].start_pos;
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
V2i gui_pos(GuiContext *ctx, const char *label, V2i pos)
{
    //V2i override_offset = get_tbl(GuiId, V2i)(&ctx->skin.element_offsets, gui_id(label));
    //if (override_offset == ctx->skin.element_offsets.null_value)
        return pos;
    //return gui_parent_turtle_start_pos(ctx) + override_offset;
}
V2i gui_size(GuiContext *ctx, const char *label, V2i size)
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

GuiContext *create_gui(GuiCallbacks callbacks)
{
    GuiContext *ctx = (GuiContext*)calloc(1, sizeof(*ctx));
    ctx->dpi_scale = 1.0f;
    ctx->callbacks = callbacks;
    return ctx;
}

void destroy_gui(GuiContext *ctx)
{
    //destroy_skin(&ctx->skin);

    for (int i = MAX_GUI_WINDOW_COUNT - 1; i >= 0; --i) {
        if (ctx->windows[i].id)
            destroy_window(ctx, i);
    }

    free(ctx);
}

void gui_set_hot(GuiContext *ctx, const char *label)
{
    // @todo hot_id might need some turtle depth checking in case of overlapping things
    if (ctx->active_id == 0 && ctx->hot_id == 0) {
        ctx->hot_id = gui_id(label);
    }
}

bool gui_is_hot(GuiContext *ctx, const char *label)
{
    return ctx->last_hot_id == gui_id(label);
}

void gui_set_active(GuiContext *ctx, const char *label)
{
    ctx->active_id = gui_id(label);
}

void gui_set_inactive(GuiContext *ctx, const char *label)
{
    if (ctx->active_id == gui_id(label))
        ctx->active_id = 0;
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

    if (gui_is_active(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) {
            if (down) *down = true;
        }
        else if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_RELEASED_BIT) {
            if (went_up) *went_up = true;
            gui_set_inactive(ctx, label);
        }

        //if (ctx->key_state[GUI_KEY_RMB] & GUI_KEYSTATE_RELEASED_BIT) {
         //   gui_set_inactive(ctx, label);
        //}
    }
    else if (gui_is_hot(ctx, label)) {
        if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_PRESSED_BIT) {
            if (down) *down = true;
            if (went_down) *went_down = true;
            gui_set_active(ctx, label);
        }
        //if (ctx->key_state[GUI_KEY_RMB] & GUI_KEYSTATE_PRESSED_BIT)
            //gui_set_active(ctx, label);
    }

    const V2i c_p = px_to_pt(ctx->cursor_pos, ctx->dpi_scale);
    if (c_p.x >= pos.x &&
        c_p.y >= pos.y &&
        c_p.x < pos.x + size.x &&
        c_p.y < pos.y + size.y && gui_hovered(ctx)) {
        gui_set_hot(ctx, label);
        if (hover) *hover = true;
    }
}

//void do_skinning(GuiContext *ctx, const char *label, V2i pos, V2i size, Color col);

void gui_begin(GuiContext *ctx, const char *label, bool detached)
{
    assert(ctx->turtle_ix < MAX_GUI_STACK_SIZE);
    if (ctx->turtle_ix >= MAX_GUI_STACK_SIZE)
        ctx->turtle_ix = 0; // Failsafe

    GuiContext_Turtle *prev = &ctx->turtles[ctx->turtle_ix];
    GuiContext_Turtle *cur = &ctx->turtles[ctx->turtle_ix + 1];

    ++ctx->turtle_ix;
    GuiContext_Turtle new_turtle = {};
    new_turtle.pos = detached ? v2i(0, 0) : gui_pos(ctx, label, prev->pos);
    new_turtle.start_pos = new_turtle.pos;
    new_turtle.bounding_max = new_turtle.pos;
    new_turtle.last_bounding_max = new_turtle.pos;
    new_turtle.window_ix = prev->window_ix;
    new_turtle.detached = detached;
    new_turtle.inactive_dragdropdata = prev->inactive_dragdropdata;
    snprintf(new_turtle.label, MAX_GUI_LABEL_SIZE, "%s", label);
    *cur = new_turtle;

    V2i size = gui_size(ctx, label, v2i(-1, -1));
    if (size != v2i(-1, -1)) {
        // This panel has predetermined (minimum) size
        gui_turtle(ctx)->bounding_max = gui_turtle(ctx)->start_pos + size;
    }
}

#define GUI_HANDLE_RIGHT_BIT 0x01
#define GUI_HANDLE_BOTTOM_BIT 0x02
// Resize handle for parent element
void do_resize_handle(GuiContext *ctx, bool right_handle)
{
    V2i panel_size = gui_turtle(ctx)->bounding_max - gui_turtle(ctx)->start_pos;
    GuiId panel_id = gui_id(gui_turtle(ctx)->label);

    char handle_label[MAX_GUI_LABEL_SIZE];
    snprintf(handle_label, ARRAY_COUNT(handle_label), "%ihandle_%s", right_handle ? 0 : 1, gui_turtle(ctx)->label);
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

#if 0
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
#endif

    // @todo Change cursor when hovering

    //Color col = ctx->skin.default_color;
    //col.a *= 0.5f; // @todo Color to skin
    //draw_rect(gui_rendering(ctx), pt_to_px(handle_pos, ctx->dpi_scale), pt_to_px(handle_size, ctx->dpi_scale), col);
    gui_end_ex(ctx, true, NULL); }
}

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

    { // Dragging logic
        if (ctx->hovered_window_ix == gui_turtle(ctx)->window_ix &&
            (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) && !ctx->dragging) {
            V2i pos = gui_turtle(ctx)->start_pos;
            V2i size = gui_turtle(ctx)->bounding_max - pos;
            const V2i c_p = px_to_pt(ctx->cursor_pos, ctx->dpi_scale);
            //if (v2i_in_rect(c_p, pos, size)) { // This if doesn't work well with window dragging
                ctx->dragging = true;
                ctx->drag_start_pos = ctx->cursor_pos;
                // Store dragdropdata
                ctx->dragdropdata = gui_turtle(ctx)->inactive_dragdropdata;
            //}
        }

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
        // Destroy closed windows
        for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
            if (!ctx->windows[i].id)
                continue;

            if (!ctx->windows[i].used)
                destroy_window(ctx, i);
            ctx->windows[i].used = false;
        }

        ctx->last_hot_id = ctx->hot_id;
        ctx->hot_id = 0;
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
    snprintf(buf, ARRAY_COUNT(buf), "skin_overlay_%s", label);

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
                snprintf(str, ARRAY_COUNT(str), "(%.2f, %.2f, %.2f, %.2f)", newcol.r, newcol.g, newcol.b, newcol.a);
                draw_text(gui_rendering(ctx), ctx->cursor_pos + V2i(0, -15), TextAlign_left, str);
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

void gui_begin_window_ex(GuiContext *ctx, const char *label, V2i pos, V2i min_size)
{
    int win_handle = -1;
    { // Find/create platform window
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
                min_size.x = MAX(min_size.x, MAX(win_client_size.x, last_content_size.x));
                min_size.y = MAX(min_size.y, MAX(win_client_size.y, last_content_size.y));
                ctx->windows[win_handle].client_size = min_size;
            }
#endif
        }
        else {
            assert(free_handle >= 0);
            // Create new window
            bool toplevel_turtle = (ctx->turtle_ix == 0);

            ctx->windows[free_handle].id = gui_id(label);
            ctx->windows[free_handle].client_size = min_size;

            win_handle = free_handle;
        }
    }
    assert(win_handle >= 0);
    GuiContext_Window *win = &ctx->windows[win_handle];
    assert(!win->used && "Same window used twice in a frame");
    win->used = true;

    gui_begin(ctx, label, true);
    gui_turtle(ctx)->window_ix = win_handle;

    { // Ordinary gui element logic
        const float title_bar_height = 25.0f;
        V2i size = win->client_size + V2i(0, title_bar_height);

        bool went_down, down, hover;
        gui_button_logic(ctx, label, win->pos, V2i(size.x, title_bar_height), NULL, &went_down, &down, &hover);
        if (hover)
            ctx->hovered_window_ix = win_handle;

        if (went_down)
            ctx->drag_start_value = v2i_to_v2f(win->pos);

        V2i prev_value = win->pos;
        if (down && ctx->dragging)
            win->pos = v2f_to_v2i(ctx->drag_start_value) - ctx->drag_start_pos + ctx->cursor_pos;

        win->pos.x = MAX(10 - size.x, win->pos.x);
        win->pos.y = MAX(10 - title_bar_height, win->pos.y);

        V2i px_pos = pt_to_px(win->pos, ctx->dpi_scale);
        V2i px_size = pt_to_px(size, ctx->dpi_scale);
        ctx->callbacks.draw_window(ctx->callbacks.user_data, px_pos.x, px_pos.y, px_size.x, px_size.y, title_bar_height, gui_label_text(label));

        V2i content_pos = win->pos + V2i(0, title_bar_height);
        gui_turtle(ctx)->start_pos = content_pos;
        gui_turtle(ctx)->pos = content_pos;
    }

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

void gui_end_window_ex(GuiContext *ctx, bool *open)
{
    gui_window(ctx)->last_frame_bounding_max = gui_turtle(ctx)->bounding_max;

    gui_end(ctx);
}

void gui_begin_window(GuiContext *ctx, const char *label, V2i min_size)
{
    bool toplevel_turtle = (ctx->turtle_ix == 0);
    gui_begin_window_ex(ctx, label, V2i(0, 0), min_size);
}

void gui_end_window(GuiContext *ctx, bool *open)
{
    gui_end_window_ex(ctx, open);
}

void gui_begin_contextmenu(GuiContext *ctx, const char *label)
{
    gui_begin_window_ex(ctx, label, ctx->cursor_pos, v2i(10, 10));
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
    V2i size = gui_size(ctx, label, V2i(32, 32));


    //if (ctx->skinning_mode == SkinningMode_none) {
        bool went_down;
        bool down;
        bool hover;
        gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

        if (went_down)
            ctx->drag_start_value.x = *value;

        float prev_value = *value;
        if (down && ctx->dragging) {
            *value = ctx->drag_start_value.x + (max - min) / 100.0f*(ctx->drag_start_pos.y - ctx->cursor_pos.y);
            *value = CLAMP(*value, min, max);

            { // Show value
                const char *str = value_str;
                char buf[128];
                if (!str) {
                    snprintf(buf, ARRAY_COUNT(buf), "%.3f", *value);
                    str = buf;
                }
                //draw_text(gui_rendering(ctx),
                //    pt_to_px(pos + V2i(size.x / 2, -15), ctx->dpi_scale),
                //    TextAlign_center,
                //    str,
                //    gui_color(ctx, label, ctx->skin.default_color));
            }
        }
        ret = down;
    //}

    float d = ctx->dpi_scale;
    V2i center = pos + size / 2;
    V2i knob_center = v2i(center.x, pos.y + size.x / 2);
    float rad = size.x / 2.0f;
    //Color auxillary = ctx->skin.default_color;
    //auxillary.a *= 0.4f;
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad - ctx->skin.knob_arc_width, d) - 3.0f, 2, 4, auxillary);
    //draw_arc(gui_rendering(ctx), pt_to_px(knob_center, d), pt_to_px_f32(rad, d),
    //    pt_to_px_f32(ctx->skin.knob_arc_width, d), pt_to_px_f32(ctx->skin.knob_bloom_width, d),
    //    gui_color(ctx, label, ctx->skin.default_color),
    //    TAU * 3 / 8, TAU * 3 / 4 * phase_f32(*value, min, max));
    //draw_text(gui_rendering(ctx), pt_to_px(center + V2i(0, size.y / 2 - 14), d), TextAlign_center, gui_label_text(label), ctx->skin.default_color);
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
    V2i size(30, 30); // TODO
    gui_set_turtle_pos(ctx, pos);
    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);
}

bool gui_button(GuiContext *ctx, const char *label)
{
    gui_begin(ctx, label);
    V2i margin(5, 3);
    V2i pos = gui_turtle(ctx)->pos;
    V2i size = gui_size(ctx, label, V2i(50, 20)); // @todo Minimum size to skin
    float text_size[2];
    ctx->callbacks.calc_text_size(text_size, ctx->callbacks.user_data, gui_label_text(label));
    size.x = MAX(text_size[0] + margin.x*2, size.x);
    size.y = MAX(text_size[1] + margin.y*2, size.y);

    bool went_up, hover, down;
    gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

    V2i px_pos = pt_to_px(pos, ctx->dpi_scale);
    V2i px_size = pt_to_px(size, ctx->dpi_scale);
    ctx->callbacks.draw_button(ctx->callbacks.user_data, px_pos.x, px_pos.y, px_size.x, px_size.y, down, hover);

    V2i px_margin = pt_to_px(margin, ctx->dpi_scale);
    ctx->callbacks.draw_text(ctx->callbacks.user_data, px_pos.x + px_margin.x, px_pos.y + px_margin.y, gui_label_text(label));

    gui_enlarge_bounding(ctx, pos + size);
    gui_end(ctx);

    return went_up && hover;
}

void gui_next_row(GuiContext *ctx)
{
    gui_set_turtle_pos(ctx, V2i(gui_turtle(ctx)->pos.x, gui_turtle(ctx)->last_bounding_max.y));
}

void gui_next_col(GuiContext *ctx)
{
    gui_set_turtle_pos(ctx, V2i(gui_turtle(ctx)->last_bounding_max.x, gui_turtle(ctx)->pos.y));
}

void gui_ver_space(GuiContext *ctx)
{
    V2i pos = gui_turtle(ctx)->pos;
    gui_enlarge_bounding(ctx, pos + V2i(25, 0));
    gui_next_col(ctx);
}

void gui_hor_space(GuiContext *ctx)
{
    V2i pos = gui_turtle(ctx)->pos;
    gui_enlarge_bounding(ctx, pos + V2i(0, 25));
    gui_next_row(ctx);
}

