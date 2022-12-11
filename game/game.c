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

static bool game_needs_restart = false;
#ifdef DEBUG
static bool show_debug = false;
#endif

static bool game_start(GameParams params);

enum {
    MOUSE_NONE = 0,
    MOUSE_LEFT_DOWN,
    MOUSE_LEFT_RELEASED,
    MOUSE_RIGHT_RELEASED
};

void check_if_won(Board *board)
{
    u32 num_explored = 0;
    for (u32 i = 0; i < board->num_cells; ++i) {
        Cell *cell = &board->cells[i];
        if (cell->state == CELL_EXPLORED) {
            ASSERT(!cell->is_bomb);
            num_explored++;
        }
    }
    if (num_explored == board->num_cells - board->num_bombs) {
        game_state.playing = false;
        game_state.face_state = FACE_COOL;
    }
}

static void explore(Board *board, Cell *cell)
{
    ASSERT(board);
    ASSERT(cell);
    ASSERT(cell->state == CELL_UNEXPLORED);

    cell->state = CELL_EXPLORED;

    if (cell->is_bomb) {
        // lose the game
        game_state.face_state = FACE_DEAD;
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
        check_if_won(board);
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
    check_if_won(board);
}

void handle_input(Board *board, Input input)
{
    Cell *cell_under_mouse = NULL;
    bool face_is_under_mouse = false;
    Vec2f face_pos = face_pos_px();
    Vec2f cells_offset = cells_offset_px();
    Input last_input = game_state.last_input;

#ifdef DEBUG
    if (!input.debug_key && last_input.debug_key) {
        show_debug = !show_debug;
    }
#endif

    // convert window pixel coords to game pixel coords determined by scaling power
    input.mouse_x <<= game_state.window_scale;
    input.mouse_y <<= game_state.window_scale;

    i64 mouse_x_off = (i64)input.mouse_x - (i64)cells_offset.x;
    i64 mouse_y_off = (i64)input.mouse_y - (i64)cells_offset.y;
    if (mouse_x_off >= 0 && mouse_y_off >= 0) {
        i64 mouse_cell_col = mouse_x_off / CELL_PIXEL_WIDTH;
        i64 mouse_cell_row = mouse_y_off / CELL_PIXEL_HEIGHT;
        // already checked not negative above
        if (mouse_cell_col < board->width && mouse_cell_row < board->height) {
            cell_under_mouse = board_pos_to_cell(board, mouse_cell_col, mouse_cell_row);
        }
    } else if (input.mouse_x >= face_pos.x && input.mouse_x < face_pos.x + FACE_PIXEL_WIDTH &&
               input.mouse_y >= face_pos.y && input.mouse_y < face_pos.y + FACE_PIXEL_HEIGHT
               ){
        face_is_under_mouse = true;
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
    switch (mouse_state) {
        case MOUSE_LEFT_DOWN:
        {
            if (cell_under_mouse && game_state.playing) {
                if (cell_under_mouse && cell_under_mouse->state == CELL_UNEXPLORED) {
                    cell_under_mouse->state = CELL_CLICKED;
                    board->cell_last_clicked = cell_under_mouse;
                    game_state.face_state = FACE_SCARED;
                }
                break;
            }
            if (face_is_under_mouse) {
                game_state.face_clicked = true;
            }
            break;
        }
        case MOUSE_LEFT_RELEASED:
        {
            if (cell_under_mouse && game_state.playing) {
                if (cell_under_mouse->state == CELL_UNEXPLORED) {
                    explore(board, cell_under_mouse);
                    // start timer
                    if (game_state.time_started_ms == UINT64_MAX) {
                        game_state.time_started_ms = SDL_GetTicks64();
                        game_state.time_ms = game_state.time_started_ms;
                    }
                }
                break;
            }
            if (face_is_under_mouse) {
                game_needs_restart = true;
            }
            break;
        }
        case MOUSE_RIGHT_RELEASED:
        {
            if (!cell_under_mouse || !game_state.playing) {
                break;
            } else if (cell_under_mouse->state == CELL_UNEXPLORED) {
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

bool game_update_and_render(Input input)
{
    Board *board = &game_state.board;

/*
 * We draw the menu bar at the start of the frame because
 * resize_window_to_game depends on getting its height
 * (we could also hardcode the height, see start_game())
 */
#ifdef DEBUG
    if (show_debug) {
        gui_debug();
    }
#endif
    game_needs_restart = gui_difficulty(&game_state.params);

    mem_ctx_t mem_ctx = mem_set_context(MEM_CTX_SCRATCH);
    CHECK_LOG(mem_scratch_scope_begin() == 1, true, "unexpected mem scratch scope");

    // TODO maybe have special calibration function to do all this
    // janky resizing stuff that needs to happen after some stuff
    // is already rendered...
    /* Need to do this in render loop so we can get game window
     * decoration size from OS
     */
    if (game_state.window_needs_resize) {
        game_state.window_scale = resize_window_to_game();
        draw_resize();
        game_state.window_needs_resize = false;
    }

    // reset clicked cell to treat it as unexplored
    if (board->cell_last_clicked->state == CELL_CLICKED) {
        board->cell_last_clicked->state = CELL_UNEXPLORED;
    }

    if (game_state.playing) {
        if (game_state.time_started_ms != UINT64_MAX) {
            game_state.time_ms = SDL_GetTicks64();
        }
        // Switch FACE_SCARED back to smile by default
        game_state.face_state = FACE_SMILE;
    }
    game_state.face_clicked = false;

    handle_input(board, input);

    draw_game();
    game_state.last_input = input;

    CHECK_LOG(mem_scratch_scope_end() == 0, true, "unexpected mem scratch scope");

    if (game_needs_restart) {
        game_needs_restart = false;
        game_start(game_state.params);
    }

    mem_ctx = mem_set_context(mem_ctx);
    ASSERT(mem_ctx == MEM_CTX_SCRATCH);

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
    board->num_bombs = num_bombs;
    board->num_cells = num_cells;
    // TODO calloc
    board->cells = mem_alloc(num_cells * sizeof(Cell));
    if (!board->cells) {
        log_error("Failed to allocate board");
        return false;
    }
    memset(board->cells, 0, num_cells * sizeof(Cell));
    board->cell_last_clicked = board->cells;
    board->bomb_clicked = NULL;

    /* place bombs */
    i32 bombs_left = num_bombs;
    ASSERT((u32)bombs_left < num_cells);

    i64 max_iters = 1 << 20;
    i64 iters = max_iters;

    // to generate random numbers we'll mask out the unneeded bits from a call to rand()
    u32 mask = (1 << (32 - ((u32)CLZ_U64(num_cells) - 32))) - 1;
    ASSERT(num_cells < RAND_MAX); // otherwise we can't use this method...
    log_debug("num_cells 0x%x", num_cells);
    log_debug("mask 0x%x", mask);

    while (bombs_left > 0 && iters > 0) {
        u32 rand_bits = num_cells;
        while (rand_bits >= num_cells) {
            u32 rand_num = (u32)rand();
            rand_bits = rand_num & mask;
        }
        // now we have random bits which represent a number less than num_cells
        u32 idx = rand_bits;
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
    log_debug("Used %ld iters", max_iters - iters);

    if (iters <= 0) {
        log_error("Failed to place bombs - RNG is broken!");
        return false;
    }

    return true;
}

static bool game_start(GameParams params)
{
    ASSERT(mem_get_current_context() == MEM_CTX_SCRATCH);
    Board *board = &game_state.board;

    /*
     * We both end the current game and start a new one here
     * TODO if this gets more complicated, should probably
     * have a game_end() function
     */
    draw_end_game(board);
    // end all the scopes

    CHECK_LOG(mem_scratch_scope_end() == -1, false, "unexpected mem scratch scope");
    CHECK_LOG(mem_scratch_scope_begin() == 0, false, "unexpected mem scratch scope");

    if (!board_init(board, params.width, params.height, params.num_bombs)) {
        log_error("Failed to init board");
        return false;
    }
    game_state.time_started_ms = UINT64_MAX;
    game_state.time_ms = UINT64_MAX;
    game_state.face_state = FACE_SMILE;
    game_state.playing = true;
    game_state.window_needs_resize = true;
    // Reset the scale to something really wrong... should be visible if there's a problem
    game_state.window_scale = 99999;
    game_state.params = params;
    // We make a guess here, but the rendering loop is arranged so we don't really have to
    game_state.main_menu_bar_height_window_px = 19;

    if (!draw_start_game(board)) {
        log_error("Failed to init draw state for board");
        return false;
    }

    return true;
}

bool game_init()
{
    if (!draw_init()) {
        log_error("Failed to init draw");
        return false;
    }

    // TODO replace c srand() and rand()
    srand((unsigned int)time(NULL));

    mem_set_context(MEM_CTX_SCRATCH);
    if (!game_start(game_easy)) {
        log_error("Failed to start game");
        return false;
    }
    mem_set_context(MEM_CTX_NOFREE);

    // Try and resize the window correctly before the first frame
    game_state.window_scale = resize_window_to_game();

    // init is done, now use regular allocate by default
    mem_set_context(MEM_CTX_LONGTERM);

    return true;
}

