#define REVOLUTION_SEC 10.0
#define SQUARE_NB 50

float MakeCenteredSquare(in vec2 pos, in float size) {
  float ratio = resolution.x / resolution.y;

  vec2 center;
  center.x = 0.5 - size / 2.0;
  center.y = (0.5 - (size / 2.0) * ratio);

  return MakeSquare(pos, center.x, center.y, size);
}

float MakeAlienSquare(in vec2 pos, in float phase, in float border) {
  float progression =
      mod(phase + mod(time_s, REVOLUTION_SEC) / REVOLUTION_SEC, 1.0);
  float size = progression * (1.0 + border);
  float thickness = progression * border + 0.01;

  return MakeCenteredSquare(pos, size + thickness) -
         MakeCenteredSquare(pos, size);
}

mat2 MakeRotation(in float angle) {
  return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

void main() {
  vec2 pos = gl_FragCoord.xy / resolution;
  vec3 color = vec3(0.0);

  for (int i; i < SQUARE_NB; i++) {
    float phase = 0.0;
    phase = float(i) / float(SQUARE_NB);

    pos -= vec2(0.5);
    pos = pos * MakeRotation(time_s);
    pos += vec2(0.5);

    float pct = MakeAlienSquare(pos, phase, 0.01);
    color += pct * vec3(phase * cos(time_s), phase * sin(time_s),
                        phase * sin(time_s) + 0.5);
  }

  gl_FragColor = vec4(color, 1.0);
}
