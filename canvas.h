#include "./matrix/include/led-matrix.h"
#include <Magick++.h>
#include <magick/image.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using Magick::Image;

void ImageToCanvas(const Magick::Image &image, Canvas *canvas, int x_pos, int y_pos);

void RainChartToCanvas(Wx& Weather, Canvas *matrix);

void PressureChartToCanvas(Wx& Weather, Canvas *matrix);