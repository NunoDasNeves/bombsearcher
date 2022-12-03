#pragma once
#include"types.h"
#include"vec.h"

C_BEGIN

/*
 * Two sections, top and cells, each with an interior border on all sides which
 * is added to the total height/width
 * Total width is board.width * CELL_PIXEL_WIDTH + CELLS_SECTION_BORDER * 2
 */
#define TOP_SECTION_INTERIOR_HEIGHT 128
#define TOP_SECTION_BORDER 16
#define TOP_SECTION_HEIGHT ( (TOP_SECTION_INTERIOR_HEIGHT) + ((TOP_SECTION_BORDER) * 2) )
#define CELL_PIXEL_WIDTH 64
#define CELL_PIXEL_HEIGHT CELL_PIXEL_WIDTH
#define FACE_PIXEL_WIDTH 108
#define FACE_PIXEL_HEIGHT FACE_PIXEL_WIDTH
#define CELLS_SECTION_BORDER 16

#define CELLS_X_OFF CELLS_SECTION_BORDER
#define CELLS_Y_OFF ( (TOP_SECTION_HEIGHT) + (CELLS_SECTION_BORDER) )

#define CELLS_NUM_X_EASY 9
#define CELLS_NUM_Y_EASY 9
#define CELLS_NUM_BOMBS_EASY 10

#define INIT_GAME_WINDOW_WIDTH ( ((CELLS_SECTION_BORDER) * 2) + ((CELLS_NUM_X_EASY) * (CELL_PIXEL_WIDTH)) )
#define INIT_GAME_WINDOW_HEIGHT ( ((CELLS_SECTION_BORDER) * 2) + ((CELLS_NUM_Y_EASY) * (CELL_PIXEL_WIDTH)) + (TOP_SECTION_HEIGHT) )

static inline Vec2f get_face_pos()
{
    // TODO get width dynamically
    Vec2f pos = vec2f(
        INIT_GAME_WINDOW_WIDTH/2 - FACE_PIXEL_WIDTH/2,
        TOP_SECTION_BORDER + TOP_SECTION_INTERIOR_HEIGHT/2 - FACE_PIXEL_HEIGHT/2
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
} GameState;

extern GameState game_state;

void draw_game();
bool draw_init();

bool game_update_and_render(Input input);
bool game_init();

C_END
