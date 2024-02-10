#version 460
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vb;
} PushConstants;

void main()
{	
	Vertex v = PushConstants.vb.vertices[gl_VertexIndex];
	
	debugPrintfEXT("idx = %d", gl_VertexIndex);
	debugPrintfEXT("in vtx pos  = %v3f", v.position);

	gl_Position = PushConstants.render_matrix * vec4(v.position, 1.0f);
	outColor = v.color.xyz;
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}