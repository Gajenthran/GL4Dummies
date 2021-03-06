#version 330
uniform int basses;  // basses
uniform int time;    // temps
uniform int wave;    // mode "wave" pour "représentation graphique"
uniform float line;  // nombre de lignes pour la construction du cercle
in  vec2 vsoTexCoord;
out vec4 fragColor;


void main(void) {
  /* mouvement des pics de la forme selon la musique */
  vec2 pos = vec2(0.5) - vsoTexCoord;
  float radius = length(pos) * 7.0;

  float a = atan(pos.y,pos.x);
  float f = cos(basses);
  float a2 = atan(pos.y,pos.x);
  float f2 = cos(a * line) * 0.01;

  vec3 c = vec3(smoothstep(f, f + 0.000001, radius)) +
          vec3(smoothstep(0.0, f2 + 0.002, radius));
  
  vec4 colorCercle = vec4(1.0 - c, 0.0);
  fragColor += colorCercle;

  if(wave != 0) {
    /* création des ondes qui varient selon la musique */
    const float speed = .000001;
    const float rate = 15.0;
    const float amp = 0.19;

    pos = -1.9 + 3.0 * vsoTexCoord;
    float x = speed * time + rate * pos.x;
    float base = (1.0 + cos(x * 0.5 + time/100)) * (1.0 + sin(x * basses/4 + basses));
    float z = fract(0.05 * x);
    z = max(z, 1.0 - z);
    z = pow(z, 15.0);
    float pulse = exp(-10000.0 * z);
    vec4 colorWave = vec4(1.0, 1.0, 1.0, 1.0) * pow(clamp(1.0-abs(pos.y-(amp * base + pulse - 0.5)), 0.0, 1.0), 12.0);
    fragColor += colorWave;
  } else {
    /* création du cercle (non remplie) dont les extrémités seront ondulées selon la musique */
    pos = vec2(basses / 4.0) - vsoTexCoord * (basses / 2.0);
    float u = sin((atan(pos.y, pos.x) + time * 0.003) * basses / 2.0) * 0.005 * basses;
    float t = 0.01 * abs(sin(time)) * 5.0 / abs(0.5 + u - length(pos));
    fragColor = mix(colorCercle, vec4(vec3(t), 1.0), 0.1);
  }
}