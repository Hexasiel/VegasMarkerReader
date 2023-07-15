#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <regex>
#include <unordered_map>
#include <filesystem>



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

void GenerateReport(const std::string& filename) {

    std::ifstream input(filename, std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::vector<uint64_t> data = ReadBinaryFile(filename);
    std::vector<Entry> entries = {};

    entries = AnalyzeData(data);
    
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
    std::cout << "Welcome to VegasMarkerReader!\n Press 's' for single file mode \n Press 'm' for mass processing!" << std::endl;
    char c;
    std::string filePath;
    std::vector<uint64_t> data;
    std::vector<Entry> entries;

    do {
        std::cin >> c;
    } while (c != 's' && c != 'm');

    switch (c)
    {
    case 's': 

        std::cout << "Enter the file path: ";
        std::cin >> filePath;

        data = ReadBinaryFile(filePath);
        entries = AnalyzeData(data);

        // Print the analyzed entries
        std::cout << "Analyzed entries:" << std::endl;
        for (const auto& entry : entries) {
            std::cout << "Name: " << entry.name << ", TimeInSeconds: " << entry.timeInMinutes << std::endl;
        }
        break;
    case 'm': break;
        std::cout << "Place the File you want to read the marker data from in this directory and Press any key to continue!" << std::endl;
        std::cin.get();
        ProcessFilesWithExtension(".veg");
        break;
    default: std::cout << "Something went wrong..." << std::endl;
        break;
    }
    
    std::cout << "Generation Done! Press any key to exit!" << std::endl;
    std::cin.get();
    
    return 0;
}
