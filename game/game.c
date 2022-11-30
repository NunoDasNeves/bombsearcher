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

    board->width = CELLS_NUM_X_EASY;
    board->height = CELLS_NUM_Y_EASY;
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

