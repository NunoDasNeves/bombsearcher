#include"types.h"
#include"glad/glad.h"
#include"render.h"
#include"game.h"

#define CELL_WIDTH 20

void draw_cell(u32 col, u32 row, Cell *cell)
{
    f32 pos_x = col * CELL_WIDTH;
    f32 pos_y = row * CELL_WIDTH;
    f32 width = CELL_WIDTH;
    f32 height = CELL_WIDTH;
    // TODO
}

void draw_board(Board *board)
{
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            draw_cell(c, r, &board->cells[r_off + c]);
        }
    }
}


