cmake_minimum_required(VERSION 3.14)
project(OmniPayCore VERSION 2.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX- /O2 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -pthread)
endif()

# Core static library
set(CORE_SOURCES
    src/instrument/IPaymentInstrument.cpp
    src/instrument/CreditCardPayment.cpp
    src/instrument/DebitCardPayment.cpp
    src/instrument/NFCTokenPayment.cpp
    src/instrument/QRWalletPayment.cpp
    src/instrument/PaymentInstrumentFactory.cpp
    src/risk/VelocityCheckStrategy.cpp
    src/risk/GeoAnomalyStrategy.cpp
    src/risk/CompositeRiskStrategy.cpp
    src/observer/AuditLoggingObserver.cpp
    src/observer/LatencyTelemetryObserver.cpp
    src/observer/AlertDispatchObserver.cpp
    src/concurrency/TransactionProcessorPool.cpp
    src/engine/TransactionEngine.cpp
)

add_library(omnipay_core STATIC ${CORE_SOURCES})
target_include_directories(omnipay_core PUBLIC include)

# Unit test binary
add_executable(omnipay_tests tests/test_main.cpp)
target_link_libraries(omnipay_tests PRIVATE omnipay_core)

# Benchmark binary
add_executable(omnipay_benchmarks benchmarks/benchmark_suite.cpp)
target_link_libraries(omnipay_benchmarks PRIVATE omnipay_core)

if(WIN32)
    target_link_libraries(omnipay_benchmarks PRIVATE psapi)
endif()
