# Consumer-side smoke test for a release package: configures against an
# installed tree via find_package(), compiles against the single-entry
# header, links, and runs. Used by .github/workflows/release.yml; can
# also be run by hand:
#
#   cmake -S .github/package-test -B build-pkgtest -DCMAKE_PREFIX_PATH=<install-prefix>
#   cmake --build build-pkgtest --config Release
#   ctest --test-dir build-pkgtest -C Release
cmake_minimum_required(VERSION 3.20)
project(aspose_pdf_foss_package_test LANGUAGES CXX)

find_package(aspose_pdf_foss 26.11 CONFIG REQUIRED)

add_executable(package_test main.cpp)
target_link_libraries(package_test PRIVATE aspose_pdf_foss::aspose_pdf_foss)

enable_testing()
add_test(NAME package_test COMMAND package_test)
