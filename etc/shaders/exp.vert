uniform sampler2D texture;

void main() {
  vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

  pixel.a = 0.8;

  gl_FragColor = pixel;
}
