#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"game.h"
#include"mem.h"
#include"vec.h"
#include"file.h"

typedef struct {
    u32 num_tris;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
} Geom;

Color background_color = COLOR_RGB8(153,153,153);

#define VERTEX_POS_ARRAY_ATTRIB 0
#define VERTEX_TEX_ARRAY_ATTRIB 1
#define VERTEX_COLOR_ARRAY_ATTRIB 2
typedef struct {
    f32 pos[3];
    f32 tex[3];
    f32 color[4];
} Vertex;

/*
 * 6.7.2.2 Enumeration specifiers
 * "
 * If the first enumerator has no =, the value of its enumeration constant is 0.
 * Each subsequent enumerator with no = defines its enumeration constant as the
 * value of the constant expression obtained by adding 1 to the value of the
 * previous enumeration constant.
 * "
 * i.e.
 * No need to specify the first enum member as '= 0' if we just want the values to
 * be 0, 1, 2...etc
 * This is relevant to the below macros because there used to be a number specified,
 * but turns out it's not needed
 */

/* macro magic! put all texture stuff here */
/*
#define TEXTURES(op) \
    op("cell", CELL) \
    op("numbers", NUMBERS) \
    op("face", FACE) \
    op("border", BORDER) \
    op("counter", COUNTER) \
    op("numbers_7seg", NUMBERS_7SEG)

#define TEX_ENUM(s, e) \
    TEX_##e,

// e.g. TEX_GET(CELL);
#define TEX_GET(e) \
    (textures[TEX_##e])

enum {
    TEXTURES(TEX_ENUM)
    TEX_NUM_TEXTURES
};

glTexture *textures[TEX_NUM_TEXTURES] = {0};

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
#define TEX_LOAD(s, e) \
    __load_texture(s, "assets/"s".png", TEX_##e);
*/

/* macro magic! put all texture stuff here */
#define SPRITESHEETIMAGES(op) \
    op("cell", CELL) \
    op("numbers", NUMBERS) \
    op("face", FACE) \
    op("border", BORDER) \
    op("numbers_7seg", NUMBERS_7SEG) \
    op("counter", COUNTER)

#define SPRSHIMG_ENUM(s, e) \
    SPRSHIMG_##e,

#define SPRSHIMG_LOAD(s, e) \
    __load_sprshimg(s, "assets/"s".png", SPRSHIMG_##e);

#define SPRSHIMG_GET(e) \
    (&sprshimgs[SPRSHIMG_##e])

enum {
    SPRITESHEETIMAGES(SPRSHIMG_ENUM)
    SPRSHIMG_NUM_SPRITESHEETIMAGES
};

static SpriteSheetImage sprshimgs[SPRSHIMG_NUM_SPRITESHEETIMAGES] = {0};
static glTextureArray *tex_array;

bool __load_sprshimg(const char *name, const char *filename, u32 slot)
{
    SpriteSheetImage *sprshimg = &sprshimgs[slot];

    log_debug("Loading sprshimg \"%s\"", name);
    sprshimg->data = image_file_read(filename, &sprshimg->size, &sprshimg->width, &sprshimg->height);
    if (sprshimg->data == NULL) {
        log_error("Failed to load spritesheet image \"%s\"", name);
        return false;
    }
    log_debug("Loaded sprshimg \"%s\", size %u, dims (%u %u)", name, sprshimg->size, sprshimg->width, sprshimg->height);

    return true;
}

typedef struct {
    Vec2f uv_start;
    Vec2f uv_size;
    Vec2f size_px;
    u32 layer;
} Sprite;

typedef struct {
    Sprite *sprites;
    u32 layer;
    u32 num_sprites;
    u32 cols;
    u32 rows;
} SpriteSheet;

// so far, name == tex_name, but maybe it won't always...?
#define SPRITESHEETS(op) \
    op("cell", CELL, 2, 2, CELL_PIXEL_WIDTH, CELL_PIXEL_HEIGHT) \
    op("numbers", NUMBERS, 8, 1, CELL_PIXEL_WIDTH, CELL_PIXEL_HEIGHT) \
    op("face", FACE, 5, 2, FACE_PIXEL_WIDTH, FACE_PIXEL_HEIGHT) \
    op("border", BORDER, 4, 4, BORDER_PIXEL_WIDTH, BORDER_PIXEL_HEIGHT) \
    op("numbers_7seg", NUMBERS_7SEG, 10, 1, NUM_7SEG_PIXEL_WIDTH, NUM_7SEG_PIXEL_HEIGHT) \
    op("counter", COUNTER, 1, 1, COUNTER_PIXEL_WIDTH, COUNTER_PIXEL_HEIGHT)

#define SPRSH_ENUM(name, enum_name, cols, rows, spr_width, spr_height) \
    SPRSH_##enum_name,

#define SPRSH_LOAD(name, enum_name, cols, rows, spr_width, spr_height) \
    __init_spritesheet(name, SPRSH_GET(enum_name), SPRSHIMG_GET(enum_name), cols, rows, spr_width, spr_height);

#define SPRSH_GET(name) \
    (&spritesheets[SPRSH_##name])

enum {
    SPRITESHEETS(SPRSH_ENUM)
    SPRSH_NUM_SPRSHEETS
};

static SpriteSheet spritesheets[SPRSH_NUM_SPRSHEETS] = {0};

static bool init_spritesheet_uniform(SpriteSheet *sheet, SpriteSheetImage *sprshimg, u32 cols, u32 rows, u32 spr_width, u32 spr_height);
static void __init_spritesheet(const char* name, SpriteSheet *sheet, SpriteSheetImage *sprshimg,
                               u32 cols, u32 rows, u32 spr_width, u32 spr_height)
{
    if (!init_spritesheet_uniform(sheet, sprshimg, cols, rows, spr_width, spr_height)) {
        log_error("Could not init cell sprite sheet %s", name);
        ASSERT(1==0);
    }
}

static Sprite *get_sprite(SpriteSheet *sheet, u32 col, u32 row)
{
    ASSERT(sheet);
    ASSERT(col < sheet->cols);
    ASSERT(row < sheet->rows);

    return &sheet->sprites[row * sheet->cols + col];
}

/*
 * Little more succinct e.g.
 * get_sprite(&SPRSH(CELL), 0, 0)
 * vs
 * SPRITE(CELL, 0, 0)
 */
#define SPRITE(spritesheet_name, col, row) \
    get_sprite(SPRSH_GET(spritesheet_name), col, row)

#define SPRITEI(spritesheet_name, idx) \
    (&(SPRSH_GET(spritesheet_name)->sprites[idx]))

static bool init_spritesheet_uniform(SpriteSheet *sheet, SpriteSheetImage *sprshimg, u32 cols, u32 rows, u32 spr_width, u32 spr_height)
{
    ASSERT(sheet);
    ASSERT(sprshimg);
    ASSERT(cols > 0);
    ASSERT(rows > 0);
    ASSERT(spr_width > 0);
    ASSERT(spr_height > 0);
    ASSERT(sprshimg->width >= cols * spr_width);
    ASSERT(sprshimg->height >= rows * spr_height);
    ASSERT((u64)cols * (u64)rows < UINT32_MAX);

    sheet->layer = sprshimg->layer;
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
    f32 spr_width_tx = sprshimg->max_u / (f32)cols;//(f32)spr_width / (f32)tex->width;
    f32 px_width_tx = 1.0F / (f32)tex_array->width;//tex->width;
    f32 x_off_tx = 0.5F/(f32)tex_array->width;
    f32 end_width_tx = spr_width_tx - px_width_tx;

    f32 spr_height_tx = sprshimg->max_v / (f32)rows;//(f32)spr_height / (f32)tex->height;
    f32 px_height_tx = 1.0F / (f32)tex_array->height;
    f32 y_off_tx = 0.5F/(f32)tex_array->height;
    f32 end_height_tx = spr_height_tx - px_height_tx;

    //log_debug("sprshimg max_u %f", sprshimg->max_u);
    //log_debug("sprsh w %u spr_w %u max_w %u spr_w_tx %f x_off_tx %f", cols*spr_width, spr_width, tex_array->width, spr_width_tx, x_off_tx);

    u32 i = 0;
    for (u32 r = 0; r < rows; ++r) {
        for (u32 c = 0; c < cols; ++c) {
            Sprite *spr = &sheet->sprites[i++];
            // starting offset
            spr->uv_start.x = x_off_tx + (f32)c * spr_width_tx;
            spr->uv_start.y = y_off_tx + (f32)r * spr_height_tx;
            // width until midway through last pixel
            spr->uv_size.x = end_width_tx;
            spr->uv_size.y = end_height_tx;
            spr->size_px.x = (f32)spr_width;
            spr->size_px.y = (f32)spr_height;
            spr->layer = sprshimg->layer;
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
    glEnableVertexAttribArray(VERTEX_COLOR_ARRAY_ATTRIB);
    /* Binding the vbo lets us set the attribs for it */
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    /*
     * The attribs are set for the current vbo, and also
     * store the currently bound vbo in the vao
     */
    Vertex v;
    glVertexAttribPointer(VERTEX_POS_ARRAY_ATTRIB,
                          ARRAY_LEN(v.pos), // number of floats in a vert
                          GL_FLOAT, GL_FALSE, // don't normalize
                          sizeof(Vertex), // stride
                          (void*)0);
    glVertexAttribPointer(VERTEX_TEX_ARRAY_ATTRIB,
                          ARRAY_LEN(v.tex), // number of floats in a texture coord
                          GL_FLOAT, GL_FALSE, // don't normalize
                          sizeof(Vertex), // stride
                          (void*)offsetof(Vertex, tex));
    glVertexAttribPointer(VERTEX_COLOR_ARRAY_ATTRIB,
                          ARRAY_LEN(v.color), // number of floats in a texture coord
                          GL_FLOAT, GL_FALSE, // don't normalize
                          sizeof(Vertex), // stride
                          (void*)offsetof(Vertex, color));
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

void get_sprite_verts_indices(Vec2f pos, Vec2f dims, Sprite *spr, Vertex *verts, GLuint *indices, GLuint index_offset)
{
    ASSERT(spr);
    ASSERT(verts);
    ASSERT(indices);

    static const GLuint spr_indices[] = {
        0,1,2,
        3,2,1
    };
    for (u32 i = 0; i < ARRAY_LEN(spr_indices); ++i) {
        indices[i] = spr_indices[i] + index_offset;
    }

    Vertex spr_verts[] = {
        {
            // top left
            {pos.x, pos.y, 0},
            {spr->uv_start.x, spr->uv_start.y, spr->layer}
        }, {
            // bottom left
            {pos.x, pos.y + dims.y, 0},
            {spr->uv_start.x, spr->uv_start.y + spr->uv_size.y, spr->layer}
        }, {
            // top right
            {pos.x + dims.x, pos.y, 0},
            {spr->uv_start.x + spr->uv_size.x, spr->uv_start.y, spr->layer}
        }, {
            // bottom right
            {pos.x + dims.x, pos.y + dims.y, 0},
            {spr->uv_start.x + spr->uv_size.x, spr->uv_start.y + spr->uv_size.y, spr->layer}
        }
    };
    for (u32 i = 0; i < ARRAY_LEN(spr_verts); ++i) {
        verts[i] = spr_verts[i];
    }
}

static void geom_load_sprite(Geom *geom, Vec2f pos, Vec2f dims, Sprite *spr)
{
    ASSERT(geom);
    ASSERT(spr);

    Vertex verts[4];
    GLuint indices[6];
    get_sprite_verts_indices(pos, dims, spr, verts, indices, 0);
    geom_load(geom, verts, sizeof(verts), indices, sizeof(indices), 2);
}

static void draw_sprite(Sprite *sprite, Vec2f pos, Vec2f dims)
{
    ASSERT(sprite);

    Geom geom;

    geom_init(&geom);
    geom_load_sprite(&geom, pos, dims, sprite);
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, // num indices
                   GL_UNSIGNED_INT, 0); // offset
    geom_deinit(&geom);
}

static Vec2f cell_pixel_pos(Board *board, Cell *cell)
{

    i64 col, row;
    Vec2f pos;
    Vec2f offset = cells_offset_px();

    board_cell_to_pos(board, cell, &col, &row);
    pos.x = offset.x + (f32)col * CELL_PIXEL_WIDTH;
    pos.y = offset.y + (f32)row * CELL_PIXEL_WIDTH;

    return pos;
}

enum {
    SPR_CELL_UP = 0,
    SPR_CELL_DOWN = 1,
    SPR_CELL_FLAG = 2,
    SPR_CELL_BOMB = 3
};

static Sprite *spr_cell_back(Board *board, Cell *cell)
{
    ASSERT(board);
    ASSERT(cell);

    Sprite *spr = SPRITEI(CELL, SPR_CELL_UP);
    if (cell->state == CELL_EXPLORED || cell->state == CELL_CLICKED) {
        spr = SPRITEI(CELL, SPR_CELL_DOWN);
    }

    return spr;
}

static Sprite *spr_cell_front(Board *board, Cell *cell)
{
    ASSERT(board);
    ASSERT(cell);

    Sprite *spr = SPRITEI(CELL, SPR_CELL_FLAG);
    if (cell->state != CELL_FLAGGED) {
        if (cell->state == CELL_EXPLORED) {
            if (cell->is_bomb) {
                spr = SPRITEI(CELL, SPR_CELL_BOMB);
            } else if (cell->bombs_around > 0) {
                ASSERT(cell->bombs_around <= 8);
                spr = SPRITEI(NUMBERS, cell->bombs_around - 1);
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    return spr;
}

static void draw_cells(Board *board)
{
    Geom geom;

    Vertex *verts = mem_alloc(sizeof(Vertex) * 4 * board->num_cells);
    if (!verts) {
        log_error("Failed to alloc cell verts");
        return;
    }
    GLuint *indices = mem_alloc(sizeof(GLuint) * 2 * 3 * board->num_cells);
    if (!indices) {
        log_error("Failed to alloc cell indices");
        return;
    }
    Vertex *curr_verts = verts;
    GLuint *curr_indices = indices;
    u32 index_off = 0;
    u32 num_tris = 0;
    u32 num_verts = 0;
    u32 num_indices = 0;

    for (u32 i = 0; i < board->num_cells; ++i) {
        Cell *cell = &board->cells[i];
        Vec2f pos = cell_pixel_pos(board, cell);
        Sprite *spr_back = spr_cell_back(board, cell);
        get_sprite_verts_indices(pos, spr_back->size_px, spr_back, curr_verts, curr_indices, index_off);
        curr_verts += 4;
        num_verts += 4;
        curr_indices += 6;
        num_indices += 6;
        index_off += 4;
        num_tris += 2;
    }

    geom_init(&geom);
    geom_load(&geom, verts, num_verts * sizeof(verts[0]), indices, num_indices * sizeof(indices[0]), num_tris);
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, // num indices
                   GL_UNSIGNED_INT, 0); // offset
    geom_deinit(&geom);

    // red bomb background
    if (board->bomb_clicked != NULL) {
        Sprite *spr = SPRITEI(CELL, 0);
        shader_set_color(shader_flat, color_red());
        draw_sprite(spr, cell_pixel_pos(board, board->bomb_clicked), spr->size_px);
        shader_set_color(shader_flat, color_none());
    }

    curr_verts = verts;
    curr_indices = indices;
    index_off = 0;
    num_tris = 0;
    num_verts = 0;
    num_indices = 0;
    for (u32 i = 0; i < board->num_cells; ++i) {
        Cell *cell = &board->cells[i];
        Sprite *spr_front = spr_cell_front(board, cell);
        if (!spr_front) {
            continue;
        }
        Vec2f pos = cell_pixel_pos(board, cell);
        get_sprite_verts_indices(pos, spr_front->size_px, spr_front, curr_verts, curr_indices, index_off);
        curr_verts += 4;
        num_verts += 4;
        curr_indices += 6;
        num_indices += 6;
        index_off += 4;
        num_tris += 2;
    }

    geom_init(&geom);
    geom_load(&geom, verts, num_verts * sizeof(verts[0]), indices, num_indices * sizeof(indices[0]), num_tris);
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, // num indices
                   GL_UNSIGNED_INT, 0); // offset
    geom_deinit(&geom);
}

static void draw_face()
{
    ASSERT(game_state.face_state <= FACE_COOL);

    Vec2f pos_back = face_pos_px();
    Vec2f pos_front = pos_back;
    u32 back_spr_idx = 0;

    if (game_state.face_clicked) {
        // move face to match button press when clicked
        pos_front = vec2f_add(pos_front, vec2f(3, 3));
        back_spr_idx = 1;
    }

    Sprite *spr_back = SPRITE(FACE, back_spr_idx, 1);
    draw_sprite(spr_back, pos_back, spr_back->size_px);

    Sprite *spr_front = SPRITE(FACE, game_state.face_state, 0);
    draw_sprite(spr_front, pos_front, spr_front->size_px);
}

static void draw_borders(Board *board)
{
    // this is gonna be pretty manual...
    Vec2f game_dims = game_dims_px();
    // top left of borders (below the menu bar)
    Vec2f border_orig = vec2f(0, menu_bar_y_offset_px());
    // size of a border sprite
    Vec2f spr_dims = vec2f(BORDER_PIXEL_WIDTH, BORDER_PIXEL_HEIGHT);
    // total border tiles in each direction
    u32 num_borders_x = (u32)(game_dims.x - border_orig.x)/BORDER_PIXEL_WIDTH;
    u32 num_borders_y = (u32)(game_dims.y - border_orig.y)/BORDER_PIXEL_HEIGHT;
    //log_error("(%u %u)", num_borders_x, num_borders_y);
    //log_error("%f", game_dims.y - border_orig.y);
    // offset to get to right border from left border
    Vec2f offset_right = vec2f((f32)((num_borders_x - 1) * BORDER_PIXEL_WIDTH), 0);
    // offset to get to mid border from top
    Vec2f offset_mid = vec2f(0, BORDER_PIXEL_HEIGHT + TOP_INTERIOR_HEIGHT);
    // offset to get to bottom border from top
    Vec2f offset_bot = vec2f(0, (f32)((num_borders_y - 1) * BORDER_PIXEL_HEIGHT));
    // top corners
    Vec2f pos_top_left = border_orig;
    Vec2f pos_top_right = vec2f_add(border_orig, offset_right);
    draw_sprite(SPRITE(BORDER, 0, 1), pos_top_left, spr_dims);
    draw_sprite(SPRITE(BORDER, 1, 1), pos_top_right, spr_dims);
    // middle joins
    Vec2f pos_mid_left = vec2f_add(border_orig, offset_mid);
    Vec2f pos_mid_right = vec2f_add(pos_mid_left, offset_right);
    draw_sprite(SPRITE(BORDER, 0, 2), pos_mid_left, spr_dims);
    draw_sprite(SPRITE(BORDER, 1, 2), pos_mid_right, spr_dims);
    // bottom corners
    Vec2f pos_bot_left = vec2f_add(border_orig, offset_bot);
    Vec2f pos_bot_right = vec2f_add(pos_bot_left, offset_right);
    draw_sprite(SPRITE(BORDER, 2, 1), pos_bot_left, spr_dims);
    draw_sprite(SPRITE(BORDER, 3, 1), pos_bot_right, spr_dims);
    // vert
    Sprite *spr_vert = SPRITE(BORDER, 0, 0);
    u32 num_borders_y_top = TOP_INTERIOR_HEIGHT / BORDER_PIXEL_HEIGHT;
    u32 num_borders_y_cells = (board->height * CELL_PIXEL_HEIGHT) / BORDER_PIXEL_HEIGHT;
    Vec2f pos_left = pos_top_left;
    Vec2f pos_right = vec2f_add(pos_left, offset_right);
    for (u32 i = 0; i < num_borders_y_top; ++i) {
        // inc first to skip top row
        pos_left.y += (f32)BORDER_PIXEL_HEIGHT;
        pos_right.y += (f32)BORDER_PIXEL_HEIGHT;
        draw_sprite(spr_vert, pos_left, spr_dims);
        draw_sprite(spr_vert, pos_right, spr_dims);
    }
    // skip middle joins
    pos_left.y += (f32)BORDER_PIXEL_HEIGHT;
    pos_right.y += (f32)BORDER_PIXEL_HEIGHT;
    for (u32 i = 0; i < num_borders_y_cells; ++i) {
        // inc first to skip top row
        pos_left.y += (f32)BORDER_PIXEL_HEIGHT;
        pos_right.y += (f32)BORDER_PIXEL_HEIGHT;
        draw_sprite(spr_vert, pos_left, spr_dims);
        draw_sprite(spr_vert, pos_right, spr_dims);
    }
    // horiz
    Sprite *spr_horiz = SPRITE(BORDER, 1, 0);
    u32 num_borders_x_interior = num_borders_x - 2;
    Vec2f pos_top = pos_top_left;
    Vec2f pos_mid = pos_mid_left;
    Vec2f pos_bot = pos_bot_left;
    for (u32 i = 0; i < num_borders_x_interior; ++i) {
        pos_top.x += (f32)BORDER_PIXEL_WIDTH;
        pos_mid.x += (f32)BORDER_PIXEL_WIDTH;
        pos_bot.x += (f32)BORDER_PIXEL_WIDTH;
        draw_sprite(spr_horiz, pos_top, spr_dims);
        draw_sprite(spr_horiz, pos_mid, spr_dims);
        draw_sprite(spr_horiz, pos_bot, spr_dims);
    }
}

void draw_counter(u32 n, Vec2f pos)
{
    draw_sprite(SPRITEI(COUNTER, 0), pos, vec2f(COUNTER_PIXEL_WIDTH, COUNTER_PIXEL_HEIGHT));
    pos = vec2f_add(pos, vec2f(10, 10));
    pos = vec2f_add(pos, vec2f((NUM_7SEG_PIXEL_WIDTH + 2) * 2, 0));
    for (u32 i = 0; i < 3; ++i) {
        i64 digit = n % 10;
        draw_sprite(SPRITEI(NUMBERS_7SEG, digit), pos, vec2f(NUM_7SEG_PIXEL_WIDTH, NUM_7SEG_PIXEL_HEIGHT));
        pos = vec2f_sub(pos, vec2f(NUM_7SEG_PIXEL_WIDTH + 2, 0));
        n /= 10;
    }
}

void draw_counters()
{
    Board *board = &game_state.board;
    // stop at 0, no negative numbers
    i64 n = MAX(board->bombs_left, 0);
    ASSERT(n <= COUNTER_MAX);
    draw_counter((u32)n, counter_bombs_pos_px());

    u64 t = game_state.time_ms;
    if (t == UINT64_MAX) {
        t = 0;
    } else {
        ASSERT(t >= game_state.time_started_ms);
        t -= game_state.time_started_ms;
        t /= 1000;
        t = MIN(t, COUNTER_MAX);
    }
    draw_counter((u32)t, counter_timer_pos_px());
}

void draw_game()
{
    render_start(background_color);

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

    shader_set_texture_array(shader_flat, tex_array);

    draw_cells(&game_state.board);
    draw_face();
    draw_borders(&game_state.board);
    draw_counters();

    render_end();
}

void draw_resize()
{
    Vec2f dims = game_dims_px();
    shader_set_transform_pixels(shader_flat, dims.x, dims.y);
}

bool draw_start_game(Board* board)
{
    ASSERT(board);

    return true;
}

void draw_end_game(Board *board)
{
    ASSERT(board);
}

bool draw_init()
{
    SPRITESHEETIMAGES(SPRSHIMG_LOAD);
    tex_array = create_texture_array(sprshimgs, ARRAY_LEN(sprshimgs));
    SPRITESHEETS(SPRSH_LOAD);

    return true;
}
