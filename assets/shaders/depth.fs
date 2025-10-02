#version 120

uniform vec3 vPosition, vScale;
uniform float fTime;
uniform sampler2D framebufferTexture;
uniform sampler2D framebufferDepth;

void main () {
	gl_FragColor = 1.0 - log (7.0 - (6.0 * texture2D (framebufferDepth, gl_TexCoord[0].st))) / 1.0;
}

