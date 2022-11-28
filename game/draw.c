#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"game.h"

#define CELL_WIDTH 20

Color background_color = {{0,0,0,1}};

void draw_cell(u32 col, u32 row, Cell *cell)
{
    f32 pos_x = col * (CELL_WIDTH + 10);
    f32 pos_y = row * (CELL_WIDTH + 10);
    f32 width = CELL_WIDTH;
    f32 height = CELL_WIDTH;

    f32 verts[] = {
        // top left
        pos_x, pos_y, 0,
        //0,0, // texture
        // bottom left
        pos_x, pos_y + height, 0,
        //0,1,
        // top right
        pos_x + width, pos_y, 0,
        //1,0,
        // bottom right
        pos_x + width, pos_y + height, 0,
        //1,1
    };
    GLuint indices[] = {
        0,1,2,
        3,2,1
    };

    //glLineWidth(1);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // init
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(f32),
                 verts, GL_DYNAMIC_DRAW);
    dump_errors();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, // number of floats in a vert
                          GL_FLOAT, GL_FALSE, // don't normalize
                          3 * sizeof(f32), // stride
                          (void*)0);
    //glVertexAttribPointer(1, 4, // num verts
    //                      GL_FLOAT, GL_FALSE, // don't normalize
    //                      (3+2) * sizeof(f32), // stride
    //                      (void*)(3*sizeof(f32)));
    dump_errors();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
                 indices, GL_DYNAMIC_DRAW);

    // usage
    // glUseProgram(shader_id);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, // num indices; num_tris * 3
                   GL_UNSIGNED_INT, 0); // offset

    dump_errors();

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);

    dump_errors();
}

void draw_board(Board *board)
{
    for(u32 r = 0; r < board->height; ++r) {
        u32 r_off = r * board->width;
        for(u32 c = 0; c < board->width; ++c) {
            draw_cell(c, r, &board->cells[r_off + c]);
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

void draw_init()
{

}
