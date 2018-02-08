#version 330 core

in vec2 texCoord;
in vec3 colorMod;
out vec4 color;
uniform sampler2D texBuf;

void main(){
	vec3 texColor = texture( texBuf, texCoord).rgb;
    color = vec4(texColor.r * 0.8 + colorMod.r * 0.1, texColor.g * 0.8 + colorMod.g *0.1, texColor.b * 0.8 + colorMod.b * 0.1, 1.0);
}