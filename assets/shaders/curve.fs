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

float FunctionR (float a, float p, float t);
vec3 FunctionD1 (float a, float t);
vec3 FunctionD2 (float a, float t);
float DifferentialA (float a, float p, float t);
float DifferentialP (float a, float p, float t);
vec4 Colour (float a, float p, float r, float t);
float audio (float x);

void main () {
	vec3 vHalf;
	float fNDotL;
	float fNDotHV;
	vec4 vColour;
	vec4 vColourRaw;
	vec4 vTexColour;
	float fAttenuation;
	vec3 vNormalBidir;
	vec3 vNormal;
	vec3 vNormalC;
	float fDiffA;
	float fDiffP;
	vec3 vTangent;
	vec3 vSecond;
	vec3 vBinormal;
	vec3 vTangentA;
	vec3 vTangentP;
	float fA;
	float fP;
	float fR;
	float fRDerivA;
	float fRDerivP;
	vec3 vN;
	float fLen;
	float fCos;
	float fSin;
	vec3 vBump;
	vec3 vNormInt;
	vec4 vShadowTexPos;
	float fShadowStrength;
	int nShadowX;
	int nShadowY;
	
	fA = vFuncParams.x;
	fP = vFuncParams.y;
	fR = vFuncParams.z;

	// Perform lots of calculations to figure out the normal
	fR = FunctionR (fA, fP, fTime);
	vTangent = FunctionD1 (fA, fTime);
	vSecond = FunctionD2 (fA, fTime);
	fLen = length (vTangent);
	vTangent = normalize (vTangent);
	vBinormal = cross (vTangent, vSecond);
	
	if (length (vBinormal) < 0.000001) {
		vBinormal = cross (vTangent, vec3(0.0, 0.0, 1.0));
	}
	
	vBinormal = normalize (vBinormal);
	vNormalC = cross (vTangent, vBinormal);
	
	// Calculate two tangents to the curve at the point where the fragment touches
	fCos = cos (fP);
	fSin = sin (fP);

	fRDerivA = DifferentialA (fA, fP, fTime);
	fRDerivP = DifferentialP (fA, fP, fTime);

	vTangentA.x = (fRDerivA * fCos);
	vTangentA.y = (fRDerivA * fSin);
	vTangentA.z = fLen;
	
	vTangentP.x = (fRDerivP * fCos) - (fR * fSin);
	vTangentP.y = (fRDerivP * fSin) + (fR * fCos);
	vTangentP.z = 0.0;
	
	// Calculate the normal from the tangents
	vBump = normalize (texture2D (tBumpMap, gl_TexCoord[0].st).xyz * 2.0 - 1.0);
	vBump.x *= -fBumpScale;
	// The y (longitudinal) height has far less effect than the x (radial) height, so we have to scale it accordingly
	vBump.y *= -fBumpScale * 100.0;
	vN = cross (vTangentP, vTangentA);
	//vNormInt = cross (vTangentP, vTangentA);
	//vN = vTangentP * vBump.y;
	//vN += vTangentA * vBump.x;
	//vN += vNormInt * vBump.z;

	//vN = cross (vTangentP, vTangentA);
	vN = normalize (vN);

	vNormal.x = (vNormalC.x * vN.x) + (vBinormal.x * vN.y) + (vTangent.x * vN.z);
	vNormal.y = (vNormalC.y * vN.x) + (vBinormal.y * vN.y) + (vTangent.y * vN.z);
	vNormal.z = (vNormalC.z * vN.x) + (vBinormal.z * vN.y) + (vTangent.z * vN.z);

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

	if (vColourRaw.a <= 0.0) {
		discard;
	}

	// Calculate dot product between normal and light direction
	fNDotL = max (dot (vNormalBidir, normalize (vDir)), 0.0);

	if (fNDotL > 0.0) {
		fAttenuation = 1.0 / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation * fDist + gl_LightSource[0].quadraticAttenuation * fDist * fDist);
		vColour += fAttenuation * (vDiffuse * fNDotL + vAmbient) * vColourRaw;

		vHalf = normalize (vHalfVector);
		fNDotHV = max (dot (vNormalBidir, vHalf), 0.0);
		vColour += fAttenuation * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow (fNDotHV, gl_FrontMaterial.shininess);
	}

	vTexColour = 1.0 - ((1.0 - texture2D (tTexture, gl_TexCoord[0].st)) * fTextureStrength);
	//gl_FragColor = vTexColour * vColour;
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

vec4 Colour (float a, float p, float r, float t) {
	vec4 c;

	c.r = /*REPLACE:red*/1.0f/*END*/;
	c.g = /*REPLACE:green*/1.0f/*END*/;
	c.b = /*REPLACE:blue*/1.0f/*END*/;
	c.a = /*REPLACE:alpha*/1.0f/*END*/;
	
	return c;
}

float FunctionR (float a, float p, float t) {
	float r;

	r = /*REPLACE:function*/1.0f/*END*/;

	return r;
}

vec3 FunctionD1 (float a, float t) {
	vec3 v;

	v.x = /*REPLACE:diffAX*/1.0f/*END*/;
	v.y = /*REPLACE:diffAY*/1.0f/*END*/;
	v.z = /*REPLACE:diffAZ*/1.0f/*END*/;

	return v;
}

vec3 FunctionD2 (float a, float t) {
	vec3 v;

	v.x = /*REPLACE:diff2AX*/1.0f/*END*/;
	v.y = /*REPLACE:diff2AY*/1.0f/*END*/;
	v.z = /*REPLACE:diff2AZ*/1.0f/*END*/;

	return v;
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


