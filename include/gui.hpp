#ifndef ALIVE_GUI_H
#define ALIVE_GUI_H

#include <stdint.h>
#include <stddef.h>

// Non-bloaty immediate mode gui

typedef uint32_t GuiId;

struct V2i {
    int x, y;

    V2i() : x(0), y(0) {}
    V2i(int x, int y) : x(x), y(y) {}
};

V2i v2i(int x, int y);
V2i operator+(V2i a, V2i b);
V2i operator-(V2i a, V2i b);
V2i operator*(V2i a, V2i b);
V2i operator*(V2i a, int m);
V2i operator/(V2i v, int d);
bool operator==(V2i a, V2i b);
bool operator!=(V2i a, V2i b);
V2i rounded_to_grid(V2i v, int grid);

struct V2f {
    float x, y;

    V2f() : x(0), y(0) {}
    V2f(float x, float y) : x(x), y(y) {}
};
V2f operator+(V2f a, V2f b);
V2f operator*(V2f a, float m);
V2f v2i_to_v2f(V2i v);
V2i v2f_to_v2i(V2f v);
bool v2i_in_rect(V2i v, V2i pos, V2i size);

struct Skin {
#if 0
    // Global defaults. In points, not pixels.
    float knob_arc_width;
    float knob_bloom_width;
    float knob_size;

    Color default_color;

    // Element-wise values
    HashTbl(GuiId, V2i) element_offsets;
    HashTbl(GuiId, V2i) element_sizes;
    HashTbl(GuiId, U8) element_grids;
    HashTbl(GuiId, Color) element_colors;
    HashTbl(GuiId, U8) element_resize_handles;
#endif
};

#if 0
Skin create_skin();
void destroy_skin(Skin *skin);

enum SkinningMode {
    SkinningMode_none,
    SkinningMode_pos,
    SkinningMode_size,
    SkinningMode_r,
    SkinningMode_g,
    SkinningMode_b,
    SkinningMode_a,
    SkinningMode_right_handle, // Enable/disable a handle which is dragged in x-direction
    SkinningMode_bottom_handle, // Enable/disable a handle which is dragged in y-direction
    SkinningMode_grid_1,
    SkinningMode_grid_2,
    SkinningMode_grid_3,
    SkinningMode_grid_4,
    SkinningMode_grid_5,
    SkinningMode_grid_last,
};
#endif

#define MAX_GUI_LABEL_SIZE 256

struct DragDropData {
    const char *tag; // Receiver checks this 
    int ix;
};

struct GuiContext_Turtle {
    V2i pos; // Output "cursor
    V2i start_pos;
    V2i bounding_max;
    V2i last_bounding_max; // Most recently added gui element
    char label[MAX_GUI_LABEL_SIZE]; // Label of the gui_begin
    int window_ix;
    int layer; // Graphical layer
    bool detached; // If true, moving of this turtle doesn't affect parent bounding boxes etc.
    DragDropData inactive_dragdropdata; // This is copied to gui context when actual dragging and dropping within this turtle starts
};

struct Rendering;
struct GuiContext_Window {
    GuiId id;
    bool used;
    V2i last_frame_bounding_size;
    int scroll; // Translation in pt. Cannot be relative, because adding content shouldn't cause translation to change.

    V2i pos; // Top-left position
    V2i client_size; // Size, not taking account title bar or borders
    V2i total_size; // Value depends from client_size
};

#define MAX_GUI_STACK_SIZE 32
#define MAX_GUI_ELEMENT_COUNT 256 // @todo Remove limit
#define MAX_GUI_WINDOW_COUNT 64 // @todo Remove limit
#define GUI_FILENAME_SIZE MAX_PATH_SIZE // @todo Remove limit

#define GUI_KEYSTATE_DOWN_BIT 0x1
#define GUI_KEYSTATE_PRESSED_BIT 0x2
#define GUI_KEYSTATE_RELEASED_BIT 0x4

#define GUI_KEY_COUNT 256
#define GUI_KEY_LMB 0
#define GUI_KEY_MMB 1
#define GUI_KEY_RMB 2
#define GUI_KEY_0 '0'
#define GUI_KEY_1 '1'
#define GUI_KEY_2 '2'
#define GUI_KEY_3 '3'
#define GUI_KEY_4 '4'
#define GUI_KEY_5 '5'
#define GUI_KEY_6 '6'
#define GUI_KEY_7 '7'
#define GUI_KEY_8 '8'
#define GUI_KEY_9 '9'

typedef void (*DrawButtonFunc)(void *user_data, float x, float y, float w, float h, bool down, bool hover, int layer);
typedef void (*DrawCheckBoxFunc)(void *user_data, float x, float y, float w, bool checked, bool down, bool hover, int layer);
typedef void (*DrawRadioButtonFunc)(void *user_data, float x, float y, float w, bool checked, bool down, bool hover, int layer);
typedef void (*DrawTextFunc)(void *user_data, float x, float y, const char *text, int layer);
typedef void (*CalcTextSizeFunc)(float ret[2], void *user_data, const char *text, int layer);
typedef void (*DrawWindowFunc)(void *user_data, float x, float y, float w, float h, float title_bar_height, const char *title, int layer);

// User-supplied callbacks
// TODO: Not sure if callbacks are better than providing an array containing all drawing commands of a frame.
struct GuiCallbacks {
    void *user_data;
    DrawButtonFunc draw_button;
    DrawCheckBoxFunc draw_checkbox;
    DrawRadioButtonFunc draw_radiobutton;
    DrawTextFunc draw_text;
    CalcTextSizeFunc calc_text_size;
    DrawWindowFunc draw_window;
};

// Handles the gui state
struct GuiContext {
    // Write to these to make gui work
    V2i cursor_pos; // Screen position, pixel coordinates
    uint8_t key_state[GUI_KEY_COUNT];
    V2i next_window_pos;

    // Internals

    V2i cursor_delta;

    V2i drag_start_pos; // Pixel coordinates
    bool dragging;
    V2f drag_start_value; // Knob value, or xy position, or ...
    DragDropData dragdropdata; // Data from gui component which is currently dragged

    GuiContext_Turtle turtles[MAX_GUI_STACK_SIZE];
    int turtle_ix;

    GuiContext_Window windows[MAX_GUI_WINDOW_COUNT];
    int window_order[MAX_GUI_WINDOW_COUNT];
    int window_count;

    // This is used at mouse input and render dimensions. All gui calculations are done in pt.
    float dpi_scale; // 1.0: pixels == points, 2.0: pixels == 2.0*points (gui size is doubled). 

    GuiId hot_id, last_hot_id;
    int hot_win_ix;
    GuiId active_id;
    int active_win_ix;

    Skin skin;
    //SkinningMode skinning_mode;

    GuiCallbacks callbacks;
};

const char *gui_label_text(const char *label);

// Startup and shutdown of GUI
// @todo Size should be defined in gui_begin_window()
// @note skin_source_file contents is not copied
GuiContext *create_gui(GuiCallbacks callbacks);
void destroy_gui(GuiContext *ctx);

int gui_layer(GuiContext *ctx);

void gui_begin_window(GuiContext *ctx, const char *label, V2i min_size);
void gui_end_window(GuiContext *ctx, bool *open = NULL);

void gui_begin_contextmenu(GuiContext *ctx, const char *label);
void gui_end_contextmenu(GuiContext *ctx, bool *open);
bool gui_contextmenu_item(GuiContext *ctx, const char *label);

void gui_begin_dragdrop_src(GuiContext *ctx, DragDropData data);
void gui_end_dragdrop_src(GuiContext *ctx);

// Gui controls
bool gui_knob(GuiContext *ctx, const char *label, float min, float max, float *value, const char *value_str = NULL);
void gui_label(GuiContext *ctx, const char *label);

bool gui_button(GuiContext *ctx, const char *label);
bool gui_checkbox(GuiContext *ctx, const char *label, bool *value);
bool gui_radiobutton(GuiContext *ctx, const char *label, bool value);
void gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max);

void gui_begin(GuiContext *ctx, const char *label, bool detached = false);
void gui_end(GuiContext *ctx);
void gui_end_droppable(GuiContext *ctx, DragDropData *dropped);
void gui_end_ex(GuiContext *ctx, bool make_zero_size, DragDropData *dropped);

void gui_set_turtle_pos(GuiContext *ctx, V2i pos);
V2i gui_turtle_pos(GuiContext *ctx);
void gui_next_row(GuiContext *ctx);
void gui_next_col(GuiContext *ctx);

void gui_ver_space(GuiContext *ctx);
void gui_hor_space(GuiContext *ctx);

#endif
