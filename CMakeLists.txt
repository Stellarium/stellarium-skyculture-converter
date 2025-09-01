cmake_minimum_required(VERSION 3.10.0)
project(skyculture-converter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

FIND_PACKAGE(Qt6 COMPONENTS Core Gui Xml)

find_package(GettextPo REQUIRED)
find_package(LibTidy REQUIRED)

# Create a library for core converter components (loaders, utils, converter)
add_library(libskycultureconverter
    Utils.cpp
    SkyCultureConverter.cpp
    NamesOldLoader.cpp
    AsterismOldLoader.cpp
    DescriptionOldLoader.cpp
    ConstellationOldLoader.cpp
)
target_include_directories(libskycultureconverter
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(libskycultureconverter
    PUBLIC Qt::Core Qt::Gui Qt::Xml
           GettextPo::GettextPo LibTidy::LibTidy
)

# Build the CLI executable linking against the library
add_executable(skyculture-converter
    main.cpp
)
# Link executable to the converter library
target_link_libraries(skyculture-converter
    PRIVATE libskycultureconverter
)

if(WIN32 AND (NOT MINGW))
    set(installBinDir ".")
    set(installLibDir "${installBinDir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs")

    # Deploy Qt
    set(libsToInstall Core;Gui;Xml)
    foreach(lib ${libsToInstall})
        install(FILES "$<TARGET_FILE:Qt6::${lib}>" DESTINATION "${installBinDir}")
    endforeach()
    install(FILES "${Qt6Core_DIR}/../../../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll" DESTINATION "${installBinDir}/platforms")
    foreach(format qgif qjpeg qtga qtiff qwbmp qwebp)
            install(FILES "${Qt6Core_DIR}/../../../plugins/imageformats/${format}$<$<CONFIG:Debug>:d>.dll" DESTINATION "${installBinDir}/plugins/imageformats")
    endforeach()

else()
    include(GNUInstallDirs)
    set(installBinDir "${CMAKE_INSTALL_BINDIR}")
    set(installLibDir "${CMAKE_INSTALL_LIBDIR}")
endif()

install(TARGETS skyculture-converter DESTINATION "${installBinDir}")
