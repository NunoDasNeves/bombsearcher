#version 330 core

in vec2 tex_coord;

uniform sampler2D screen_texture;

void main()
{
    gl_FragColor = texture2D(screen_texture, tex_coord);
} 
