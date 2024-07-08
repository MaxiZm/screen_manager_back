cmake_minimum_required(VERSION 3.28)
project(ScreenManager)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find necessary packages
find_package(Boost 1.70 REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

# Add include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SQLite3_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# Add the executable
add_executable(ScreenManager main.cpp
        crow_all.h
        api_handler.h
        api_handler.cpp
        database.cpp)

# Link the libraries
target_link_libraries(ScreenManager Boost::boost SQLite::SQLite3 OpenSSL::SSL OpenSSL::Crypto)
