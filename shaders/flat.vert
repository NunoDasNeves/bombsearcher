#version 330 core
layout (location = 0) in vec3 in_vert;
//layout (location = 1) in vec2 in_tex_coord;

out vec2 tex_coord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(in_vert, 1.0);
    tex_coord = vec2(0,0);//in_tex_coord;
}