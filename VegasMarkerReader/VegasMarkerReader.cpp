#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <regex>
#include <unordered_map>
#include <filesystem>

int convertStringToInt(const std::string& str, int removeAtEnd) {
    // Find the position of the first comma
    size_t commaPos = str.find(',');

    // If comma is found
    if (commaPos != std::string::npos) {
        // Erase characters from the beginning until the comma
        std::string remainingStr = str.substr(commaPos + 1);

        // Remove the last three characters
        if (remainingStr.length() >= removeAtEnd) {
            remainingStr = remainingStr.substr(0, remainingStr.length() - removeAtEnd);
        }

        // Convert the remaining string to an integer
        try {
            return std::stoi(remainingStr);
        } catch (const std::exception& e) {
            // Handle any potential conversion error
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    // Return a default value if the conversion fails or no comma is found
    return 0;
}


std::vector<uint64_t> ReadBinaryFile(const std::string& filePath) {
    std::ifstream inputFile(filePath, std::ios::binary);

    if (!inputFile.is_open()) {
        std::cout << "Failed to open the file." << std::endl;
        return {};
    }

    // Determine the size of the file
    inputFile.seekg(0, std::ios::end);
    std::streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // Calculate the number of uint64_t values based on the file size
    std::size_t numValues = fileSize / sizeof(uint64_t);

    // Create a vector to store the uint64_t values
    std::vector<uint64_t> data(numValues);

    // Read the binary data into the vector
    inputFile.read(reinterpret_cast<char*>(data.data()), fileSize);

    inputFile.close();

    return data;
}

struct Entry {
    std::string name;
    uint64_t time;
    double timeInMinutes;
};

struct Pause {
    std::string name;
    uint64_t startTime;
    uint64_t endTime;
    double durationInSeconds;
    int sectionNumber;
};

std::list<Pause> GeneratePauses(const std::vector<Entry>& entries) {
    std::list<Pause> pauses;
    std::vector<Entry>::const_iterator startEntry = entries.end();
    int currentSection = 0;
    std::regex sectionMarkerRegex(R"(\d+m)");
    std::regex sectionMarkerRegex2(R"(\d+min)");

    for (const auto& entry : entries) {
        if (entry.name.substr(0, 2) == "TP") {
            if (entry.name[entry.name.length() - 1] == 'A') {
                std::string pauseName = entry.name;
                pauseName.erase(0, 2); // Remove "T P " prefix
                pauseName.erase(pauseName.length() - 1); // Remove " A" suffix

                Pause pause;
                pause.name = pauseName;
                pause.startTime = entry.time;
                pause.sectionNumber = currentSection;
                startEntry = entries.begin() + (&entry - &entries[0]);
            } else if (entry.name[entry.name.length() - 1] == 'E' && startEntry != entries.end()) {
                Pause pause;
                pause.name = startEntry->name;
                pause.startTime = startEntry->time;
                pause.endTime = entry.time;
                pause.durationInSeconds = (double)(pause.endTime - pause.startTime) / (double)10000000;
                pause.sectionNumber = currentSection;
                pauses.emplace_back(pause);
                startEntry = entries.end();
            }
        } else if (std::regex_match(entry.name, sectionMarkerRegex) || std::regex_match(entry.name, sectionMarkerRegex2)) {
            currentSection = std::stoi(entry.name);
            if (startEntry != entries.end()) {
                Pause pause;
                pause.name = startEntry->name;
                pause.startTime = startEntry->time;
                pause.endTime = entry.time;
                pause.durationInSeconds = (double)(pause.endTime - pause.startTime) / (double)10000000;
                pause.sectionNumber = currentSection-1;
                pauses.emplace_back(pause);

                startEntry = entries.end();

                Pause pause2;
                pause2.name = entry.name;
                pause2.startTime =  pause.endTime;
                pause2.sectionNumber = currentSection;
                startEntry = entries.begin() + (&entry - &entries[0]);
            }
        }
    }
    return pauses;
}

std::vector<Entry> AnalyzeData(const std::vector<uint64_t>& data) {
    std::vector<Entry> entries;

    bool patternFound = false;
    size_t dataIndex = 0;

    while (dataIndex < data.size() - 7) {
        if (!patternFound && data[dataIndex] == 1284151712075440726 && data[dataIndex + 1] == 10005747470308525958) {
            patternFound = true;
            dataIndex += 2;
        } else if (patternFound) {
            Entry entry;
            entry.name = "";
            size_t asciiIndex = dataIndex + 5;
            while (true) {
                uint64_t asciiValue = data[asciiIndex];
                if (asciiValue == 1284151712075440726 || asciiValue ==1283404051827091820) {
                    break;
                }
                for(int i = 0; i<8;i++)
                {
                    char c = static_cast<char>((asciiValue>>8*(i)) & 0xFF);
                    if(c != 0)
                    entry.name += c;
                }
                ++asciiIndex;
            }
            entry.time = data[dataIndex + 1];
            double time = (double)entry.time;
            entry.timeInMinutes = time / (double)10000000;
            entries.push_back(entry);
            patternFound = false;
        }

        dataIndex++;
    }

    return entries;
}

std::unordered_map<int, double> CalculateSectionDurations(const std::list<Pause>& pauses) {
    std::unordered_map<int, double> sectionDurations;

    for (const auto& pause : pauses) {
        sectionDurations[pause.sectionNumber] += pause.durationInSeconds;
    }

    return sectionDurations;
}

std::unordered_map<int, int> CalculateSectionPauseCounts(const std::list<Pause>& pauses) {
    std::unordered_map<int, int> pauseCounts;

    for (const auto& pause : pauses) {
        pauseCounts[pause.sectionNumber] += 1;
    }

    return pauseCounts;
}

void GenerateReport(const std::string& filename) {

    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::vector<uint64_t> data = ReadBinaryFile(filename);
    std::vector<Entry> entries = {};
    std::list<Pause> pauses = {};
    std::unordered_map<int, double> sectionDurations = {};
    std::unordered_map<int, int> sectionPausecounts = {};

    entries = AnalyzeData(data);
    pauses = GeneratePauses(entries);
    if(pauses.size() > 0)
    {
        sectionDurations = CalculateSectionDurations(pauses);
        sectionPausecounts = CalculateSectionPauseCounts(pauses);
    }
    
    std::ofstream output(filename + ".txt");
    if (!output.is_open()) {
        std::cerr << "Failed to create output file: " << filename + ".txt" << std::endl;
        return;
    }


    // Write entries
    output << "Entries:" << std::endl;
    for (const auto& entry : entries) {
        output << "Name: " << entry.name << ", Time: " << entry.timeInMinutes << std::endl;
    }

    output << std::endl;

    // Write pauses
    output << "Pauses:" << std::endl;
    for (const auto& pause : pauses) {
        output << "Name: " << pause.name << ", Section: " << pause.sectionNumber + 1<< ", Duration: " << pause.durationInSeconds << " seconds" << std::endl;
    }

    output << std::endl;

    double multiplier = 1;
    std::regex subSectionMarkerRegex(R"(\d+,\d+m)");
    std::regex subSectionMarkerRegex2(R"(\d+,\d+min)");

    for (const auto& entry : entries) {
        if(std::regex_match(entry.name,subSectionMarkerRegex) )
        {
            int i = convertStringToInt(entry.name, 1);
            if(i % 30 == 0) multiplier = 2;
            if(i % 20 == 0) multiplier = 3;
            break;
        }
        else if(std::regex_match(entry.name,subSectionMarkerRegex2))
        {
            int i = convertStringToInt(entry.name, 3);
            if(i % 30 == 0) multiplier = 2;
            if(i % 20 == 0) multiplier = 3;
            break;
        }
    }

    // Write section durations
    output << "Section Pause counts:" << std::endl;
    for (const auto& entry : sectionPausecounts) {
        output << "Section: " << entry.first + 1<< ", Pause Count: " << entry.second << " pauses." << std::endl;
    }

    output << std::endl;
    
    output << "Time Multiplier: " << multiplier << std::endl;

    output << std::endl;
    // Write section durations
    output << "Section Durations:" << std::endl;
    for (const auto& entry : sectionDurations) {
        output << "Section: " << entry.first + 1<< ", Duration: " << entry.second<< ", Multiplied Duration: " << entry.second / multiplier << " seconds" << std::endl;
    }

    
}


void ProcessFilesWithExtension(const std::string& extension) {
    std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator it("."); it != end; ++it) {
        if (it->is_regular_file() && it->path().extension() == extension) {
            std::cout << "Generating Data for :" << it->path().filename().string() << std::endl;
            GenerateReport(it->path().filename().string());
        }
    }
}

int main() {
    std::cout << "Willkommen to VegasMarkerReader! Place the File you want to read the marker data from in this directory and Press any key to continue" << std::endl;
    std::cin.get();
    ProcessFilesWithExtension(".veg");
    std::cout << "Generation Done! Press any key to exit!" << std::endl;
    std::cin.get();
    /* 
    std::string filePath;
    std::cout << "Enter the file path: ";
    std::cin >> filePath;

    std::vector<uint64_t> data = ReadBinaryFile(filePath);
    std::vector<Entry> entries = AnalyzeData(data);
    std::list<Pause> pauses = GeneratePauses(entries);
    std::unordered_map<int, double> sectionDurations = CalculateSectionDurations(pauses);
    
    // Print the analyzed entries
    std::cout << "Analyzed entries:" << std::endl;
    for (const auto& entry : entries) {
        std::cout << "Name: " << entry.name << ", TimeInSeconds: " << entry.timeInMinutes<< std::endl;
    }
    // Print the pauses
    std::cout << "Pauses:" << std::endl;
    for (const auto& pause : pauses) {
        std::cout << "Name: " << pause.name << ", Duration: " << pause.durationInSeconds
                  << ", Section: " << pause.sectionNumber << std::endl;
    }
    // Print section durations
    std::cout << "Section Durations:" << std::endl;
    for (const auto& entry : sectionDurations) {
        std::cout << "Section: " << entry.first << ", Duration: " << entry.second << " seconds" << std::endl;
    }
    std::cin >> filePath;*/
    return 0;
}
