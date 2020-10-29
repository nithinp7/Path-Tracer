#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 pos;

void main()
{
    //pos = 0.5 * aPos + vec2(0.5, 0.5);
    //pos = vec2(pos.x, 1 - pos.y);
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}