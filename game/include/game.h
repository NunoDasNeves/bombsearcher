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
#define BORDER_WIDTH 32
#define TOP_INTERIOR_HEIGHT ((FACE_PIXEL_HEIGHT) + 32)

#define CELLS_X_OFF BORDER_WIDTH
#define CELLS_Y_OFF ( (TOP_INTERIOR_HEIGHT) + ((BORDER_WIDTH) * 2) )

#define CELLS_NUM_X_EASY 9
#define CELLS_NUM_Y_EASY 9
#define CELLS_NUM_BOMBS_EASY 10

#define INIT_GAME_WINDOW_WIDTH ( ((BORDER_WIDTH) * 2) + ((CELLS_NUM_X_EASY) * (CELL_PIXEL_WIDTH)) )
#define INIT_GAME_WINDOW_HEIGHT ( ((BORDER_WIDTH) * 3) + ((CELLS_NUM_Y_EASY) * (CELL_PIXEL_WIDTH)) + (TOP_INTERIOR_HEIGHT) )

static inline Vec2f get_face_pos()
{
    // TODO get width dynamically
    Vec2f pos = vec2f(
        INIT_GAME_WINDOW_WIDTH/2 - FACE_PIXEL_WIDTH/2,
        BORDER_WIDTH + ((TOP_INTERIOR_HEIGHT - FACE_PIXEL_HEIGHT) >> 1) // i.e. border + top_interior/2 - face_height/2
    );
    return pos;
}

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
    FACE_CLICKED,
    FACE_SCARED,
    FACE_DEAD,
    FACE_COOL
};

typedef struct {
    u32 mouse_x;
    u32 mouse_y;
    bool mouse_left_down;
    bool mouse_right_down;
} Input;

typedef struct {
    Board board;
    Input last_input;
    u64 time_started;
    u8 face_state;
    bool playing;
    u32 pixel_w;
    u32 pixel_h;
    u32 window_scale; // window is scaled down by >>window_scale
    bool window_needs_resize;
} GameState;

extern GameState game_state;

u32 resize_window_to_game(u32 desired_width, u32 desired_height);

void draw_game();
bool draw_start_game(Board* board);
bool draw_init();

bool game_update_and_render(Input input);
bool game_init();

C_END
