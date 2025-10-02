#version 120

//uniform vec3 vPosition, vScale;
uniform float fTime;
//varying vec4 vDiffuse, vGlobal, vAmbient;
//varying vec3 vDir, vHalfVector;
//varying float fDist;
varying vec3 vFuncParams;
//varying mat3 vNormalMatrix;

///*REPLACE:controlvars*//*END*/

//float DifferentialX (float x, float y, float t);
//float DifferentialY (float x, float y, float t);
vec4 Colour (float x, float y, float z, float t);
//uniform sampler2D tAudio;

//float audio (float x);

void main () {
//	vec3 vHalf;
//	float fNDotL;
//	float fNDotHV;
//	vec4 vColour;
	vec4 vColourRaw;
//	float fAttenuation;
//	vec3 vNormalBidir;
//	vec3 vTangentX;
//	vec3 vTangentY;
//	vec3 vNormal;

//	// Calculate two tangents to the curve at the point where the fragment touches
//	vTangentX.x = 1.0f;
//	vTangentX.y = 0.0f;
//	vTangentX.z = (DifferentialX (vFuncParams.x, vFuncParams.y, fTime) - vPosition.z) / vScale.z;

//	vTangentY.x = 0.0f;
//	vTangentY.y = 1.0f;
//	vTangentY.z = (DifferentialY (vFuncParams.x, vFuncParams.y, fTime) - vPosition.z) / vScale.z;

//	// Calculate the normal from the tangents
//	vNormal = cross (vTangentX, vTangentY);
//	vNormal = normalize (vNormalMatrix * vNormal);

//	if (gl_FrontFacing) {
//		vNormalBidir = vNormal;
//	}
//	else {
//		vNormalBidir = -vNormal;
//	}

	// Calculate the material colour
	vColourRaw = Colour (vFuncParams.x, vFuncParams.y, vFuncParams.z, fTime);
//	vColour = vGlobal * vColourRaw;

	if (vColourRaw.a <= 0.0) {
		discard;
	}

//	// Calculate dot product between normal and light direction
//	fNDotL = max (dot (vNormalBidir, normalize (vDir)), 0.0);

//	if (fNDotL > 0.0) {
//		fAttenuation = 1.0 / (gl_LightSource[0].constantAttenuation + gl_LightSource[0].linearAttenuation * fDist + gl_LightSource[0].quadraticAttenuation * fDist * fDist);
//		vColour += fAttenuation * (vDiffuse * fNDotL + vAmbient) * vColourRaw;

//		vHalf = normalize (vHalfVector);
//		fNDotHV = max (dot (vNormalBidir, vHalf), 0.0);
//		vColour += fAttenuation * gl_FrontMaterial.specular * gl_LightSource[0].specular * pow (fNDotHV, gl_FrontMaterial.shininess);
//	}

//	gl_FragColor = vColour;
//	gl_FragColor.a = vColourRaw.a;

	gl_FragColor = vec4 (1.0, 1.0, 1.0, 1.0);
}

//float audio (float x) {
//	return texture2D (tAudio, vec2(x, 0)).r;
//}

//float DifferentialX (float x, float y, float t) {
//	float z;
//	
//	z = /*REPLACE:diffX*/0.0f/*END*/;
//	
//	return z;
//}

//float DifferentialY (float x, float y, float t) {
//	float z;

//	z = /*REPLACE:diffY*/0.0f/*END*/;
//	
//	return z;
//}

vec4 Colour (float x, float y, float z, float t) {
	vec4 c;

	c.a = /*REPLACE:alpha*/1.0f/*END*/;
	
	return c;
}

