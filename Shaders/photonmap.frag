#version 430 core
out vec4 FragColor;
  
void main()
{ 
    FragColor = texture(screenTexture, TexCoords);
}