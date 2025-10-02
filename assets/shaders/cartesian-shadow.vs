#version 120

uniform vec3 vPosition, vScale;
uniform float fTime;
//varying vec4 vDiffuse, vGlobal, vAmbient;
//varying vec3 vDir, vHalfVector;
//varying float fDist;
varying vec3 vFuncParams;
//varying mat3 vNormalMatrix;
uniform sampler2D tAudio;

/*REPLACE:controlvars*//*END*/

float FunctionZ (float x, float y, float t);
float audio (float x);

void main() {
	vec4 vEyeCoordsPos;
//	vec3 vPos;
	vec4 vVertex;

	// Calculate new vertex position
	vVertex = gl_Vertex;
	vFuncParams.x = (vVertex.x * vScale.x) + vPosition.x;
	vFuncParams.y = (vVertex.y * vScale.y) + vPosition.y;
	vFuncParams.z = FunctionZ (vFuncParams.x, vFuncParams.y, fTime);
	vVertex.z = (vFuncParams.z - vPosition.z) / vScale.z;

	// There appears to be a bug accessing gl_NormalMatrix from the fragment shader
	// We therefore pass it across using a varying variable
//	vNormalMatrix = gl_NormalMatrix;

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
}

float audio (float x) {
	return texture2D (tAudio, vec2(x, 0)).r;
}

float FunctionZ (float x, float y, float t) {
	float z;

	z = /*REPLACE:function*/0.0f/*END*/;

	return z;
}

