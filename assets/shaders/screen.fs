#version 120

uniform vec3 vPosition, vScale;
uniform float fTime;
uniform sampler2D framebufferTexture;
uniform sampler2D framebufferDepth;

// Blur based on focus: 0 = near; 1 = far
// 0 ... fFocusNear ... fFocusFar ... 1

// Blur near from 0 (max) to fFocusNear (none)
uniform float fFocusNear = 0.99;
// Blur far from fFocusFar (min) to 1 (max)
uniform float fFocusFar = 0.99;
// The smaller the value the greater the blur
const float fFocusScaleNear = 10.0;
const float fFocusScaleFar = 320.0;
const float fDarkenMax = 0.5;

void main () {
	int nXPos;
	int nYPos;
	vec4 vBlur;
	vec2 vTexPos;
	vec3 vTexSize;
	float fDepth;
	float fAmount;
	float fDepthScaled;
	float fDarken;
	
	fDepth = texture2D (framebufferDepth, gl_TexCoord[0].st).x;

	fDepthScaled = 0.0;
	fAmount = 0.0;
	fDarken = 1.0;

	// Blur based on focus: 0 = near; 1 = far
	if (fDepth < fFocusNear) {
		// Too near to the camera
		fDepthScaled += (fFocusNear - fDepth) / fFocusNear;
		fAmount += (fDepthScaled) / fFocusScaleNear;
		fDarken *= 1.0;
	}
	if (fDepth > fFocusFar) {
		// Too far from the camera
		fDepthScaled += ((fDepth - fFocusFar) / (1.0 - fFocusFar));
		fAmount += (fDepthScaled) / fFocusScaleFar;
		fDarken *= fDarkenMax + ((1.0 - fDarkenMax) * (1.0 - fDepthScaled));
	}
	
	vBlur = vec4 (0.0);
	for (nXPos = -3; nXPos <= 3; nXPos++) {
		for (nYPos = -3; nYPos <= 3; nYPos++) {
			vTexPos.s = gl_TexCoord[0].s + (nXPos * fAmount);
			vTexPos.t = gl_TexCoord[0].t + (nYPos * fAmount);
			vBlur += texture2D (framebufferTexture, vTexPos) / 49.0;
		}
	}
   
	vBlur.x *= fDarken;
	vBlur.y *= fDarken;
	vBlur.z *= fDarken;
   
	gl_FragColor = vBlur;
}

