#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform sampler2D ebump;
uniform int bumpMap;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;
 
out vec2 vsoTexCoord;

void main(void) {
  vsoTexCoord = vec2(vsiTexCoord.x, 1.0 - vsiTexCoord.y);
  vec3 bpos = bumpMap != 0 ? vsiPosition + 0.04 * texture(ebump, vsoTexCoord).r * vsiNormal : vsiPosition;
  vec4 mp = modelViewMatrix * vec4(bpos, 1.0);
  gl_Position = projectionMatrix * mp;
}