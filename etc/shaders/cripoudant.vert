// Cripoudant.

float plot(vec2 pos, float pct) {
  return smoothstep(pct - 0.01, pct, pos.y) -
         smoothstep(pct, pct + 0.01, pos.y);
}

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(1.0);

  float cripoudant = 0.2 + mod(pos.x * 10.0, 0.5);

  float pct = plot(pos, cripoudant);
  color = vec3(1.0) * pct;
  gl_FragColor = vec4(color, 1.0);
}
