#version 400

varying vec3 fragment_color;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragment_color, 1);
}
