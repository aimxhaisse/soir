// Playground to experiment with shaders.

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(1.0);
  float pct = MakeSquare(pos, 0.4, 0.4, 0.2) +
              MakeRectangle(pos, 0.0, 0.0, 0.1, 0.2 * abs(sin(time_s)));
  gl_FragColor = vec4(pct * color, 1.0);
}
