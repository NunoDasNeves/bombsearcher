#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"game.h"
#include"mem.h"

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

#define TEX_LOAD(s, e, n) \
    do {    \
        textures[TEX_##e] = load_texture("assets/"s".png"); \
        if (!textures[TEX_##e]) {   \
            log_error("Failed to load texture: "s); \
            textures[TEX_##e] = empty_texture;  \
        }   \
    } while (0);

enum {
    TEXTURES(TEX_ENUM)
    TEX_NUM_TEXTURES
};

Texture *textures[TEX_NUM_TEXTURES] = {0};

typedef struct {
    Texture *tex;
    /* These are floats because they're texture coords */
    f32 t_x;
    f32 t_y;
    f32 t_width;
    f32 t_height;
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

    // half pixel offset
    f32 x_off = 0.5F/(f32)tex->width;
    f32 y_off = 0.5F/(f32)tex->height;
    f32 t_width = (f32)spr_width / (f32)tex->width;
    f32 t_height = (f32)spr_height / (f32)tex->height;
    u32 i = 0;
    for (u32 r = 0; r < rows; ++r) {
        for (u32 c = 0; c < cols; ++c) {
            Sprite *spr = &sheet->sprites[i++];
            spr->tex = tex;
            spr->t_x = x_off + (f32)c * t_width;
            spr->t_y = y_off + (f32)r * t_height;
            spr->t_width = t_width;
            spr->t_height = t_height;
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
            {spr->t_x, spr->t_y}
        }, {
            // bottom left
            {pos_x, pos_y + height, 0},
            {spr->t_x, spr->t_y + spr->t_height}
        }, {
            // top right
            {pos_x + width, pos_y, 0},
            {spr->t_x + spr->t_width, spr->t_y}
        }, {
            // bottom right
            {pos_x + width, pos_y + height, 0},
            {spr->t_x + spr->t_width, spr->t_y + spr->t_height}
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

typedef union {
    struct {
        f32 x,y;
    };
    f32 data[2];
} Vec2f;

static inline Vec2f vec2f(f32 x, f32 y)
{
    Vec2f v = {{x,y}};
    return v;
}

void geom_load_sprite(Geom *geom, Vec2f pos, Vec2f dims, Sprite *spr)
{
    ASSERT(geom);
    ASSERT(spr);

    // TODO
    ASSERT(geom);

    GLuint indices[] = {
        0,1,2,
        3,2,1
    };

    Vertex verts[] = {
        {
            // top left
            {pos.x, pos.y, 0},
            {spr->t_x, spr->t_y}
        }, {
            // bottom left
            {pos.x, pos.y + dims.y, 0},
            {spr->t_x, spr->t_y + spr->t_height}
        }, {
            // top right
            {pos.x + dims.x, pos.y, 0},
            {spr->t_x + spr->t_width, spr->t_y}
        }, {
            // bottom right
            {pos.x + dims.x, pos.y + dims.y, 0},
            {spr->t_x + spr->t_width, spr->t_y + spr->t_height}
        }
    };

    geom_load(geom, verts, sizeof(verts), indices, sizeof(indices), 2);
}

void draw_face()
{
    Geom geom;
    // TODO get width dynamically
    Vec2f pos = vec2f(
        INIT_GAME_WINDOW_WIDTH/2 - FACE_PIXEL_WIDTH/2,
        TOP_SECTION_BORDER + TOP_SECTION_INTERIOR_HEIGHT/2 - FACE_PIXEL_HEIGHT/2
    );
    Vec2f dims = vec2f(FACE_PIXEL_WIDTH, FACE_PIXEL_HEIGHT);
    ASSERT(game_state.face_state <= FACE_COOL);
    Sprite *spr = &face_sheet.sprites[game_state.face_state];

    shader_set_texture(shader_flat, TEX_GET(FACE)); // this does glUseProgram(shader_id);
    geom_init(&geom);
    geom_load_sprite(&geom, pos, dims, spr);
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_tris * 3, // num indices
                   GL_UNSIGNED_INT, 0); // offset
    geom_deinit(&geom);
}

void draw_game()
{
    render_start(background_color);
    draw_board(&game_state.board);
    draw_face();
    render_end();
}

bool draw_init()
{
    Board *board = &game_state.board;

    cell_geoms = mem_alloc(sizeof(Geom)*board->num_cells);
    if (!cell_geoms) {
        return false;
    }
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            geom_init(&cell_geoms[r_off + c]);
        }
    }

    TEXTURES(TEX_LOAD);

    if (!init_spritesheet_uniform(&numbers_sheet, TEX_GET(NUMBERS),
                                  8, 1, 64, 64)) {
        log_error("Could not init numbers sprite sheet");
        return false;
    }
    if (!init_spritesheet_uniform(&face_sheet, TEX_GET(FACE),
                                  5, 1, 128, 128)) {
        log_error("Could not init numbers sprite sheet");
        return false;
    }

    //render_resize(CELLS_SECTION_BORDER * 2 + board->width * CELL_PIXEL_WIDTH,
    //              CELLS_SECTION_BORDER * 2 + board->height * CELL_PIXEL_WIDTH + TOP_SECTION_HEIGHT);

    return true;
}
