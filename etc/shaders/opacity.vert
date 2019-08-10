uniform float opacity;

void main() {
  vec4 color = texture2D(texture, gl_TexCoord[0].xy);
  gl_FragColor = vec4(color.rgb, opacity * color.a);
}
