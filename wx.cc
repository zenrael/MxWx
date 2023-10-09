#include "wx.h"

#include <stdio.h>
#include <unistd.h> 
// for sleep()

#include <iostream>
#include <map>
#include <vector>
#include <string>

#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/value.h>

#include <cmath>
#include <chrono>


size_t Wx::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int Wx::UpdateDaily() {
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
        Json::Value today = root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"];
        this->fcst.dailyWx = root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"][0]["W"].asString();
        this->fcst.nightlyWx = root["SiteRep"]["DV"]["Location"]["Period"][0]["Rep"][1]["W"].asString();
        this->fcst.dayMaximum = today[0]["Dm"].asString();
        this->fcst.nightMinimum = today[1]["Nm"].asString();
    }
    curl_easy_cleanup(curl);
    return 0;
}

int Wx::Update3Hourly() {
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

        for(int i = 0; i < period.size(); i++) {
            for(Json::Value rep : period[i]["Rep"]) {
                forecastReport rpt;
                rpt.windDir = rep["D"].asString();
                rpt.feelsLike = rep["F"].asString();
                rpt.windGust = rep["G"].asString();
                rpt.humidity = rep["H"].asString();
                rpt.precProb = rep["Pp"].asString();
                rpt.windSpeed = rep["S"].asString();
                rpt.temp = rep["T"].asString();
                rpt.vis = rep["V"].asString();
                rpt.wxType = rep["W"].asString();
                rpt.uvIndex = rep["U"].asString();
                rpt.repTime = rep["$"].asString();
                rpt.repDate = period[i]["value"].asString();
                this->fcst.reports.push_back(rpt);
            }
        }
                
    }
    curl_easy_cleanup(curl);
    return 0;
}

int Wx::UpdateObs() {
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
        Json::Value period = root["SiteRep"]["DV"]["Location"]["Period"];
        int repNum = 0;
        this->obsDataDate = root["SiteRep"]["DV"]["dataDate"].asString();
        for(int i = 0; i < period.size(); i++) {
            // The met give observations in order of oldest first
            // So if we're pushing to a vector, element 0 will be the oldest obs
            // not the latest. 
            for(Json::Value rep : period[i]["Rep"]) {
                observationReport obr;
                obr.windDir = rep["D"].asString();
                obr.windGust = rep["G"].asString();
                obr.humidity = rep["H"].asString();
                obr.pressure = rep["P"].asString();
                obr.windSpeed = rep["S"].asString();
                obr.temp = rep["T"].asString();
                obr.vis = rep["V"].asString();
                obr.wxType = rep["W"].asString();
                obr.presTendency = rep["Pt"].asString();
                obr.dewPoint = rep["Dp"].asString();
                obr.repTime = rep["$"].asString();
                obr.repDate = period[i]["value"].asString();
                obr.obsNumber = repNum;
                this->obReps.push_back(obr);
                repNum++;
            }
        }
    }
    curl_easy_cleanup(curl);
    return 0;
}
