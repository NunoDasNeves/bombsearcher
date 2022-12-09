#version 330 core
layout (location = 0) in vec3 in_vert;
layout (location = 1) in vec3 in_tex_coord;
layout (location = 2) in vec4 in_color;

out vec3 tex_coord;
out vec4 color;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(in_vert, 1.0);
    tex_coord = in_tex_coord;
    color = in_color;
}
