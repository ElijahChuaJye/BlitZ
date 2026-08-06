#version 450

//Input from C++ vertex buffer (via location 0 and 1)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

//Output for going to Fragment Shader
layout(location = 0) out vec3 fragColor;

// Hardcoded positions for a perfect fullscreen triangle
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),   // Top vertex
    vec2(0.5, 0.5),    // Bottom right vertex
    vec2(-0.5, 0.5)    // Bottom left vertex
);

void main() {
    //Using the actual positon from C++ buffer
    gl_Position = vec4(inPosition, 1.0);

    //Forward the color data to fragment shader
    fragColor = inColor;
}