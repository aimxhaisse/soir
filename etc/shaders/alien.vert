// Inspired from the movie Alien.

#define REVOLUTION_SEC 10.0
#define SQUARE_NB 5

float MakeCenteredSquare(in vec2 pos, in float size) {
  float val = 1.0;
  float ratio = resolution.x / resolution.y;

  vec2 center;
  center.x = 0.5 - size / 2.0;
  center.y = (0.5 - (size / 2.0) * ratio);

  val *= MakeSquare(pos, center.x, center.y, size);

  return val;
}

float MakeAlienSquare(in vec2 pos, in float phase, in float border) {
  float progression =
      0.01 + mod(phase + mod(time_s, REVOLUTION_SEC) / REVOLUTION_SEC, 1.0);
  float size = progression * (1.0 + border);
  float thickness = progression * border;
  return MakeCenteredSquare(pos, size + thickness) -
         MakeCenteredSquare(pos, size);
}

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(0.0, 1.0, .0);
  float phase = 0.0;
  float pct = 0.0;

  for (int i; i < SQUARE_NB; i++) {
    phase = float(i) / float(SQUARE_NB);
    pct += MakeAlienSquare(pos, phase, 0.05);
  }

  gl_FragColor = vec4(pct * color, 1.0);
}
