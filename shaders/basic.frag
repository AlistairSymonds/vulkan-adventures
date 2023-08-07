#version 460 core
layout (location = 0) out vec4 FragColor;

void main()
{
    float r = 1.0f;
    if (gl_FragCoord.x < (1920/2)){
        r = (gl_FragCoord.x/1920) * r;
    }

    FragColor = vec4(0.5f, r, 0.2f, 1.0f);
} 