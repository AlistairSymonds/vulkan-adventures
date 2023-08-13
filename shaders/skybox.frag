#version 460
//layout(binding = 1) uniform sampler2D texSampler;

layout (location = 0) out vec4 FragColor;


layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 view_matrix;
} PushConstants;

void main()
{
	vec3 color;
	color.x = PushConstants.view_matrix[0][0];
	color.y = PushConstants.view_matrix[1][1];
	color.z = PushConstants.view_matrix[2][2];
	FragColor = vec4(color, 1.0f);
}