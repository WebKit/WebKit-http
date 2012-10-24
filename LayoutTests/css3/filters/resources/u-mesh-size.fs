// Test whether u_meshSize is correctly set; change fragment to color green if yes.
precision mediump float;
uniform vec2 u_meshSize;

bool areFloatsEqual(float a, float b)
{
    const float epsilon = 0.001;
    return (a > (b - epsilon)) && (a < (b + epsilon));
}

bool areVectorsEqual(vec2 a, vec2 b)
{
    return areFloatsEqual(a.x, b.x) && areFloatsEqual(a.y, b.y);
}

void main()
{
    gl_FragColor = areVectorsEqual(u_meshSize, vec2(10.0, 20.0)) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
