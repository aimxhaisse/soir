// Common functions and helpers.
//
// This file is implicitly included in all shaders.

// CONSTANTS.

#define PI 3.14159265359

// SHAPES.

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
