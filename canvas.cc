#include "wx.h"
#include "./matrix/include/led-matrix.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>

#include <Magick++.h>
#include <magick/image.h>

#include <cmath>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using Magick::Image;

void ImageToCanvas(const Magick::Image &image, Canvas *canvas, int x_pos = 0, int y_pos = 0) {
    const int offset_x = x_pos, offset_y = y_pos;
    // Copy all pixels to the canvas.
    for (size_t y = 0; y < image.rows(); ++y) {
        for (size_t x = 0; x < image.columns(); ++x) {
            const Magick::Color &c = image.pixelColor(x, y);
                canvas->SetPixel(x + offset_x, y + offset_y,
                (char)((float)( ScaleQuantumToChar(c.redQuantum()) * (1 - ((float)ScaleQuantumToChar(c.alphaQuantum())/255)))),
                (char)((float)( ScaleQuantumToChar(c.greenQuantum()) * (1 - ((float)ScaleQuantumToChar(c.alphaQuantum())/255)))),
                (char)((float)( ScaleQuantumToChar(c.blueQuantum()) * (1 - ((float)ScaleQuantumToChar(c.alphaQuantum())/255)))));
        }
    }
}

void RainChartToCanvas(Wx& Weather, Canvas *matrix) {
    int startPeriod = 0;
    for (int r = 0; r < Weather.fcst.reports.size(); r++) {
        // Remembering that observations by the met are in order of most recent last
        if(stoi(Weather.obReps.back().repTime) - stoi(Weather.fcst.reports[r].repTime) < 180) {
            startPeriod = r;
            break;
        }
    }
    int t_period = 0;
    for (int r = startPeriod; r < (16 + startPeriod); r++) {
        if(r <= Weather.fcst.reports.size()) {
            int probPixels = (int)std::round((16.00/100.00)*stoi(Weather.fcst.reports[r].precProb));
            for(int x = 0; x <= 2 ; x++) {
                for(int y = 31; y >= (32-probPixels); y--) {
                    matrix->SetPixel(t_period + x, y, 100-(3*y), 160-(3*y), 210-(3*y));
                }
            }
            if(stoi(Weather.fcst.reports[r].repTime) == 1260) {
                for(int y = 31; y >= 26; y--) {
                    matrix->SetPixel(t_period + 3, y, 96,96,96);
                }
            } else if (stoi(Weather.fcst.reports[r].repTime) == 540) {
                for(int y = 31; y >= 28; y--) {
                    matrix->SetPixel(t_period + 3, y, 48, 48, 0);
                }
            } else {
                for(int y = 31; y >= 30 ; y--) {
                    matrix->SetPixel(t_period + 3, y, 48, 48, 0);
                }
            }
            t_period=t_period+4;
        }
    }
}

void PressureChartToCanvas(Wx& Weather, Canvas *matrix) {
    // Get min and max pressure over obs range
    int minPres = stoi(Weather.obReps.front().pressure);
    int maxPres = stoi(Weather.obReps.front().pressure);
    for(Wx::observationReport rep : Weather.obReps) {
        if(stoi(rep.pressure) < minPres) {
            minPres = stoi(rep.pressure);
        } else if(stoi(rep.pressure) > maxPres) {
            maxPres = stoi(rep.pressure);
        }
    }
    if(maxPres == minPres)
        maxPres = minPres + 50;
    for(Wx::observationReport rep : Weather.obReps) {
        int x_pos = 20;
        int y_pos = 20;
        // Limits for pressure are about 925 and 1050
        // There'll need to be some sanity checks here increase its out of bounds
        //std::cout << rep.obsNumber << std::endl;
        //std::cout << (int)std::round((16.00/(maxPres-minPres))*(stoi(rep.pressure)-minPres)) << std::endl;
        if(maxPres - minPres < 10)
            matrix->SetPixel(x_pos + rep.obsNumber, y_pos - (int)std::round(stoi(rep.pressure) - minPres), 64, 0, 0);
        else
            matrix->SetPixel(x_pos + rep.obsNumber, y_pos - (int)std::round((10.00/(maxPres-minPres))*(stoi(rep.pressure)-minPres)), 64, 0, 0);
    }
}