// Playground to start playing with shaders.

// All available uniforms.
uniform vec2 resolution;
uniform sampler2D texture;
uniform float time_ms;
uniform float time_s;

#define PI 3.14159265359

float plot(vec2 position, float pct) {
  return smoothstep(pct - 0.02, pct + 0.02, position.y) -
         smoothstep(pct, pct + 0.02, position.y);
}

// Returns 1.0 for points that belong to the square. Can later be
// multiplied by a color to get the actual square. Width is expressed
// on the x-scale.
float MakeSquare(in vec2 pos, in float x, in float y, in float w) {
  float value = 1.0;

  // Add this to all shaders.
  float ratio = resolution.x / resolution.y;

  float vw = w;
  float hw = w / ratio;

  value *= step(x, pos.x);
  value *= step(y, pos.y);
  value *= 1.0 - step(x + hw, pos.x);
  value *= 1.0 - step(y + vw, pos.y);

  return value;
}

void main() {
  // Position of the fragment downscaled to a [0.0 ,1.0] range.
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(1.0);

  gl_FragColor = vec4(MakeSquare(pos, 0.4, 0.4, 0.3) * color, 1.0);
}
