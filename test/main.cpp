// Copyright (c) 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#include <iostream>
#include <signal.h>

#ifdef ENABLE_OUTPUT
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#endif

#include "CameraBuffer.h"

#ifdef DEBUG
#include <chrono>

using namespace std::chrono;
#endif

using namespace std;

using namespace camera;

bool running = true;

#ifdef ENABLE_OUTPUT
class StreamStore : public cv::VideoWriter {
public:
    StreamStore(const std::string &stream_path)
      : fileStream_(stream_path), windowName_("PoseTracking") {
        if(fileStream_.empty()) {
            live_show_ = true;
            cv::namedWindow(windowName_, /* flag - AUTO_SIZE */ 1);
        }
        else
           live_show_ = false;
    }
    ~StreamStore() { if(isOpened()) release(); }

    bool writeOutput(cv::Mat &frame) {
        if(isStorageEnabled()) {
            if(!isOpened()) {
                open(fileStream_, fourcc_, (double)fps_, frame.size());
                cout << "opening output file " << fileStream_ << endl;
            }
            if(isOpened()) {
                write(frame);
            }
        }
        else {
            cv::imshow(windowName_, frame);
        }
        return true;
    }

    void configure(int fps, int fourcc) { fps_ = fps; fourcc_ = fourcc; };

    bool isStorageEnabled() const { return !live_show_;}

private:
    std::string fileStream_;
    bool live_show_;
    int fourcc_;
    int fps_;
    std::string windowName_;
};
#endif

#define DEFAULT_OUTPUT "output.avi"

#ifdef __linux__
void linux_signal_handler(int signal) {
    switch(signal) {
    case SIGINT:
    case SIGKILL:
        running = false;
        break;
    default:
        exit(0);
    }
}
#endif

int main(int argc, char* argv[]) {
#ifdef __linux__
    signal(SIGINT, linux_signal_handler);
    signal(SIGKILL, linux_signal_handler);
#endif

    string outputFile = DEFAULT_OUTPUT;
    string memType;
    key_t shmKey = 0;

    CameraBuffer::InterfaceType type = CameraBuffer::SHMEM_POSIX;

    int i = 0;
    char cmd = '\0';
    while(i < argc) {
#ifdef DEBUG
        cout << "processing input " << argv[i] << endl;
#endif
        string arg = argv[i];
        if(cmd == 'k') { cmd = 0; shmKey = atoi(arg.c_str()); }
        if(cmd == 't') { cmd = 0; memType = arg; }
        if(cmd == 'o') { cmd = 0; outputFile = arg; }
        if(arg == "-k") cmd = 'k';
        if(arg == "-t") cmd = 't';
        if(arg == "-o") cmd = 'o';
        i++;
    }

    if (shmKey <= 0) {
        cout << "Invalid key value passed" << shmKey << endl;
        return -1;
    }

    if (memType == "posix")
        type = CameraBuffer::SHMEM_POSIX;
    else if (memType == "systemv")
        type = CameraBuffer::SHMEM_SYSTEMV;
    else {
        cout << "Invalid memtype given " << memType << endl;
        return -1;
    }

#ifdef ENABLE_OUTPUT
    StreamStore output(outputFile);
    output.configure(30 /* fps */, output.fourcc('A', 'V', 'C', '1'));
#endif

    CameraBuffer* camera_buffer = nullptr;

    camera_buffer = new CameraBuffer(type);
    if (camera_buffer == nullptr) {
        cout << "Failed to create camera buffer instance. Type: " << type << endl;
        return -1;
    }

    if (!camera_buffer->Open(shmKey)) {
        cout << "Failed to open shared memory for the given key " << shmKey << endl;
        return -1;
    }

    do {
        uint8_t* pBuf = nullptr;
        int len = 0;

#ifdef DEBUG
        auto start = high_resolution_clock::now();
#endif
        if (!camera_buffer->ReadData(&pBuf, &len)) {
            cout << "Error while reading data" << endl;
        }
#ifdef DEBUG
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "Time taken for ReadData() is " << duration.count() << " ms" << endl;
#endif

        cout << "Read data size " << len << endl;
        if (len <= 0) {
            cout << "buffer is not obtained" << endl;
            continue;
        }
#ifdef ENABLE_OUTPUT
        cv::Mat frame(1, len, CV_8UC1, pBuf);
        frame = imdecode(frame, cv::IMREAD_COLOR);
        output.writeOutput(frame);
#endif
    }while (running);

    delete camera_buffer;

    return 0;
}
