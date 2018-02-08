#version  330 core
layout(location = 0) in vec3 vertPos;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform vec3 portalNormal;
uniform float boxScale;

out vec2 texCoord;
out vec3 colorMod;


void main()
{
	vec4 pos = P * V * M * vec4(vertPos, 1);
	gl_Position = pos;
	if (portalNormal.x == -1.0) {
		texCoord = vec2(1.0 - (vertPos.z + boxScale) / (2.0 * boxScale), (vertPos.y + boxScale) / (2.0 * boxScale));
		colorMod = vec3(-1.0, 0.0, -1.0);
	}
	else if (portalNormal.x == 1.0) {
		texCoord = (vertPos.zy+vec2(boxScale, boxScale)) / (2.0 * boxScale);
		colorMod = vec3(-1.0, 0.0, -1.0);
	}
	else if (portalNormal.z == -1.0) {
		texCoord = (vertPos.xy+vec2(boxScale, boxScale)) / (2.0 * boxScale);
		colorMod = vec3(-1.0, -1.0, 0.0);
	}
	else if (portalNormal.z == 1.0) {
		texCoord = vec2(1.0 - (vertPos.x + boxScale) / (2.0 * boxScale), (vertPos.y + boxScale) / (2.0 * boxScale));
		colorMod = vec3(-1.0, -1.0, 0.0);
	}
}

