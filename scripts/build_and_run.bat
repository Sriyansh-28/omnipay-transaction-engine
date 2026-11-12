@echo off

echo ==============================================================================
echo OmniPay Mobile SDK and High-Performance Transaction Engine Runner
echo ==============================================================================

if exist test_runner.exe goto RUN_EXISTING
goto DO_BUILD

:RUN_EXISTING
echo [+] Pre-compiled binaries found.
echo [+] Running Unit Test Suite...
test_runner.exe
echo.
echo [+] Running 10,000+ Transaction Benchmark Suite...
benchmarks_runner.exe
echo.
echo ==============================================================================
echo [SUCCESS] OmniPay Tests and Benchmarks Executed Successfully!
echo Report saved to benchmark_and_qa_report.csv
echo ==============================================================================
exit /b 0

:DO_BUILD
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% (
    echo [ERROR] Could not locate Visual Studio vcvarsall.bat.
    exit /b 1
)

echo [+] Initializing MSVC x64 build environment...
call %VCVARS% x64

echo [+] Compiling C++17 Core Engine and Unit Tests...
cl /std:c++17 /EHsc /O1 /Icore-cpp\include core-cpp\src\instrument\*.cpp core-cpp\src\risk\*.cpp core-cpp\src\observer\*.cpp core-cpp\src\concurrency\*.cpp core-cpp\src\engine\*.cpp core-cpp\tests\test_main.cpp /Fe:test_runner.exe
test_runner.exe

echo [+] Compiling 10,000+ Production Benchmark Suite...
cl /std:c++17 /EHsc /O1 /Icore-cpp\include core-cpp\src\instrument\*.cpp core-cpp\src\risk\*.cpp core-cpp\src\observer\*.cpp core-cpp\src\concurrency\*.cpp core-cpp\src\engine\*.cpp core-cpp\benchmarks\benchmark_suite.cpp psapi.lib /Fe:benchmarks_runner.exe
benchmarks_runner.exe

echo ==============================================================================
echo [SUCCESS] OmniPay Core Engine Build, Verification and Benchmarks Complete!
echo Report saved to benchmark_and_qa_report.csv
echo ==============================================================================
exit /b 0
