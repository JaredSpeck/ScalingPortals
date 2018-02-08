#version 330 core

in vec2 texCoord;
out vec4 color;
uniform sampler2D texBuf;

void main(){
	vec3 texColor = texture( texBuf, vec2(texCoord.x, texCoord.y)).rgb;
   color = vec4(texColor.r * 0.8, texColor.g * 0.8, texColor.b * 0.8, 1.0);
}
