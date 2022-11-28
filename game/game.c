#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"
#include"mem.h"
#include"game.h"

GameState game_state;

bool game_update_and_render()
{
    draw_game();
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

    if (!draw_init()) {
        log_error("Failed to init draw");
    }

    return true;
}

