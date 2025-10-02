#version 120

#define SHADOW_STRENGTH (0.7)

uniform vec3 vPosition, vScale;
uniform float fTime;
varying vec4 vDiffuse, vGlobal, vAmbient;
varying vec3 vDir, vHalfVector;
varying float fDist;
varying vec3 vFuncParams;
varying mat3 mNormalMatrix;
uniform sampler2D tTexture;
uniform sampler2D tBumpMap;
uniform sampler2DShadow tShadow;
uniform sampler2D tAudio;
uniform float fBumpScale;
uniform float fTextureStrength;
varying vec4 vShadowPos;
uniform mat4 mLightTransform;
uniform float fShadowBias;
uniform float fShadowBlurX;
uniform float fShadowBlurY;

/*REPLACE:controlvars*//*END*/

float DifferentialA (float a, float p, float t);
float DifferentialP (float a, float p, float t);
vec4 Colour (float a, float p, float r, float t);
float audio (float x);

void main () {
	vec3 vHalf;
	float fNDotL;
	float fNDotHV;
	vec4 vColour;
	vec4 vTexColour;
	vec4 vColourRaw;
	float fAttenuation;
	vec3 vNormalBidir;
	vec3 vNormal;
	float fDiffA;
	float fDiffP;
	vec3 vTangentA;
	vec3 vTangentP;
	float fA;
	float fP;
	float fR;
	vec3 vBump;
	vec3 vNormInt;
	vec4 vShadowTexPos;
	float fShadowStrength;
	int nShadowX;
	int nShadowY;
	
	fA = vFuncParams.x;
	fP = vFuncParams.y;
	fR = vFuncParams.z;

	// Calculate two tangents to the curve at the point where the fragment touches
	fDiffA = DifferentialA (fA, fP, fTime);
	fDiffP = DifferentialP (fA, fP, fTime);

	vTangentA.x = sin (fP) * ((fDiffA * cos (fA)) - (fR * sin (fA)));
	vTangentA.y = sin (fP) * ((fDiffA * sin (fA)) + (fR * cos (fA)));
	vTangentA.z = fDiffA * cos (fP);

	vTangentP.x = cos (fA) * ((fDiffP * sin (fP)) + (fR * cos (fP)));
	vTangentP.y = sin (fA) * ((fDiffP * sin (fP)) + (fR * cos (fP)));
	vTangentP.z = (fDiffP * cos (fP)) - (fR * sin (fP));

	// Calculate the normal from the tangents
	vNormal = cross (vTangentA, vTangentP);

  // Since sin(p) is zero when p is zero, we need to fix the normal
	if (fP == 0.0f) {
		vNormal = vec3(0.0f, 0.0f, -1.0f);
	}
	vNormal = normalize (mNormalMatrix * vNormal);

	if (gl_FrontFacing) {
		vNormalBidir = vNormal;
	}
	else {
		vNormalBidir = -vNormal;
	}

	// Calculate the material colour
	vColourRaw = Colour (vFuncParams.x, vFuncParams.y, vFuncParams.z, fTime);
	vColour = vGlobal * vColourRaw;

	// Calculate dot product between normal and light direction
	fNDotL = max (dot (vNormalBidir, normalize (vDir)), 0.0);

	if (fNDotL > 0.0) {
		fAttenuation = 1.0 / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation * fDist + gl_LightSource[0].quadraticAttenuation * fDist * fDist);
		vColour += fAttenuation * (vDiffuse * fNDotL + vAmbient) * vColourRaw;

		vHalf = normalize (vHalfVector);
		fNDotHV = max (dot (vNormalBidir, vHalf), 0.0);
		vColour += fAttenuation * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow (fNDotHV, gl_FrontMaterial.shininess);
	}

	gl_FragColor = vColour;

	// Convert the fragment coordinate into a shadow texture coordinate
	vShadowTexPos = ((vShadowPos / vShadowPos.w) + 1.0f) / 2.0f;
	// Add slight bias to avoid shadow acne
	vShadowTexPos.z -= fShadowBias;

	// Blur the shadow
	fShadowStrength = 0.0;
	for (nShadowX = -1; nShadowX < 2; nShadowX++) {
		for (nShadowY = -1; nShadowY < 2; nShadowY++) {
			fShadowStrength += shadow2D (tShadow, vShadowTexPos.xyz + vec3(nShadowX * fShadowBlurX, nShadowY * fShadowBlurY, 0.0f)).x;
		}
	}

	// Darken the colour in proportion to the shadow level	
	gl_FragColor *= vec4 ((1.0 - SHADOW_STRENGTH), (1.0 - SHADOW_STRENGTH), (1.0 - SHADOW_STRENGTH), (1.0 - SHADOW_STRENGTH)) + (SHADOW_STRENGTH * fShadowStrength / 9.0f);

	gl_FragColor.a = vColourRaw.a;
}

float audio (float x) {
	return texture2D (tAudio, vec2(x, 0)).r;
}

float DifferentialA (float a, float p, float t) {
	float r;
	
	r = /*REPLACE:diffA*/0.0f/*END*/;
	
	return r;
}

float DifferentialP (float a, float p, float t) {
	float r;

	r = /*REPLACE:diffP*/0.0f/*END*/;
	
	return r;
}

vec4 Colour (float a, float p, float r, float t) {
	vec4 c;

	c.r = /*REPLACE:red*/1.0f/*END*/;
	c.g = /*REPLACE:green*/1.0f/*END*/;
	c.b = /*REPLACE:blue*/1.0f/*END*/;
	c.a = /*REPLACE:alpha*/1.0f/*END*/;
	
	return c;
}

