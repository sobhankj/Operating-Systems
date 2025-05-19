#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <math.h>
#include <pthread.h>
#include <thread>
using namespace std;

// struct ThreadData {
//     int start;
//     int end;
//     vector<float>* x;
//     float delta;
//     vector<float>* res;
//     float* time;
// };

struct ThreadData {
    int start;
    int end;
    vector<float>* x;
    float fup;
    float fdown;
    vector<float>* res;
    float* time;
    int num;
};

struct ThreadData_notch {
    int start;
    int end;
    vector<float>* x;
    float f0;
    vector<float>* res;
    float* time;
    int num;
};

struct ThreadData_FIR {
    int start;
    int end;
    vector<float>* x;
    vector<float>* res;
    float* time;
    int num;
};

struct ThreadData_IRR {
    int start;
    int end;
    vector<float>* x;
    vector<float>* res;
    float* time;
    int num;
};

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

// void* band_pass_filter(void* arg) {
//     ThreadData* data = static_cast<ThreadData*>(arg);
//     auto start = chrono::high_resolution_clock::now();
//     for (int i = data->start; i < data->end; i++)
//     {
//         float temp = 0;
//         float pow = (*data->x)[i] * (*data->x)[i];

//         temp = pow / (pow + (data->delta * data->delta));
//         (*data->res)[i] =static_cast<float>(temp);
//     }

//     auto end = chrono::high_resolution_clock::now();
//     chrono::duration<double>time = end - start;
//     cout << "time band_pass filter " << time.count() << endl;
//     *data->time += time.count();
//     pthread_exit(nullptr);
// }
void* band_pass_filter(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    float delta = data->fup - data->fdown;
    auto start = chrono::high_resolution_clock::now();
    for (int i = data->start; i < data->end; i++)
    {
        if (((*data->x)[i] > data->fup) || ((*data->x)[i] < data->fdown))
        {
            (*data->res)[i] = static_cast<float>(0.0);
        }
        else {
            float temp = 0;
            float pow = (*data->x)[i] * (*data->x)[i];
            temp = pow / (pow + (delta * delta));
            (*data->res)[i] = static_cast<float>(temp);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time band_pass filter " << data->num << " : " << time.count() << endl;
    *data->time += time.count();
    pthread_exit(nullptr);
}

void* notch_filter(void* arg) {
    ThreadData_notch* data = static_cast<ThreadData_notch*>(arg);
    auto start = chrono::high_resolution_clock::now();
    for (int i = data->start; i < data->end; i++)
    {
        float temp = 0;
        float div = (*data->x)[i] / data->f0;
        float pow = div * div;
        temp = 1 / (pow + 1);
        (*data->res)[i] = static_cast<float>(temp);
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time notch filter " << data->num << " : " << time.count() << endl;
    *data->time += time.count();
    pthread_exit(nullptr);
}

void* fir_filter(void* arg) {
    ThreadData_FIR* data = static_cast<ThreadData_FIR*>(arg);
    vector<float> h = {0 , 0.1 , 0.2 , 0.3 , 0.4 , 0.5 , 0.6 , 0.7 , 0.8 , 0.9 , 1 , 0.9 , 0.8 , 0.7 , 0.6 , 0.5 , 0.4 , 0.3 , 0.2 , 0.1};
    auto start = chrono::high_resolution_clock::now();
    for (int i = data->start; i < data->end; i++) {
        for (int k = 0; k < h.size(); k++) {
            if(i >= k) {
                (*data->res)[i] += (*data->x)[i-k] * h[k];
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time FIR filter " << data->num << " : " << time.count() << endl;
    *data->time += time.count();
    pthread_exit(nullptr);
}

void* irr_filter(void* arg) {
    ThreadData_IRR* data = static_cast<ThreadData_IRR*>(arg);
    vector<float> b = {0 , 0.1 , 0.2 , 0.3 , 0.4 , 0.5 , 0.6 , 0.7 , 0.8 , 0.9 , 1 , 0.9 , 0.8 , 0.7 , 0.6 , 0.5 , 0.4 , 0.3 , 0.2 , 0.1};
    vector<float> a = {0 , 0.001 , 0.002 , 0.003 , 0.004 , 0.005 , 0.006 , 0.007 , 0.008 , 0.009 , 0.01};

    auto start = chrono::high_resolution_clock::now();
    for (int n = data->start; n < data->end; n++) {
        for (int i = 0; i < b.size(); i++) {
            if(n >= i) {
                (*data->res)[n] += (*data->x)[n - i] * b[i];
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time IRR filter " << data->num << " : " << time.count() << endl;
    *data->time += time.count();
    pthread_exit(nullptr);
}

void irr_done(vector<float>& data , float& time_irr) {
    auto start = chrono::high_resolution_clock::now();
    vector<float> a = {0 , 0.001 , 0.002 , 0.003 , 0.004 , 0.005 , 0.006 , 0.007 , 0.008 , 0.009 , 0.01};
    for (int n = 0; n < data.size(); n++)
    {
        for (int j = 0; j < a.size(); j++)
        {
            if (n >= j) {
                data[n] -= data[n - j] * a[j];
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double>time = end - start;
    cout << "time IRR done filter " << time.count() << endl;
    time_irr += time.count();
}

int main() {
    auto start = chrono::high_resolution_clock::now();
    std::string inputFile = "input.wav";
    std::string outputFile = "output.wav";
    std::string outputFile_band = "output_band2.wav";
    std::string outputFile_notch = "output_notch2.wav";
    std::string outputFile_FIR = "output_FIR2.wav";
    std::string outputFile_IRR = "output_IRR2.wav";


    SF_INFO fileInfo;
    // SF_INFO fileInfo_band;
    std::vector<float> audioData;
    std::vector<float> audioData_band;
    std::vector<float> audioData_notch;
    std::vector<float> audioData_FIR;
    std::vector<float> audioData_IRR;

    std::memset(&fileInfo, 0, sizeof(fileInfo));
    // std::memset(&fileInfo_band, 0, sizeof(fileInfo_band));


    readWavFile(inputFile, audioData, fileInfo);
    // readWavFile(inputFile, audioData_band, fileInfo_band);

    SF_INFO fileInfo_band = fileInfo;
    SF_INFO fileInfo_notch = fileInfo;
    SF_INFO fileInfo_FIR = fileInfo;
    SF_INFO fileInfo_IRR = fileInfo;

    audioData_band = audioData;
    int NUM_THREAD = 4;
    pthread_t thread_band[NUM_THREAD];
    ThreadData threadData_band[NUM_THREAD];
    float time_band = 0;

    int chunksize_band = audioData.size() / NUM_THREAD;

    for (int i = 0; i < NUM_THREAD; i++)
    {
        threadData_band[i].start = i * chunksize_band;
        threadData_band[i].end = (i == NUM_THREAD - 1) ? audioData.size() : (i+1) * chunksize_band;
        threadData_band[i].x = &audioData;
        threadData_band[i].fup = 0.9;
        threadData_band[i].fdown = 0.001;
        threadData_band[i].res = &audioData_band;
        threadData_band[i].time = &time_band;
        threadData_band[i].num = i;
        if (pthread_create(&thread_band[i] , nullptr , band_pass_filter , &threadData_band[i]) != 0) {
            cerr << "error" << i << endl;
        }
    }
    
    for (int i = 0; i < NUM_THREAD; i++)
    {
        pthread_join(thread_band[i] , nullptr);
    }
    cout << "TIME BAND ---------------------------->" <<  time_band / NUM_THREAD << endl;


    ///////////////////////////////////////////////////////////////////////////////////////////////////

    audioData_notch = audioData;
    int NUM_THREAD_notch = 4;
    pthread_t thread_notch[NUM_THREAD_notch];
    ThreadData_notch threadData_notch[NUM_THREAD_notch];
    float time_notch = 0;

    int chunksize_notch = audioData.size() / NUM_THREAD_notch;

    for (int i = 0; i < NUM_THREAD_notch; i++)
    {
        threadData_notch[i].start = i * chunksize_notch;
        threadData_notch[i].end = (i == NUM_THREAD_notch - 1) ? audioData.size() : (i+1) * chunksize_notch;
        threadData_notch[i].x = &audioData;
        threadData_notch[i].f0 = 0.0005;
        threadData_notch[i].res = &audioData_notch;
        threadData_notch[i].time = &time_notch;
        threadData_notch[i].num = i;
        if (pthread_create(&thread_notch[i] , nullptr , notch_filter , &threadData_notch[i]) != 0) {
            cerr << "error" << i << endl;
        }
    }
    
    for (int i = 0; i < NUM_THREAD_notch; i++)
    {
        pthread_join(thread_notch[i] , nullptr);
    }
    cout << "TIME NOTCH ------------------------>" <<  time_notch / NUM_THREAD_notch << endl;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    audioData_FIR = audioData;
    int NUM_THREAD_FIR = 4;
    pthread_t thread_FIR[NUM_THREAD_FIR];
    ThreadData_FIR threadData_FIR[NUM_THREAD_FIR];
    float time_FIR = 0;

    int chunksize_FIR = audioData.size() / NUM_THREAD_FIR;

    for (int i = 0; i < NUM_THREAD_FIR; i++)
    {
        threadData_FIR[i].start = i * chunksize_FIR;
        threadData_FIR[i].end = (i == NUM_THREAD_FIR - 1) ? audioData.size() : (i+1) * chunksize_FIR;
        threadData_FIR[i].x = &audioData;
        threadData_FIR[i].res = &audioData_FIR;
        threadData_FIR[i].time = &time_FIR;
        threadData_FIR[i].num = i;
        if (pthread_create(&thread_FIR[i] , nullptr , fir_filter , &threadData_FIR[i]) != 0) {
            cerr << "error" << i << endl;
        }
    }
    
    for (int i = 0; i < NUM_THREAD_FIR; i++)
    {
        pthread_join(thread_FIR[i] , nullptr);
    }
    cout << "TIME FIR TOL ---------------------->" <<  time_FIR / NUM_THREAD_FIR << endl;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    audioData_IRR = audioData;
    int NUM_THREAD_IRR = thread::hardware_concurrency();
    pthread_t thread_IRR[NUM_THREAD_IRR];
    ThreadData_IRR threadData_IRR[NUM_THREAD_IRR];
    float time_IRR = 0;

    int chunksize_IRR = audioData.size() / NUM_THREAD_IRR;

    for (int i = 0; i < NUM_THREAD_IRR; i++)
    {
        threadData_IRR[i].start = i * chunksize_IRR;
        threadData_IRR[i].end = (i == NUM_THREAD_IRR - 1) ? audioData.size() : (i+1) * chunksize_IRR;
        threadData_IRR[i].x = &audioData;
        threadData_IRR[i].res = &audioData_IRR;
        threadData_IRR[i].time = &time_IRR;
        threadData_IRR[i].num = i;
        if (pthread_create(&thread_IRR[i] , nullptr , irr_filter , &threadData_IRR[i]) != 0) {
            cerr << "error" << i << endl;
        }
    }
    
    for (int i = 0; i < NUM_THREAD_IRR; i++)
    {
        pthread_join(thread_IRR[i] , nullptr);
    }

    cout << "TIME IRR Thread --------------------->" <<  time_IRR / NUM_THREAD_IRR << endl;
    time_IRR = time_IRR / NUM_THREAD_IRR;
    irr_done(audioData_IRR , time_IRR);
    cout << "TIME IRR TOLLLLLLL ---------------------->" <<  time_IRR  << endl;


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
