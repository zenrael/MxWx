#include "wx.h"

#include "./matrix/include/led-matrix.h"
// For obvious reasons of matrix magic

#include <signal.h>
// To handle interrupts for infinite while loop (cleanly)
#include <stdio.h>
#include <unistd.h> 
// for sleep()

#include <iostream>
#include <map>
#include <string>

#include <Magick++.h>
#include <magick/image.h>

#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/value.h>

#include <cmath>
#include <chrono>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using Magick::Image;



class Icon {
    public:
        std::string name;
        int met_number;
        std::string filename;
        Magick::Image image;

        Icon()  :   name("name"),   met_number(0),  filename("unknown.png")  {}

        Icon(std::string n, int num, std::string f = "unknown.png") {
            name = n;
            met_number = num;
            filename = f;
            image.read(filename);
        }
};

std::map<int, Icon*> WxIcons;

void LoadIcons() {
    const std::string path = "./16x16/";

    // WxIcons.insert(std::pair<int, Icon*>(23, new Icon("chanceflurries", 23, path + "chanceflurries.png"));
    //
    // This syntax is (i believe) preferred, but the [] operator is much more readable 
    // and we dont really have to worry about keys being overwritten without warning since we set this 
    // up only once. 

    WxIcons[0] = new Icon("Clear Night", 0, path + "nt_clear.png");
    WxIcons[1] = new Icon("Sunny Day", 1, path + "sunny.png");
    WxIcons[2] = new Icon("Partly Cloudy (night)", 2, path + "nt_partlycloudy.png");
    WxIcons[3] = new Icon("Partly Cloudy (day)", 3, path + "partlycloudy.png");
    // 4 is unused.
    WxIcons[5] = new Icon("Mist", 5, path + "hazy.png");
    WxIcons[6] = new Icon("Fog", 6, path + "fog.png");
    WxIcons[7] = new Icon("Cloudy", 7, path + "cloudy.png");
    WxIcons[8] = new Icon("Overcast", 8, path + "cloudy.png");
    WxIcons[9] = new Icon("Light rain shower (night)", 9, path + "nt_chancerain.png");
    WxIcons[10] = new Icon("Light rain shower (day)", 10, path + "chancerain.png");
    WxIcons[11] = new Icon("Drizzle", 11, path + "chancerain.png");
    WxIcons[12] = new Icon("Light rain", 12, path + "chancerain.png");
    WxIcons[13] = new Icon("Heavy rain shower (night)", 13, path + "nt_rain.png");
    WxIcons[14] = new Icon("Heavy rain shower (day)", 14, path + "rain.png");
    WxIcons[15] = new Icon("Heavy rain", 15, path + "rain.png");
    WxIcons[16] = new Icon("Sleet shower (night)", 16, path + "nt_sleet.png");
    WxIcons[17] = new Icon("Sleet shower (day)", 17, path + "sleet.png");
    WxIcons[18] = new Icon("Sleet", 18, path + "sleet.png");
    WxIcons[19] = new Icon("Hail shower (night)", 19, path + "nt_chanceflurries.png");
    WxIcons[20] = new Icon("Hail shower (day)", 20, path + "chanceflurries.png");
    WxIcons[21] = new Icon("Hail", 21, path + "chanceflurries.png");
    WxIcons[22] = new Icon("Light snow shower (night)", 22, path + "nt_chancesnow.png");
    WxIcons[23] = new Icon("Light snow shower (day)", 23, path + "chancesnow.png");
    WxIcons[24] = new Icon("Light snow", 24, path + "chancesnow.png");
    WxIcons[25] = new Icon("Heavy snow shower (night)", 25, path + "nt_snow.png");
    WxIcons[26] = new Icon("Heavy snow shower (day)", 26, path + "snow.png");
    WxIcons[27] = new Icon("Heavy snow", 27, path + "snow.png");
    WxIcons[28] = new Icon("Thunder shower (night)", 28, path + "nt_tstorms.png");
    WxIcons[29] = new Icon("Thunder shower (day)", 29, path + "tstorms.png");
    WxIcons[30] = new Icon("Thunder", 30, path + "chancetstorms.png");
}

void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas, int x_pos = 0, int y_pos = 0) {
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

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    Magick::InitializeMagick(*argv);
    Magick::Image image;

    rgb_matrix::Color fcastDayFontColour(153,76,96);
    rgb_matrix::Color fcastNightFontColour(96,96,96);
    rgb_matrix::Color obsTempFontColour(102,102,0);

    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular";
    defaults.rows = 32;
    defaults.cols = 64;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.show_refresh_rate = true;
    RGBMatrix *matrix = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);
    if (matrix == NULL)
        return 1;

    // Lets have a go at loading some fonts...
    rgb_matrix::Font font;
    if (!font.LoadFont("matrix/fonts/4x6.bdf")) {
        std::cout << "BDF not loading?" << std::endl;
        return 1;
    }

    Wx Weather;
    Weather.UpdateObs();
    Weather.UpdateDaily();
    Weather.Update3Hourly();
    LoadIcons();
    
    while (!interrupt_received) {
        // Remember to clear the canvas 
        matrix->Clear();

        std::cout << Weather.obsDataDate << std::endl;

        CopyImageToCanvas(WxIcons[std::stoi(Weather.obReps.back().wxType)]->image, matrix, 0, 0);
        if((360 < stoi(Weather.obReps.back().repTime)) && (stoi(Weather.obReps.back().repTime) < 1080)) {
            CopyImageToCanvas(WxIcons[std::stoi(Weather.fcst.dailyWx)]->image, matrix, 48, 0);
        } else {
            CopyImageToCanvas(WxIcons[std::stoi(Weather.fcst.nightlyWx)]->image, matrix, 48, 0);
        }

        int tempInt = std::stoi(Weather.obReps.back().temp);
        std::string tempIntStr = std::to_string(tempInt);
        rgb_matrix::DrawText(matrix, font, 17, 0 + font.baseline(), obsTempFontColour, NULL, tempIntStr.c_str(), 1);

        rgb_matrix::DrawText(matrix, font, 39, 0 + font.baseline(), fcastDayFontColour, NULL, Weather.fcst.dayMaximum.c_str(), 1);
        rgb_matrix::DrawText(matrix, font, 39, 8 + font.baseline(), fcastNightFontColour, NULL, Weather.fcst.nightMinimum.c_str(), 1);

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
                        matrix->SetPixel(t_period + x, y, 0, 0, 160);
                    }
                }
                if(stoi(Weather.fcst.reports[r].repTime) == 1260) {
                    for(int y = 31; y >= 26; y--) {
                        matrix->SetPixel(t_period + 3, y, 96,96,96);
                    }
                } else if (stoi(Weather.fcst.reports[r].repTime) == 540) {
                    for(int y = 31; y >= 26; y--) {
                        matrix->SetPixel(t_period + 3, y, 96, 96, 0);
                    }
                } else {
                    for(int y = 31; y >= 30 ; y--) {
                        matrix->SetPixel(t_period + 3, y, 96, 96, 0);
                    }
                }
                t_period=t_period+4;
            }
        }

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

        sleep(300);
        Weather.UpdateObs();
        Weather.UpdateDaily();
        Weather.Update3Hourly();
    }

    return 0;
}



