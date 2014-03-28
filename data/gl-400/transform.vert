#version 400 core

#define POSITION	0
#define COLOR		3
#define FRAG_COLOR	0

uniform transform
{
	mat4 MVP;
} Transform;

layout(location = POSITION) in vec4 Position;

out block
{
	vec4 Color;
} Out;

void main()
{	
	gl_Position = Transform.MVP * Position;
	Out.Color = vec4(clamp(vec2(Position), 0.0, 1.0), 0.0, 1.0);
}
