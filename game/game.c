#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"
#include"mem.h"
#include"game.h"

GameState game_state;

bool game_update_and_render(Input input)
{
    Board *board = &game_state.board;
    Input last_input = game_state.last_input;
    i64 mouse_cell_col = ((i64)input.mouse_x - CELLS_X_OFF) / CELL_PIXEL_WIDTH;
    i64 mouse_cell_row = ((i64)input.mouse_y - CELLS_Y_OFF) / CELL_PIXEL_HEIGHT;
    i64 i;

#ifdef DEBUG
    if (!last_input.mouse_left_down && input.mouse_left_down) {
        log_info("(%u, %u)", input.mouse_x, input.mouse_y);
        log_info("(%lld, %lld)", (i64)input.mouse_x - CELLS_X_OFF, (i64)input.mouse_y - CELLS_Y_OFF);
        log_info("(%lld, %lld)", mouse_cell_col, mouse_cell_row);
    }
#endif

    for (i = 0; i < board->width * board->height; ++i) {
        Cell *cell = &board->cells[i];
        cell->state = CELL_UNEXPLORED;
    }
    if (    mouse_cell_col >= 0 && mouse_cell_col < board->width &&
            mouse_cell_row >= 0 && mouse_cell_row < board->height) {
        board->cells[mouse_cell_row * board->width + mouse_cell_col].state = CELL_EXPLORED;
    }

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

