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

