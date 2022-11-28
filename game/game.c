#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"
#include"mem.h"
#include"game.h"

GameState game_state;
Color background_color = {{0,0,0,1}};

bool game_update_and_render()
{
    render_start(background_color);
    //draw_board(game_state.board);
    render_end();
    return true;
}

bool game_init()
{
    Board *board = &game_state.board;

    board->width = 9;
    board->height = 9;
    board->cells = mem_alloc(sizeof(Cell)*9*9);
    if (!board->cells) {
        log_error("Failed to allocate board");
        return false;
    }

    return true;
}

/*
void draw_cell(u32 col, u32 row, Cell *cell)
{
    //Texture *tex = tex_unexplored;
    //switch (cell->state) {
    //    case (CELL_
    //}
    render_2d_quad(Vec2(col * CELL_WIDTH, row * CELL_WIDTH),
                   Vec2(CELL_WIDTH, CELL_WIDTH),
                   NULL, // texture
                   color(),
                   false); // wireframe

}

void draw_board(Board *board)
{
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            draw_cell(c, r, board->cells[r_off + c]);
        }
    }
}
*/
