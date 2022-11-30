#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"
#include"mem.h"
#include"game.h"

GameState game_state;

bool game_update_and_render(Input input)
{
    //Input last_input = game_state.last_input;
    log_info("(%u, %u), (%u, %u)", input.mouse_x, input.mouse_y, input.mouse_left_down ? 1 : 0, input.mouse_right_down ? 1 : 0);

    draw_game();
    game_state.last_input = input;
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

