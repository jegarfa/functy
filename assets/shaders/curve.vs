#version 120

uniform vec3 vPosition, vScale;
uniform float fTime;
varying vec4 vDiffuse, vGlobal, vAmbient;
varying vec3 vDir, vHalfVector;
varying float fDist;
varying vec3 vFuncParams;
varying mat3 mNormalMatrix;
varying vec4 vShadowPos;
uniform mat4 mLightTransform;
uniform sampler2D tAudio;

#define M_PI (3.1415926535897932384626433832795)

/*REPLACE:controlvars*//*END*/

vec3 FunctionC (float a, float t);
float FunctionR (float a, float p, float t);
vec3 FunctionD1 (float a, float t);
vec3 FunctionD2 (float a, float t);
float audio (float x);

void main() {
	vec4 vEyeCoordsPos;
	vec3 vPos;
	vec4 vVertex;
	vec3 vNormalC;
	float fA;
	float fP;
	float fR;
	vec3 vTangent;
	vec3 vSecond;
	vec3 vBinormal;
	vec3 vOffset;
	float fLen;
	float fSin;
	float fCos;
	float fBinormalLength;

	// Calculate new vertex position
	vVertex = gl_Vertex;
	fA = vVertex.x;
	fP = vVertex.y;
	fR = FunctionR (fA, fP, fTime);
	vPos = FunctionC (fA, fTime);
	vTangent = FunctionD1 (fA, fTime);
	vSecond = FunctionD2 (fA, fTime);
	fLen = length (vTangent);
	vTangent = normalize (vTangent);
	vBinormal = cross (vTangent, vSecond);
	
	if (length (vBinormal) < 0.000001) {
		vBinormal = cross (vTangent, normalize( vec3(0.01, 0.1, 1.0)));
	}

	vBinormal = normalize (vBinormal);
	vNormalC = cross (vTangent, vBinormal);
	
	fCos = cos (fP);
	fSin = sin (fP);
	vOffset = (vNormalC * fCos) + (vBinormal * fSin);
	vVertex.x = ((vPos.x + (fR * vOffset.x)) / vScale.x) - vPosition.x;
	vVertex.y = ((vPos.y + (fR * vOffset.y)) / vScale.y) - vPosition.y;
	vVertex.z = ((vPos.z + (fR * vOffset.z)) / vScale.z) - vPosition.z;
	vFuncParams.x = fA;
	vFuncParams.y = fP;
	vFuncParams.z = fR;

	// There appears to be a bug accessing gl_NormalMatrix from the fragment shader
	// We therefore pass it across using a varying variable
	mNormalMatrix = gl_NormalMatrix;

	// Normalise light direction, which is stored in eye space
	vEyeCoordsPos = gl_ModelViewMatrix * vVertex;
	vPos = vec3 (gl_LightSource[0].position - vEyeCoordsPos);
	vDir = normalize (vPos);

	// Set the clipping coordinate to be eye coordinate
	gl_ClipVertex = vEyeCoordsPos;

	// Calculate distance to light source
	fDist = length (vPos);

	// Normalise half vector; this gets sent to the fragment shader
	vHalfVector = normalize (gl_LightSource[0].halfVector.xyz);

	// Calculate diffuse, ambient and global values
	vDiffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
	vAmbient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	vGlobal = gl_LightModel.ambient * gl_FrontMaterial.ambient;
	
	// Specify position of vertex
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vVertex;

	// Calculate position in relatiion to light for shadow calculations
	vShadowPos = mLightTransform * vVertex;

	gl_TexCoord[0].s = 1.0f * (0.1f + (fA / 1.2f));
	gl_TexCoord[0].t = 1.0f * (0.1f + (fP / (M_PI)) / 15.0);
}

float audio (float x) {
	return texture2D (tAudio, vec2(x, 0)).r;
}

vec3 FunctionC (float a, float t) {
	vec3 c;

	c.x = /*REPLACE:curveX*/a/*END*/;
	c.y = /*REPLACE:curveY*/1.0f/*END*/;
	c.z = /*REPLACE:curveZ*/1.0f/*END*/;

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


