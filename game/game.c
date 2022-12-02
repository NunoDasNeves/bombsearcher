// TODO remove these, needed for srand, rand
#include<stdlib.h>
#include<time.h>
#include<string.h> // memset

#include<SDL.h>
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

    if (cell->is_bomb) {
        // lose the game
        board->bomb_clicked = cell;
        game_state.playing = false;
        for (u32 i = 0; i < board->num_cells; ++i) {
            Cell *b_cell = &board->cells[i];
            // idk why but bombs under flags don't show
            if (b_cell->is_bomb && b_cell->state != CELL_FLAGGED) {
                b_cell->state = CELL_EXPLORED;
            }
        }
        return;
    } else if (cell->bombs_around > 0) {
        // early exit instead of searching
        return;
    }

    // err just ad hoc it
    u32 q_head = 0;
    u32 q_tail = 0;
    u32 q_len = 0;
    u32 q_size = board->num_cells;
    Cell **frontier = mem_alloc(sizeof(Cell *) * board->num_cells);
    if (frontier == NULL) {
        log_error("Failed to alloc frontier");
        return;
    }
    memset(frontier, 0, sizeof(Cell *) * board->num_cells);

    frontier[0] = cell;
    q_tail++;
    q_len++;
    while(q_len) {
        // pop queue
        Cell *curr = frontier[q_head];
        q_head = (q_head + 1) % q_size;
        q_len--;
        if (curr->bombs_around > 0) {
            continue;
        }
        i64 curr_r,curr_c;
        board_cell_to_pos(board, curr, &curr_c, &curr_r);
        for (i64 r = curr_r - 1; r <= curr_r + 1; ++r) {
            if (r < 0 || r >= board->height) {
                continue;
            }
            for (i64 c = curr_c - 1; c <= curr_c + 1; ++c) {
                if (c < 0 || c >= board->width) {
                    continue;
                }
                if (c == curr_c && r == curr_r) {
                    continue;
                }
                Cell *neighbor = board_pos_to_cell(board, c, r);
                ASSERT(!neighbor->is_bomb);
                // if cell is flagged, don't explore it
                if (neighbor->state == CELL_UNEXPLORED) {
                    neighbor->state = CELL_EXPLORED;
                    //log_error("add %u %u", c, r);
                    // TODO expand queue... should be big enough though
                    ASSERT(q_len < q_size - 1);
                    frontier[q_tail] = neighbor;
                    q_tail = (q_tail + 1) % q_size;
                    q_len++;
                }
            }
        }
    }
}

void handle_input(Board *board, Input input)
{
    Cell *cell_under_mouse = NULL;
    Input last_input = game_state.last_input;
    i64 mouse_cell_col = ((i64)input.mouse_x - CELLS_X_OFF) / CELL_PIXEL_WIDTH;
    i64 mouse_cell_row = ((i64)input.mouse_y - CELLS_Y_OFF) / CELL_PIXEL_HEIGHT;

    if (    mouse_cell_col >= 0 && mouse_cell_col < board->width &&
            mouse_cell_row >= 0 && mouse_cell_row < board->height) {
        cell_under_mouse = board_pos_to_cell(board, mouse_cell_col, mouse_cell_row);
    }

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
                    ASSERT(board->bombs_left > INT64_MIN);
                    board->bombs_left--;
                } else if (cell_under_mouse->state == CELL_FLAGGED) {
                    cell_under_mouse->state = CELL_UNEXPLORED;
                    ASSERT(board->bombs_left < INT64_MAX);
                    board->bombs_left++;
                }
                break;
            }
            default:
                break;
        }
    }

}

bool game_update_and_render(Input input)
{
    Board *board = &game_state.board;

    // reset clicked cell because it's actually unexplored
    if (board->cell_last_clicked->state == CELL_CLICKED) {
        board->cell_last_clicked->state = CELL_UNEXPLORED;
    }

    if (game_state.playing) {
        handle_input(board, input);
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
    board->bombs_left = (i64)num_bombs;
    board->num_cells = num_cells;
    board->cells = mem_alloc(sizeof(Cell) * num_cells);
    if (!board->cells) {
        log_error("Failed to allocate board");
        return false;
    }
    board->cell_last_clicked = board->cells;
    board->bomb_clicked = NULL;

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

bool game_start()
{
    Board *board = &game_state.board;

    if (!board_init(board, CELLS_NUM_X_EASY, CELLS_NUM_Y_EASY, CELLS_NUM_BOMBS_EASY)) {
        log_error("Failed to init board");
        return false;
    }
    game_state.time_started = SDL_GetTicks64();
    game_state.face_state = FACE_SMILE;
    game_state.playing = true;

    return true;
}

bool game_init()
{
    if (!game_start()) {
        log_error("Failed to start game");
        return false;
    }

    if (!draw_init()) {
        log_error("Failed to init draw");
        return false;
    }

    return true;
}

