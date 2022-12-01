// TODO remove these, needed for srand, rand
#include<stdlib.h>
#include<time.h>

#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"
#include"mem.h"
#include"game.h"

GameState game_state;

enum {
    MOUSE_NONE = 0,
    MOUSE_LEFT_DOWN,
    MOUSE_LEFT_RELEASED,
    MOUSE_RIGHT_RELEASED
};

static void explore(Board *board, Cell *cell)
{
    ASSERT(board);
    ASSERT(cell);
    ASSERT(cell->state == CELL_UNEXPLORED);

    cell->state = CELL_EXPLORED;

}

bool game_update_and_render(Input input)
{
    Board *board = &game_state.board;
    Input last_input = game_state.last_input;
    i64 mouse_cell_col = ((i64)input.mouse_x - CELLS_X_OFF) / CELL_PIXEL_WIDTH;
    i64 mouse_cell_row = ((i64)input.mouse_y - CELLS_Y_OFF) / CELL_PIXEL_HEIGHT;

    // reset clicked cell because it's actually unexplored
    if (board->cell_last_clicked->state == CELL_CLICKED) {
        board->cell_last_clicked->state = CELL_UNEXPLORED;
    }

    Cell *cell_under_mouse = NULL;
    if (    mouse_cell_col >= 0 && mouse_cell_col < board->width &&
            mouse_cell_row >= 0 && mouse_cell_row < board->height) {
        cell_under_mouse = board_pos_to_cell(board, mouse_cell_col, mouse_cell_row);
    }
/*
#ifdef DEBUG
    if (!last_input.mouse_left_down && input.mouse_left_down) {
        if (cell_under_mouse) {
            log_info("bombs_around: %u", cell_under_mouse->bombs_around);
        }
        log_info("(%u, %u)", input.mouse_x, input.mouse_y);
        log_info("(%lld, %lld)", (i64)input.mouse_x - CELLS_X_OFF, (i64)input.mouse_y - CELLS_Y_OFF);
        log_info("(%lld, %lld)", mouse_cell_col, mouse_cell_row);
    }
#endif
*/
    // Get the discrete state of the mouse we care about
    u32 mouse_state = MOUSE_NONE;
    if (input.mouse_left_down) {
        mouse_state = MOUSE_LEFT_DOWN;
    } else if (last_input.mouse_left_down) {
        mouse_state = MOUSE_LEFT_RELEASED;
    } else if (!input.mouse_right_down && last_input.mouse_right_down) {
        mouse_state = MOUSE_RIGHT_RELEASED;
    }

    /* Mouse click/drag, and release */
    if (cell_under_mouse && mouse_state != MOUSE_NONE) {
        switch (mouse_state) {
            case MOUSE_LEFT_DOWN:
            {
                if (cell_under_mouse->state == CELL_UNEXPLORED) {
                    cell_under_mouse->state = CELL_CLICKED;
                    board->cell_last_clicked = cell_under_mouse;
                }
                break;
            }
            case MOUSE_LEFT_RELEASED:
            {
                if (cell_under_mouse->state == CELL_UNEXPLORED) {
                    explore(board, cell_under_mouse);
                }
                break;
            }
            case MOUSE_RIGHT_RELEASED:
            {
                if (cell_under_mouse->state == CELL_UNEXPLORED) {
                    cell_under_mouse->state = CELL_FLAGGED;
                } else if (cell_under_mouse->state == CELL_FLAGGED) {
                    cell_under_mouse->state = CELL_UNEXPLORED;
                }
                break;
            }
            default:
                break;
        }
    }

    draw_game();
    game_state.last_input = input;
    return true;
}

static bool board_init(Board *board, u32 width, u32 height, u32 num_bombs)
{
    ASSERT(board);
    ASSERT((u64)width * (u64)height < UINT32_MAX);;

    u32 num_cells = width * height;
    board->width = width;
    board->height = height;
    board->num_cells = num_cells;
    board->cells = mem_alloc(sizeof(Cell) * num_cells);
    if (!board->cells) {
        log_error("Failed to allocate board");
        return false;
    }
    board->cell_last_clicked = board->cells;

    /* place bombs */
    i32 bombs_left = num_bombs;
    ASSERT((u32)bombs_left < num_cells);

    i64 iters = 1 << 30;
    // TODO replace c srand() and rand()
    srand((unsigned int)time(NULL));

    while (bombs_left > 0 && iters > 0) {
        u32 idx = (u32)(((f32)rand()/(f32)RAND_MAX) * (f32)num_cells);
        Cell *cell = &board->cells[idx];
        iters--;
        if (cell->is_bomb) {
            continue;
        }
        /* place bomb and numbers */
        cell->is_bomb = true;
        bombs_left--;
        i64 bomb_c, bomb_r;
        board_idx_to_pos(board, idx, &bomb_c, &bomb_r);
        for (i64 r = bomb_r - 1; r <= bomb_r + 1; ++r) {
            if (r < 0 || r >= board->height) {
                continue;
            }
            for (i64 c = bomb_c - 1; c <= bomb_c + 1; ++c) {
                if (c < 0 || c >= board->width) {
                    continue;
                }
                Cell *neighbor = board_pos_to_cell(board, c, r);
                if (neighbor->is_bomb) {
                    // reset this so bombs next to each other all get 0 for bombs_around
                    neighbor->bombs_around = 0;
                    continue;
                }
                neighbor->bombs_around++;
            }
        }
    }

    if (iters <= 0) {
        log_error("Failed to place bombs - RNG is broken!");
        return false;
    }

    return true;
}

bool game_init()
{
    Board *board = &game_state.board;

    if (!board_init(board, CELLS_NUM_X_EASY, CELLS_NUM_Y_EASY, CELLS_NUM_BOMBS_EASY)) {
        log_error("Failed to init board");
        return false;
    }

    if (!draw_init()) {
        log_error("Failed to init draw");
        return false;
    }

    return true;
}

