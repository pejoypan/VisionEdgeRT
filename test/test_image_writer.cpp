#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "image_writer.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char **argv) {
    fs::path test_root = fs::temp_directory_path() / "image_writer_test";
    fs::create_directory(test_root);
    zmq::context_t context(0);
    vert::ImageWriter writer(&context, 3, test_root.string());

#ifdef VERT_ENABLE_TEST
    writer.test();
    writer.test1();
    writer.test2();
    writer.test3();
    writer.test4();
    writer.test5();
#endif

    cout << "Test Finish" << endl;
    return 0;
}

