#version 330 core 
in vec3 fragNor;
out vec4 color;
uniform vec3 objColor;

void main()
{
	vec3 normal = normalize(fragNor);

	// Map normal in the range [-1, 1] to color in range [0, 1];
	vec3 Ncolor = 0.5f * normal + 0.5;
	vec3 fragColor = 0.3 * Ncolor + 0.7 * objColor;
	color = vec4(fragColor, 1.0);
}
