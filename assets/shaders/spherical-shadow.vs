#version 120

uniform vec3 vPosition, vScale;
uniform float fTime;
//varying vec4 vDiffuse, vGlobal, vAmbient;
//varying vec3 vDir, vHalfVector;
//varying float fDist;
//varying vec3 vFuncParams;
//varying mat3 mNormalMatrix;
//varying vec4 vShadowPos;
//uniform mat4 mLightTransform;
uniform sampler2D tAudio;

#define M_PI (3.1415926535897932384626433832795)

/*REPLACE:controlvars*//*END*/

float FunctionR (float a, float p, float t);
float audio (float x);

void main() {
	vec4 vEyeCoordsPos;
//	vec3 vPos;
	vec4 vVertex;
	float fA;
	float fP;
	float fR;

	// Calculate new vertex position
	vVertex = gl_Vertex;
	fA = vVertex.x;
	fP = vVertex.y;
	fR = FunctionR (fA, fP, fTime);
	vVertex.x = (fR * cos (fA) * sin (fP) / vScale.x) - vPosition.x;
	vVertex.y = (fR * sin (fA) * sin (fP) / vScale.y) - vPosition.y;
	vVertex.z = (fR * cos (fP) / vScale.z) - vPosition.z;
//	vFuncParams.x = fA;
//	vFuncParams.y = fP;
//	vFuncParams.z = fR;

	// There appears to be a bug accessing gl_NormalMatrix from the fragment shader
	// We therefore pass it across using a varying variable
//	mNormalMatrix = gl_NormalMatrix;

	// Normalise light direction, which is stored in eye space
	vEyeCoordsPos = gl_ModelViewMatrix * vVertex;
//	vPos = vec3 (gl_LightSource[0].position - vEyeCoordsPos);
//	vDir = normalize (vPos);

	// Set the clipping coordinate to be eye coordinate
	gl_ClipVertex = vEyeCoordsPos;

//	// Calculate distance to light source
//	fDist = length (vPos);

//	// Normalise half vector; this gets sent to the fragment shader
//	vHalfVector = normalize (gl_LightSource[0].halfVector.xyz);

//	// Calculate diffuse, ambient and global values
//	vDiffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
//	vAmbient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
//	vGlobal = gl_LightModel.ambient * gl_FrontMaterial.ambient;
	
	// Specify position of vertex
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vVertex;

//	// Calculate position in relatiion to light for shadow calculations
//	vShadowPos = mLightTransform * vVertex;

//	gl_TexCoord[0].s = 0.1f + (fA / (2.0 * M_PI)) / 1.2f;
//	gl_TexCoord[0].t = 0.1f + (fP / (M_PI)) / 2.2f;
}

float audio (float x) {
	return texture2D (tAudio, vec2(x, 0)).r;
}

float FunctionR (float a, float p, float t) {
	float r;

	r = /*REPLACE:function*/1.0f/*END*/;

	return r;
}


