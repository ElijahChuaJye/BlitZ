#version 450

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

//Input from C++ vertex buffer (via location 0 and 1)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

//Output for going to Fragment Shader
layout(location = 0) out vec3 fragColor;

void main() {
    //Using the actual positon from C++ buffer
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    //Forward the color data to fragment shader
    fragColor = inColor;
}