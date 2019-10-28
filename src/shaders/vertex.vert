#version 330 core
#extension GL_ARB_gpu_shader_fp64 : enable
layout (location = 0) in vec3 aPos;

out vec2 RawPos;

void main() {
	gl_Position = vec4(aPos, 1.0);
	RawPos = aPos.xy;
}