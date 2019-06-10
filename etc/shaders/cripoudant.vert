// Cripoudant.

float plot(vec2 pos, float pct) {
  return step(pct, pos.y + 0.02) - step(pct, pos.y - 0.04);
}

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(1.0);

  // Cripoudant.
  float cripoudant = 0.5;

  for (int i = 6; i < 10; ++i) {
    cripoudant += sin(pos.x * float(i) * 4.0) / 10.0 * sin(time_s);
  }

  float pct = plot(pos, cripoudant);
  color = vec3(1.0) * pct;
  gl_FragColor = vec4(color, 1.0);
}
