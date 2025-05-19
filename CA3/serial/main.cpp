#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <math.h>

using namespace std;

void readWavFile(const std::string& inputFile, std::vector<float>& data, SF_INFO& fileInfo) {
    auto start = chrono::high_resolution_clock::now();
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        std::cerr << "Error opening input file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        std::cerr << "Error reading frames from file." << std::endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time read from wav " << time.count() << endl;
    std::cout << "Successfully read " << numFrames << " frames from " << inputFile << std::endl;
}

void writeWavFile(const std::string& outputFile, const std::vector<float>& data,  SF_INFO& fileInfo) {
    sf_count_t originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), originalFrames);
    if (numFrames != originalFrames) {
        std::cerr << "Error writing frames to file." << std::endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    std::cout << "Successfully wrote " << numFrames << " frames to " << outputFile << std::endl;
}


// vector<float> band_pass_filter(vector<float> data , float deltaF) {
//     auto start = chrono::high_resolution_clock::now();
//     vector <float> result_data;
//     for (int i = 0; i < data.size(); i++)
//     {
//         float temp = 0;
//         float pow = data[i] * data[i];
//         temp = pow / (pow + (deltaF*deltaF));
//         result_data.push_back(temp);
//         // cout << temp << endl;
//     }
//     auto end = chrono::high_resolution_clock::now();
//     chrono::duration<double>time = end - start;
//     cout << "time band_pass filter " << time.count() << endl;
//     return result_data;
// }


vector<float> band_pass_filter(vector<float> data , float fdown , float fup) {
    float delta = fup - fdown;
    vector <float> result_data;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < data.size(); i++)
    {
        if ((data[i] > fup) || data[i] < fdown) {
            result_data.push_back(0.0);
        }
        else {
            float temp = 0;
            float pow = data[i] * data[i];
            temp = pow / (pow + (delta*delta));
            result_data.push_back(temp);
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time band_pass filter -----------------> " << time.count() << endl;
    return result_data;
}

vector<float> notch_filter(vector<float> data , float f0) {
    vector <float> result_data;
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < data.size(); i++)
    {
        float temp = 0;
        float div = data[i] / f0;
        float pow = div * div;
        temp = 1 / (pow + 1);
        result_data.push_back(temp);
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time notch filter -----------------> " << time.count() << endl;
    return result_data;
}

vector<float> fir_filter(vector<float> data) {
    vector<float> h = {0 , 0.1 , 0.2 , 0.3 , 0.4 , 0.5 , 0.6 , 0.7 , 0.8 , 0.9 , 1 , 0.9 , 0.8 , 0.7 , 0.6 , 0.5 , 0.4 , 0.3 , 0.2 , 0.1};
    vector <float> result_data(data.size() , 0.0f);
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < data.size(); i++) {
        for (int k = 0; k < h.size(); k++) {
            if(i >= k) {
                result_data[i] += data[i - k] * h[k];
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time FIR filter ------------------> " << time.count() << endl;
    return result_data;
}

vector<float> iir_filter(vector<float> data) {
    vector<float> b = {0 , 0.1 , 0.2 , 0.3 , 0.4 , 0.5 , 0.6 , 0.7 , 0.8 , 0.9 , 1 , 0.9 , 0.8 , 0.7 , 0.6 , 0.5 , 0.4 , 0.3 , 0.2 , 0.1};
    vector<float> a = {0 , 0.001 , 0.002 , 0.003 , 0.004 , 0.005 , 0.006 , 0.007 , 0.008 , 0.009 , 0.01};
    vector <float> result_data(data.size() , 0.0);

    auto start = chrono::high_resolution_clock::now();
    for (int n = 0; n < data.size(); n++) {
        for (int i = 0; i < b.size(); i++) {
            if(n >= i) {
                result_data[n] += data[n - i] * b[i];
            }
        }
        for (int j = 1; j < a.size(); j++) {
            if (n >= j) {
                result_data[n] -= result_data[n - j] * a[j];
            }
            
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time IIR filter ---------------------> " << time.count() << endl;
    return result_data;
}

int main() {
    auto start = chrono::high_resolution_clock::now();

    // set kardan esm haie file ha
    std::string inputFile = "input.wav";
    std::string outputFile = "output.wav";
    std::string outputFile_band = "output_band.wav";
    std::string outputFile_notch = "output_notch.wav";
    std::string outputFile_FIR = "output_FIR.wav";
    std::string outputFile_IRR = "output_IRR.wav";


    SF_INFO fileInfo;
    std::vector<float> audioData;
    std::vector<float> audioData_band;
    std::vector<float> audioData_notch;
    std::vector<float> audioData_FIR;
    std::vector<float> audioData_IRR;

    std::memset(&fileInfo, 0, sizeof(fileInfo));


    readWavFile(inputFile, audioData, fileInfo);

    // bad az khandan az file hala baraye har filter file info ra copy miknim
    SF_INFO fileInfo_band = fileInfo;
    SF_INFO fileInfo_notch = fileInfo;
    SF_INFO fileInfo_FIR = fileInfo;
    SF_INFO fileInfo_IRR = fileInfo;

    // dar inja filter ha ra seda mizanim
    audioData_band = band_pass_filter(audioData , 0.001 , 0.9);
    audioData_notch = notch_filter(audioData , 0.0005);
    audioData_FIR = fir_filter(audioData);
    audioData_IRR = iir_filter(audioData);

    // neveshtan khoroji dar file nahaii
    writeWavFile(outputFile, audioData, fileInfo);
    writeWavFile(outputFile_band, audioData_band, fileInfo_band);
    writeWavFile(outputFile_notch, audioData_notch, fileInfo_notch);
    writeWavFile(outputFile_FIR, audioData_FIR, fileInfo_FIR);
    writeWavFile(outputFile_IRR, audioData_IRR, fileInfo_IRR);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "ALL PROGRESS " << time.count() << endl;
    return 0;
}
