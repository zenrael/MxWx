#include <string>
#include <vector>
 
class Wx {
    private:
        std::string readBuffer;

    public:
        struct report {
            std::string windGust;       // "G"
            std::string humidity;       // "H"
            std::string temp;           // "T"
            std::string vis;            // "V"
            std::string windDir;        // "D"
            std::string windSpeed;      // "S"
            std::string wxType;         // "W"
            std::string repTime;        // "$"
            std::string repDate;
        };
        struct forecastReport : report {
            std::string feelsLike;      // "F"
            std::string uvIndex;        // "U"
            std::string precProb;       // "Pp"
        };
        struct observationReport : report {
            std::string pressure;      // "P"
            std::string presTendency;   // "Pt"
            std::string dewPoint;       // "Dp"
            int obsNumber;          // Useful for indexing in foreach loops
        };
        struct forecast {
            std::vector<forecastReport> reports;
            std::string dailyWx;
            std::string nightlyWx;
            std::string dayMaximum;
            std::string nightMinimum;
        };
        forecast fcst;
        std::vector<observationReport> obReps;
        std::string obsDataDate;

        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

        int UpdateDaily();

        int Update3Hourly();

        int UpdateObs();
};