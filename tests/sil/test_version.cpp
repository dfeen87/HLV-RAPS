#include "raps/core/raps_core_types.hpp"
#include <fstream>
#include <string>
#include <cassert>
#include <iostream>

void test_version_consistency() {
    assert(std::string(RAPSVersion::STRING) == "3.3.0");
    assert(RAPSVersion::MAJOR == 3);
    assert(RAPSVersion::MINOR == 3);
    assert(RAPSVersion::PATCH == 0);

    // The executable is typically run from build_sil/, so we might need to go up to find VERSION
    // We can try multiple paths
    std::ifstream version_file("VERSION"); // if run from repo root
    if (!version_file.is_open()) {
        version_file.open("../../VERSION"); // if run from build_sil/
    }
    if (!version_file.is_open()) {
        version_file.open("../VERSION"); // just in case
    }

    std::string version_str;
    if (version_file.is_open()) {
        std::getline(version_file, version_str);
        // Trim any trailing newline or carriage return
        while (!version_str.empty() && (version_str.back() == '\n' || version_str.back() == '\r')) {
            version_str.pop_back();
        }
        assert(version_str == "3.3.0");
        version_file.close();
    } else {
        std::cerr << "Could not open VERSION file." << std::endl;
        assert(false);
    }
}

int main() {
    test_version_consistency();
    std::cout << "Version tests passed." << std::endl;
    return 0;
}
