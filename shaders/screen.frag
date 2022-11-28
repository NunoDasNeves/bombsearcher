#version 330 core

in vec2 tex_coord;

uniform sampler2D tex;

void main()
{
    gl_FragColor = texture2D(tex, tex_coord);
} 
