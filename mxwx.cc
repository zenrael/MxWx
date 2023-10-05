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

class Wx {
    // THINGS WOT I NEED TO STORE:
    //      current temp
    //      current wind
    //      current dew point
    //      current WX
    //      current rain?
    //      current pres
    //      current whatever else
    // ALSO BUT NOT IMMEDIATELY IMPORTANT:
    //      pres history, last 24 hours
    // AND ALSO:
    //      forecast rain for next 24 hours <- impossible coz the met
    //      forecast temp for next 24 hours <- also apparently impossible
    private:
        std::string readBuffer;

    public:
        struct observations {
            std::string currentWx = "1";
            std::string screenTemp = "0";
            std::string tempDew = "0";
            std::string windSpeed = "0";
            std::string windGusts = "0";
            std::string hPa = "0";
            std::string screenHumidity = "0";
            std::string windDirection = "N";
            std::string obsTime = "NOW";
        };
        struct forecast {
            std::vector<double> precProb;
            std::string dailyWx = "0";
            std::string nightlyWx = "0";
        };
        observations obs;
        forecast fcst;

        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        int UpdateDaily() {
            Json::Value root;
            Json::Reader reader;
            CURL *curl = curl_easy_init();

            // Clear the buffer
            readBuffer.clear();

            curl_easy_setopt(curl, CURLOPT_URL,"http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/353282?res=daily&key=376ded23-35ee-4e21-90a8-f4b678bf05d9");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);

            bool parseSuccess = reader.parse(readBuffer, root, false);
            if (parseSuccess) {
                //std::cout << root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"][0]["W"].asString() << std::endl;
                this->fcst.dailyWx = root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"][0]["W"].asString();
                //std::cout << this->fcst.dailyWx << std::endl;
                this->fcst.nightlyWx = root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"][1]["W"].asString();
                //std::cout << this->fcst.nightlyWx << std::endl;
            }

            return 0;
        }

        int Update3Hourly() {
            Json::Value root;
            Json::Reader reader;
            CURL *curl = curl_easy_init();

            // Clear the buffer
            readBuffer.clear();

            curl_easy_setopt(curl, CURLOPT_URL, "http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/353282?res=3hourly&key=376ded23-35ee-4e21-90a8-f4b678bf05d9");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);

            bool parseSuccess = reader.parse(readBuffer, root, false);
            if (parseSuccess) {
                // All of the following may be using stuff that was added to C++ pretty recently, and isn't reflected in the RPI GCC yet.
                // There is another way, though...
                //
                // // std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>> t_now = std::chrono::steady_clock::now();
                // // std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>> t_midnight = std::chrono::floor<std::chrono::days>(t_now);
                // // auto minutes = std::chrono::duration_cast<std::chrono::minutes>(t_now - t_midnight);

                Json::Value period = root["SiteRep"]["DV"]["Location"]["Period"];
                for(Json::Value rep : period[0]["Rep"]) {
                    if ((std::stoi(this->obs.obsTime) - std::stoi(rep["$"].asString())) < 180) {
                        this->fcst.precProb.push_back(std::stod(rep["Pp"].asString()));
                    }
                }
                for(Json::Value rep : period[1]["Rep"]) {
                    if (this->fcst.precProb.size() < 16) {
                        this->fcst.precProb.push_back(std::stod(rep["Pp"].asString()));
                    }
                }
                for(Json::Value rep : period[2]["Rep"]) {
                    if (this->fcst.precProb.size() < 16) {
                        this->fcst.precProb.push_back(std::stod(rep["Pp"].asString()));
                    }
                }
                
            }
            return 0;
        }

        // 3hourly forecast http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/353282?res=3hourly&key=376ded23-35ee-4e21-90a8-f4b678bf05d9

        int UpdateObs() {
            Json::Value root;
            Json::Reader reader;
            CURL *curl = curl_easy_init();

            // Clear the buffer (?)
            readBuffer.clear();

            curl_easy_setopt(curl, CURLOPT_URL, "http://datapoint.metoffice.gov.uk/public/data/val/wxobs/all/json/3351?res=hourly&key=376ded23-35ee-4e21-90a8-f4b678bf05d9");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);

            bool parseSuccess = reader.parse(readBuffer, root, false);
            if (parseSuccess) {
                int days = root["SiteRep"]["DV"]["Location"]["Period"].size();
                int obs = root["SiteRep"]["DV"]["Location"]["Period"][days-1]["Rep"].size();
                Json::Value latestObs = root["SiteRep"]["DV"]["Location"]["Period"][days-1]["Rep"][obs-1];
                this->obs.currentWx = latestObs["W"].asString();
                this->obs.screenTemp = latestObs["T"].asString();
                this->obs.screenHumidity = latestObs["H"].asString();
                this->obs.windSpeed = latestObs["S"].asString();
                this->obs.windGusts = latestObs["G"].asString();
                this->obs.windDirection = latestObs["D"].asString();
                this->obs.hPa = latestObs["P"].asString();
                this->obs.tempDew = latestObs["Dp"].asString();
                this->obs.obsTime = latestObs["$"].asString();

                std::cout << this->obs.obsTime << std::endl;
                //std::cout << root["SiteRep"]["DV"]["Location"]["Period"] << std::endl;
            }

            curl_easy_cleanup(curl);
            return 0;

        }
};

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
    // CURL *curl;
    // CURLcode result;
    // std::string readBuffer;

    //curl = curl_easy_init();
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    Magick::InitializeMagick(*argv);
    Magick::Image image;

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

        CopyImageToCanvas(WxIcons[std::stoi(Weather.obs.currentWx)]->image, matrix, 0, 0);
        if((360 < stoi(Weather.obs.obsTime)) && (stoi(Weather.obs.obsTime) < 1080)) {
            CopyImageToCanvas(WxIcons[std::stoi(Weather.fcst.dailyWx)]->image, matrix, 48, 0);
        } else {
            CopyImageToCanvas(WxIcons[std::stoi(Weather.fcst.nightlyWx)]->image, matrix, 48, 0);
        }

        rgb_matrix::Color colour(0,0,255);
        // Size of screenTemp should not exceed 4, but could be 3. 
        rgb_matrix::DrawText(matrix, font, 18, 4 + font.baseline(), colour, NULL, Weather.obs.screenTemp.c_str(), 1);

        int t_period = 0;
        for (double prob : Weather.fcst.precProb) {
            int probPixels = (int)std::round((16.00/100.00)*prob);
            for(int x = 0; x <= 2 ; x++) {
                for(int y = 31; y >= (32-probPixels); y--) {
                    matrix->SetPixel(t_period + x, y, 0, 0, 160);
                }
            }
            for(int y = 31; y >= 30 ; y--) {
                matrix->SetPixel(t_period + 3, y, 128, 128, 0);
            }
            t_period=t_period+4;
        }
        sleep(300);
        Weather.UpdateObs();
        Weather.UpdateDaily();
        Weather.Update3Hourly();
    }

    return 0;

}
