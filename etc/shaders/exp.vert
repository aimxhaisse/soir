// Playground to start playing with shaders.

// All available uniforms.
uniform vec2 resolution;
uniform sampler2D texture;
uniform float time_ms;
uniform float time_s;

float plot(vec2 position, float pct) {
  return smoothstep(pct - 0.02, pct, position.y) -
         smoothstep(pct, pct + 0.02, position.y);
}

void main() {
  // Position of the fragment downscaled to a [0.0 ,1.0] range.
  vec2 position = gl_FragCoord.xy / resolution;

  // Whatever we want to draw.
  float y = 1.0 - cos(position.x * 2.0);

  float pct = plot(position, y);
  vec3 color = (1.0 - pct) * vec3(y) + pct * vec3(0.0, 1.0, 0.0);
  gl_FragColor = vec4(color, 1.0);
}
