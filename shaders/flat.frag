#version 330 core
out vec4 FragColor;

in vec3 tex_coord;
in vec4 color;

uniform sampler2D tex;
uniform sampler2DArray texarr;
uniform vec4 color_blend = vec4(1.0, 0.0, 0.0, 0.0);

void main()
{
    vec4 tex_color = texture(texarr, tex_coord);
    //if (tex_color.a < 0.1) {
    //    discard;
    //}
    vec3 color_with_blend = tex_color.xyz * (1 - color_blend.a) + color_blend.xyz * color_blend.a;
    vec3 color_with_vert_color = color_with_blend.xyz * (1 - color.a) + color.xyz * color.a;
    vec4 final_color = vec4(color_with_vert_color, tex_color.a);
    FragColor = final_color;
} 
