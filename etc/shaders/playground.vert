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

// Returns 1.0 for points that belong to the rectangle. Can later be
// multiplied by a color to get the actual rectangle.
float MakeRectangle(in vec2 pos, in float x, in float y, in float w,
                    in float h) {
  float value = 1.0;

  value *= step(x, pos.x);
  value *= step(y, pos.y);
  value *= 1.0 - step(x + w, pos.x);
  value *= 1.0 - step(y + h, pos.y);

  return value;
}

// Returns 1.0 for points that belong to the square. Can later be
// multiplied by a color to get the actual square. Width is expressed
// on the x-scale.
float MakeSquare(in vec2 pos, in float x, in float y, in float w) {
  float ratio = resolution.x / resolution.y;
  float h = w * ratio;

  return MakeRectangle(pos, x, y, w, h);
}

void main() {
  // Position of the fragment downscaled to a [0.0 ,1.0] range.
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(1.0);

  float pct =
      MakeSquare(pos, 0.4, 0.4, 0.3) + MakeRectangle(pos, 0.0, 0.0, 0.1, 0.2);

  gl_FragColor = vec4(pct * color, 1.0);
}
