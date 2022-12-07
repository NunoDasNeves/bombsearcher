#pragma once
#include"types.h"
#include"vec.h"

C_BEGIN

/*
 * Two sections, 'top' with the face and cells
 * Top each have borders. Top shares bottom border with cells top border. single border width
 * Total width is board.width * CELL_PIXEL_WIDTH + BORDER_WIDTH * 2
 */
#define CELL_PIXEL_WIDTH 64
#define CELL_PIXEL_HEIGHT CELL_PIXEL_WIDTH
#define FACE_PIXEL_WIDTH 108
#define FACE_PIXEL_HEIGHT FACE_PIXEL_WIDTH
#define BORDER_PIXEL_WIDTH 16
#define BORDER_PIXEL_HEIGHT BORDER_PIXEL_WIDTH
#define NUM_7SEG_PIXEL_WIDTH 32
#define NUM_7SEG_PIXEL_HEIGHT 64
#define COUNTER_MAX 999
#define COUNTER_PIXEL_WIDTH 120
#define COUNTER_PIXEL_HEIGHT 84
// interior height should be a multiple of BORDER_PIXEL_HEIGHT
#define TOP_INTERIOR_HEIGHT ( (FACE_PIXEL_HEIGHT) + 20 + (BORDER_PIXEL_HEIGHT) )

typedef struct {
    u32 width;
    u32 height;
    u32 num_bombs;
} GameParams;

#define PARAMS_MIN_WIDTH 6
#define PARAMS_MIN_HEIGHT 6
#define PARAMS_MIN_BOMBS 5
#define PARAMS_MAX_WIDTH 31
#define PARAMS_MAX_HEIGHT 31
#define PARAMS_MAX_BOMBS COUNTER_MAX
static const GameParams game_easy = { 9, 9, 10 };
static const GameParams game_medium = { 16, 16, 40 };
static const GameParams game_hard = { 30, 16, 99 };
static const GameParams game_custom_default = { 6, 31, 42 };

enum {
    CELL_UNEXPLORED = 0,
    CELL_FLAGGED,
    CELL_CLICKED,
    CELL_EXPLORED
};

typedef struct {
    u8 state;
    u8 bombs_around;
    bool is_bomb;
} Cell;

typedef struct {
    Cell *cells;
    i64 bombs_left;
    Cell *cell_last_clicked;
    Cell *bomb_clicked;
    u32 num_cells; // == width * height
    u32 num_bombs;
    u32 width;
    u32 height;
} Board;

static void board_idx_to_pos(Board *board, u32 idx, i64 *col, i64 *row)
{
    ASSERT(board);
    ASSERT(col);
    ASSERT(row);
    ASSERT(idx < board->num_cells);
    ASSERT(board->width > 0);

    *col = idx % board->width;
    *row = idx / board->width;
}

static Cell *board_pos_to_cell(Board *board, i64 c, i64 r)
{
    ASSERT(board);
    ASSERT(board->cells);
    ASSERT(c >= 0 && c < board->width);
    ASSERT(r >= 0 && r < board->height);

    return &board->cells[r * board->width + c];
}

static void board_cell_to_pos(Board *board, Cell *cell, i64 *c, i64 *r)
{
    ASSERT(board);
    ASSERT(board->cells);
    ASSERT(cell >= board->cells);
    ASSERT(cell < &board->cells[board->num_cells]);

    u32 idx = (u32)(cell - board->cells);
    board_idx_to_pos(board, idx, c, r);
}

enum {
    FACE_SMILE = 0,
    FACE_SCARED,
    FACE_DEAD,
    FACE_COOL
};

typedef struct {
    u32 mouse_x;
    u32 mouse_y;
    bool mouse_left_down;
    bool mouse_right_down;
#ifdef DEBUG
    bool debug_key;
#endif
} Input;

typedef struct {
    Board board;
    Input last_input; // input from previous frame
    u64 time_started_ms;
    u64 time_ms;
    u8 face_state; // one of FACE_SMILE, FACE_SCARED, etc
    bool face_clicked;
    bool playing; // cells can be clicked on
    u32 window_scale; // game is scaled down by >>window_scale
    bool window_needs_resize;
    f32 main_menu_bar_height_window_px; // 'native' height of top menu bar; not scaled by window_scale
    GameParams params; // last params used to start the game, reused when face clicked on
} GameState;

extern GameState game_state;

static f32 menu_bar_y_offset_px()
{
    return (f32)((u32)game_state.main_menu_bar_height_window_px << game_state.window_scale);
}

static Vec2f game_dims_px()
{
    return vec2f(
        (f32)(((BORDER_PIXEL_WIDTH) * 2) + (game_state.params.width * CELL_PIXEL_WIDTH)),
        (f32)((BORDER_PIXEL_HEIGHT * 3) + (game_state.params.height * CELL_PIXEL_HEIGHT) + TOP_INTERIOR_HEIGHT + (u32)menu_bar_y_offset_px())
    );
}

static Vec2f game_dims_px_no_menu()
{
    return vec2f(
        (f32)(((BORDER_PIXEL_WIDTH) * 2) + (game_state.params.width * CELL_PIXEL_WIDTH)),
        (f32)((BORDER_PIXEL_HEIGHT * 3) + (game_state.params.height * CELL_PIXEL_HEIGHT) + TOP_INTERIOR_HEIGHT)
    );
}

static Vec2f cells_offset_px()
{
    return vec2f(
        (f32)BORDER_PIXEL_WIDTH,
        (f32)((TOP_INTERIOR_HEIGHT) + ((BORDER_PIXEL_HEIGHT) * 2) + (u32)menu_bar_y_offset_px())
    );
}

static f32 top_interior_center_y_px()
{
    return (f32)(BORDER_PIXEL_HEIGHT + (TOP_INTERIOR_HEIGHT / 2) + (u32)menu_bar_y_offset_px());
}

static Vec2f face_pos_px()
{
    Vec2f game_dims = game_dims_px();
    Vec2f pos = vec2f(
        (f32)(((u32)game_dims.x - FACE_PIXEL_WIDTH) >> 1), // i.e. window_width/2 - face_width/2
        top_interior_center_y_px() - (FACE_PIXEL_HEIGHT / 2)
    );
    return pos;
}

static Vec2f counter_bombs_pos_px()
{
    Vec2f pos = vec2f(
        (f32)(BORDER_PIXEL_WIDTH * 2),
        top_interior_center_y_px() - (COUNTER_PIXEL_HEIGHT / 2)
    );
    return pos;
}

static Vec2f counter_timer_pos_px()
{
    Vec2f game_dims = game_dims_px();
    Vec2f pos = vec2f(
        (f32)( (u32)game_dims.x - (BORDER_PIXEL_WIDTH * 2) - COUNTER_PIXEL_WIDTH ),
        top_interior_center_y_px() - (COUNTER_PIXEL_HEIGHT / 2)
    );
    return pos;
}

u32 resize_window_to_game();

void gui_FPS();
bool gui_difficulty(GameParams *params);

void draw_game();
void draw_resize();
void draw_end_game(Board *board);
bool draw_start_game(Board* board);
bool draw_init();

bool game_update_and_render(Input input);
bool game_init();

C_END
