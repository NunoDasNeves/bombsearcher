#pragma once
#include"types.h"

C_BEGIN

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
    u32 width;
    u32 height;
} Board;

enum {
    FACE_SMILE = 0,
    FACE_SCARED,
    FACE_DEAD,
    FACE_CLICKED,
    FACE_COOL
};

typedef struct {
    Board board;
    u32 bombs_left;
    u32 time;
    u8 face_state;
    bool busy_animating;
} GameState;

extern GameState game_state;

void draw_cell(u32 col, u32 row, Cell *cell);
void draw_board(Board *board);
void draw_game();
void draw_init();

bool game_update_and_render();
bool game_init();

C_END
