#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"game.h"
#include"mem.h"

#define CELL_WIDTH 40

typedef struct {
    u32 num_tris;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
} Geom;

Color background_color = {{0,0,0,1}};
Geom *cell_geoms;

#define VERTEX_POS_ARRAY_ATTRIB 0
#define VERTEX_TEX_ARRAY_ATTRIB 1
typedef struct {
    f32 pos[3];
    f32 tex[2];
} Vertex;

Texture *cell_texture;

void init_cell_geom(Geom *geom, u32 col, u32 row, Cell *cell)
{
    f32 pos_x = (f32)col * (CELL_WIDTH + 10);
    f32 pos_y = (f32)row * (CELL_WIDTH + 10);
    f32 width = CELL_WIDTH;
    f32 height = CELL_WIDTH;

    Vertex verts[] = {
        {
            // top left
            {pos_x, pos_y, 0},
            {0,0}
        }, {
            // bottom left
            {pos_x, pos_y + height, 0},
            {0,1}
        }, {
            // top right
            {pos_x + width, pos_y, 0},
            {1,0}
        }, {
            // bottom right
            {pos_x + width, pos_y + height, 0},
            {1,1}
        }
    };
    GLuint indices[] = {
        0,1,2,
        3,2,1
    };

    // init
    glGenVertexArrays(1, &geom->vao);
    glGenBuffers(1, &geom->vbo);
    glGenBuffers(1, &geom->ebo);
    glBindVertexArray(geom->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts),
                 verts, GL_STATIC_DRAW);
    dump_errors();
    glEnableVertexAttribArray(VERTEX_POS_ARRAY_ATTRIB);
    glEnableVertexAttribArray(VERTEX_TEX_ARRAY_ATTRIB);
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
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
                 indices, GL_STATIC_DRAW);

    dump_errors();

    /*
    glDeleteBuffers(1, &geom->vbo);
    glDeleteBuffers(1, &geom->ebo);
    glDeleteVertexArrays(1, &geom->vao);

    dump_errors();
    */
}

void draw_cell(Geom *geom, Cell *cell)
{
    shader_set_texture(shader_flat, cell_texture); // this does glUseProgram(shader_id);

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

    glBindVertexArray(geom->vao);
    glDrawElements(GL_TRIANGLES, 6, // num indices; num_tris * 3
                   GL_UNSIGNED_INT, 0); // offset
}

void draw_board(Board *board)
{
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            draw_cell(&cell_geoms[r_off + c], &board->cells[r_off + c]);
        }
    }
}

void draw_game()
{
    render_start(background_color);
    draw_board(&game_state.board);
    dump_errors();
    render_end();
}

bool draw_init()
{
    Board *board = &game_state.board;

    cell_geoms = mem_alloc(sizeof(Geom)*board->width*board->height);
    if (!cell_geoms) {
        return false;
    }
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            init_cell_geom(&cell_geoms[r_off + c],
                           c, r, &board->cells[r_off + c]);
        }
    }

    cell_texture = load_texture("assets/cell.png");

    return true;
}
