precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec4 color_offset;

void main()
{
    // Offset the color value using the uniform parameter that we get from CSS.
    gl_FragColor = texture2D(u_texture, v_texCoord) + color_offset;
}
