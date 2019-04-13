// Inspired from the movie Alien.

float MakeCenteredSquare(in vec2 pos, in float size) {
  float val = 1.0;
  float ratio = resolution.x / resolution.y;

  vec2 center;
  center.x = 0.5 - size / 2.0;
  center.y = (0.5 - (size / 2.0) * ratio);

  val *= MakeSquare(pos, center.x, center.y, size);

  return val;
}

float MakeAlienSquare(in vec2 pos, in float size) {
  float val = 0.0;

  for (float i = 1.0; i < 5.0; i = i + 1.0) {
    float size = i * 0.1 * mod(time_s / 2.0, 4.0);
    float outer_size = size + 0.01;
    float inner_size = size;
    val += MakeCenteredSquare(pos, outer_size) -
           MakeCenteredSquare(pos, inner_size);
  }

  return val;
}

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(0.0, 1.0, .0);

  float pct = MakeAlienSquare(pos, 0.3);

  gl_FragColor = vec4(pct * color, 1.0);
}
