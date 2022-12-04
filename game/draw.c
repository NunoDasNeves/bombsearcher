#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"game.h"
#include"mem.h"
#include"vec.h"

typedef struct {
    u32 num_tris;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
} Geom;

Color background_color = COLOR_RGB8(40,40,40);
Geom *cell_geoms;

#define VERTEX_POS_ARRAY_ATTRIB 0
#define VERTEX_TEX_ARRAY_ATTRIB 1
typedef struct {
    f32 pos[3];
    f32 tex[2];
} Vertex;

/* macro magic! put all texture stuff here */
#define TEXTURES(op) \
    op("cell_up", CELL_UP, 0) \
    op("cell_down", CELL_DOWN, 1) \
    op("flag", FLAG, 2) \
    op("bomb", BOMB, 3) \
    op("numbers", NUMBERS, 4) \
    op("face", FACE, 5)

#define TEX_ENUM(s, e, n) \
    TEX_##e = n,

/* e.g. TEX_GET(CELL_DOWN); */
#define TEX_GET(e) \
    textures[TEX_##e]

enum {
    TEXTURES(TEX_ENUM)
    TEX_NUM_TEXTURES
};

Texture *textures[TEX_NUM_TEXTURES] = {0};

bool __load_texture(const char *name, const char *path, u32 slot)
{
    ASSERT(slot < TEX_NUM_TEXTURES);
    textures[slot] = load_texture(path);
    if (!textures[slot]) {
        log_error("Failed to load texture \"%s\"", name);
        textures[slot] = empty_texture;
        return false;
    }
    return true;
}
#define TEX_LOAD(s, e, n) \
    __load_texture(s, "assets/"s".png", TEX_##e);

typedef struct {
    Texture *tex;
    /* Texture coords */
    Vec2f pos_tx;
    Vec2f dims_tx;
    /* Sprite dims */
    Vec2f dims;
} Sprite;

static Sprite spr_default = {NULL, 0, 0, 1, 1};

typedef struct {
    Texture *tex;
    Sprite *sprites;
    u32 num_sprites;
    u32 cols;
    u32 rows;
} SpriteSheet;

static SpriteSheet numbers_sheet;
static SpriteSheet face_sheet;

static bool init_spritesheet_uniform(SpriteSheet *sheet, Texture *tex, u32 cols, u32 rows, u32 spr_width, u32 spr_height)
{
    ASSERT(sheet);
    ASSERT(tex);

    // TODO not sure if we need these, but whatevs
    i64 tex_x_excess = tex->width - cols * spr_width;
    i64 tex_y_excess = tex->height - rows * spr_height;

    if (tex_x_excess < 0) {
        log_error("Texture width %u insufficient for %u sprites of width %u", tex->width, cols, spr_width);
        return false;
    }
    if (tex_y_excess < 0) {
        log_error("Texture height %u insufficient for %u sprites of height %u", tex->height, rows, spr_height);
        return false;
    }

    sheet->tex = tex;
    sheet->sprites = NULL;
    sheet->num_sprites = cols * rows;
    sheet->cols = cols;
    sheet->rows = rows;
    sheet->sprites = mem_alloc(sheet->num_sprites * sizeof(Sprite));
    if (sheet->sprites == NULL) {
        log_error("Failed to alloc sprites");
        return false;
    }

    /*
     * Half pixel offset to make pixel sampling consistent
     * We have tex coords 0->1 mapping to pixels 0->N
     * But we want to select the middle of the pixels,
     * i.e. 0.5->(N-0.5), so this is how to determine that
     *
     * e.g.
     * | are pixel boundaries
     * {} show the sprites' boundaries
     * So this is 2 sprites, 4 pixels wide each:
     * 0        1        2        3        4        5        6        7           <- pixel indices
     * |{       |        |        |       }|{       |        |        |       }|
     * 0      0.125     0.25    0.375     0.5               ...                1  <- tex coords
     * |{       |        |        |       }|{       |        |        |       }|
     * Each 4-pixel sprite occupies 0.5 units in tex coords
     * Each pixel occupies 0.5/4 = 0.125 in tex coords
     * Need to start halfway through a pixel; so 0.125/2 = 0.0625
     *   0.0625   0.1875   0.3125  0.4375    0.5625  ....                         <- tex coords we want to use
     *     \/       \/       \/       \/       \/
     * |{       |        |        |       }|{       |        |        |       }|
     * Naming of below stuff:
     * spr_width_tx == sprite width in texture coords
     * tex_width_px == texture width in pixels
     * ...etc.
     * so we have:
     * spr_width_tx = spr_width_px / tex_width_px
     * pixel_width_tx = spr_width_tx / spr_width_px
     * i.e.:
     * pixel_width_tx = (spr_width_px/tex_width_px)/spr_width_px
     *                = 1/tex_width_px
     * start_offset_tx = pixel_width_tx/2
     * start_offset_tx = 1/tex_width_px/2
     * start_offset_tx = 0.5/tex_width_px
     * We also need the width to add to the offset, and we need it in texture units.
     * It's not just the sprite with in texture units.
     * See the above diagram: start at 0.0625, then + 0.375 = 0.4375
     * The width we want is 0.375, not 0.5 which is the full width of the sprite in texture units
     * And that's just 0.5 - 0.125
     * i.e:
     * end_width_tx = spr_width_tx - pixel_width_tx
     * And the starting offset of the next sprite is start_offset_tx + spr_width_tx
     */
    f32 spr_width_tx = (f32)spr_width / (f32)tex->width;
    f32 px_width_tx = 1.0F / (f32)tex->width;
    f32 x_off_tx = 0.5F/(f32)tex->width;
    f32 end_width_tx = spr_width_tx - px_width_tx;

    f32 spr_height_tx = (f32)spr_height / (f32)tex->height;
    f32 px_height_tx = 1.0F / (f32)tex->height;
    f32 y_off_tx = 0.5F/(f32)tex->height;
    f32 end_height_tx = spr_height_tx - px_height_tx;

    u32 i = 0;
    for (u32 r = 0; r < rows; ++r) {
        for (u32 c = 0; c < cols; ++c) {
            Sprite *spr = &sheet->sprites[i++];
            spr->tex = tex;
            // starting offset
            spr->pos_tx.x = x_off_tx + (f32)c * spr_width_tx;
            spr->pos_tx.y = y_off_tx + (f32)r * spr_height_tx;
            // width until midway through last pixel
            spr->dims_tx.x = end_width_tx;
            spr->dims_tx.y = end_height_tx;
            spr->dims.x = (f32)spr_width;
            spr->dims.y = (f32)spr_height;
        }
    }

    return true;
}

static void geom_load(Geom *geom, Vertex *verts, u32 size_of_verts, GLuint *indices, u32 size_of_indices, u32 num_tris)
{
    ASSERT(geom);
    ASSERT(geom->vao);
    ASSERT(geom->vbo);
    ASSERT(geom->ebo);
    geom->num_tris = num_tris;

    glBindVertexArray(geom->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBufferData(GL_ARRAY_BUFFER, size_of_verts, verts, GL_STATIC_DRAW);
    dump_errors();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_of_indices, indices, GL_STATIC_DRAW);
    dump_errors();
}

static void geom_init(Geom *geom)
{
    ASSERT(geom);
    /* create objects */
    glGenVertexArrays(1, &geom->vao);
    glGenBuffers(1, &geom->vbo);
    glGenBuffers(1, &geom->ebo);

    glBindVertexArray(geom->vao);
    /* Enable the attributes for the current vao */
    glEnableVertexAttribArray(VERTEX_POS_ARRAY_ATTRIB);
    glEnableVertexAttribArray(VERTEX_TEX_ARRAY_ATTRIB);
    /* Binding the vbo lets us set the attribs for it */
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    /*
     * The attribs are set for the current vbo, and also
     * store the currently bound vbo in the vao
     */
    glVertexAttribPointer(VERTEX_POS_ARRAY_ATTRIB,
                          3, // number of floats in a vert
                          GL_FLOAT, GL_FALSE, // don't normalize
                          sizeof(Vertex), // stride
                          (void*)0);
    glVertexAttribPointer(VERTEX_TEX_ARRAY_ATTRIB,
                          2, // number of floats in a texture coord
                          GL_FLOAT, GL_FALSE, // don't normalize
                          sizeof(Vertex), // stride
                          (void*)offsetof(Vertex, tex));
    dump_errors();
}

static void geom_deinit(Geom *geom)
{
    ASSERT(geom);
    glDeleteBuffers(1, &geom->vbo);
    glDeleteBuffers(1, &geom->ebo);
    glDeleteVertexArrays(1, &geom->vao);

    dump_errors();
}

static void update_cell_geom(Geom *geom, u32 col, u32 row, Sprite *spr)
{
    ASSERT(geom);

    GLuint indices[] = {
        0,1,2,
        3,2,1
    };

    f32 pos_x = CELLS_X_OFF + (f32)col * CELL_PIXEL_WIDTH;
    f32 pos_y = CELLS_Y_OFF + (f32)row * CELL_PIXEL_WIDTH;
    f32 width = CELL_PIXEL_WIDTH;
    f32 height = CELL_PIXEL_WIDTH;

    Vertex verts[] = {
        {
            // top left
            {pos_x, pos_y, 0},
            {spr->pos_tx.x, spr->pos_tx.y}
        }, {
            // bottom left
            {pos_x, pos_y + height, 0},
            {spr->pos_tx.x, spr->pos_tx.y + spr->dims_tx.y}
        }, {
            // top right
            {pos_x + width, pos_y, 0},
            {spr->pos_tx.x + spr->dims_tx.x, spr->pos_tx.y}
        }, {
            // bottom right
            {pos_x + width, pos_y + height, 0},
            {spr->pos_tx.x + spr->dims_tx.x, spr->pos_tx.y + spr->dims_tx.y}
        }
    };

    geom_load(geom, verts, sizeof(verts), indices, sizeof(indices), 2);
}

static void draw_cell_back(Board *board, Geom *geom, Cell *cell)
{
    ASSERT(board);
    ASSERT(geom);
    ASSERT(cell);

    i64 col, row;
    board_cell_to_pos(board, cell, &col, &row);

    Texture *tex = TEX_GET(CELL_UP);
    if (cell->state == CELL_EXPLORED || cell->state == CELL_CLICKED) {
        tex = TEX_GET(CELL_DOWN);
    }
    update_cell_geom(geom, (u32)col, (u32)row, &spr_default);
    shader_set_texture(shader_flat, tex); // this does glUseProgram(shader_id);
    if (board->bomb_clicked == cell) {
        shader_set_color(shader_flat, color_red());
    } else {
        shader_set_color(shader_flat, color_none());
    }

    glBindVertexArray(geom->vao);
    glDrawElements(GL_TRIANGLES, 6, // num indices; num_tris * 3
                   GL_UNSIGNED_INT, 0); // offset
}

static void draw_cell_front(Board *board, Geom *geom, Cell *cell)
{
    ASSERT(board);
    ASSERT(geom);
    ASSERT(cell);

    i64 col, row;
    board_cell_to_pos(board, cell, &col, &row);

    Texture *tex = TEX_GET(FLAG);
    if (cell->state != CELL_FLAGGED) {
        if (cell->state == CELL_EXPLORED) {
            if (cell->is_bomb) {
                tex = TEX_GET(BOMB);
            } else if (cell->bombs_around > 0) {
                tex = TEX_GET(NUMBERS);
                ASSERT(cell->bombs_around <= 8);
                Sprite *spr = &numbers_sheet.sprites[cell->bombs_around - 1];
                update_cell_geom(geom, (u32)col, (u32)row, spr);
            } else {
                return;
            }
        } else {
            return;
        }
    }
    shader_set_texture(shader_flat, tex); // this does glUseProgram(shader_id);
    shader_set_color(shader_flat, color_none());

    glBindVertexArray(geom->vao);
    glDrawElements(GL_TRIANGLES, 6, // num indices; num_tris * 3
                   GL_UNSIGNED_INT, 0); // offset
}

static void draw_board(Board *board)
{
    //glLineWidth(1);
    /* not needed really
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    */
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* NOTE we gotta draw things back to front! */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(u32 i = 0; i < board->num_cells; ++i) {
        Geom *geom = &cell_geoms[i];
        Cell *cell = &board->cells[i];
        draw_cell_back(board, geom, cell);
        draw_cell_front(board, geom, cell);
    }
}

void geom_load_sprite(Geom *geom, Vec2f pos, Vec2f dims, Sprite *spr)
{
    ASSERT(geom);
    ASSERT(spr);

    GLuint indices[] = {
        0,1,2,
        3,2,1
    };

    Vertex verts[] = {
        {
            // top left
            {pos.x, pos.y, 0},
            {spr->pos_tx.x, spr->pos_tx.y}
        }, {
            // bottom left
            {pos.x, pos.y + dims.y, 0},
            {spr->pos_tx.x, spr->pos_tx.y + spr->dims_tx.y}
        }, {
            // top right
            {pos.x + dims.x, pos.y, 0},
            {spr->pos_tx.x + spr->dims_tx.x, spr->pos_tx.y}
        }, {
            // bottom right
            {pos.x + dims.x, pos.y + dims.y, 0},
            {spr->pos_tx.x + spr->dims_tx.x, spr->pos_tx.y + spr->dims_tx.y}
        }
    };

    geom_load(geom, verts, sizeof(verts), indices, sizeof(indices), 2);
}

static void draw_sprite(Sprite *sprite, Vec2f pos, Vec2f dims)
{
    Geom geom;

    shader_set_texture(shader_flat, sprite->tex);
    geom_init(&geom);
    geom_load_sprite(&geom, pos, sprite->dims, sprite);
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, // num indices
                   GL_UNSIGNED_INT, 0); // offset
    geom_deinit(&geom);
}

static void draw_face()
{
    ASSERT(game_state.face_state <= FACE_COOL);

    Vec2f pos = get_face_pos();
    Sprite *spr = &face_sheet.sprites[game_state.face_state];

    draw_sprite(spr, pos, spr->dims);
}

void draw_game()
{
    render_start(background_color);
    draw_board(&game_state.board);
    draw_face();
    render_end();
}

bool draw_start_game(Board* board)
{
    ASSERT(board);

    shader_set_transform_pixels(shader_flat, game_state.pixel_w, game_state.pixel_h);

    cell_geoms = mem_alloc(sizeof(Geom)*board->num_cells);
    if (!cell_geoms) {
        return false;
    }
    for (u32 i = 0; i < board->num_cells; ++i) {
        geom_init(&cell_geoms[i]);
    }

    return true;
}

void draw_end_game(Board *board)
{
    ASSERT(board);
    /*
     * NOTE
     * when this is first called, board->num_cells will be 0
     * so this will safely do nothing
     */
    for (u32 i = 0; i < board->num_cells; ++i) {
        geom_deinit(&cell_geoms[i]);
    }
}

bool draw_init()
{
    TEXTURES(TEX_LOAD);

    if (!init_spritesheet_uniform(&numbers_sheet, TEX_GET(NUMBERS),
                                  8, 1, CELL_PIXEL_WIDTH, CELL_PIXEL_HEIGHT)) {
        log_error("Could not init numbers sprite sheet");
        return false;
    }
    if (!init_spritesheet_uniform(&face_sheet, TEX_GET(FACE),
                                  5, 1, FACE_PIXEL_WIDTH, FACE_PIXEL_HEIGHT)) {
        log_error("Could not init numbers sprite sheet");
        return false;
    }

    return true;
}
