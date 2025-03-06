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
find_package(Qt6 6.2 REQUIRED Core Gui)
find_package(GettextPo REQUIRED)

add_executable(skyculture-converter
                Utils.cpp
                NamesOldLoader.cpp
                AsterismOldLoader.cpp
                DescriptionOldLoader.cpp
                ConstellationOldLoader.cpp
                main.cpp
              )
target_link_libraries(skyculture-converter Qt::Core Qt::Gui GettextPo::GettextPo)

if(WIN32 AND (NOT MINGW))
    set(shadersDestPath "${CMAKE_PROJECT_NAME}")
    set(installBinDir "${CMAKE_PROJECT_NAME}")
    set(installLibDir "${installBinDir}")
    set(installConfDir "${CMAKE_PROJECT_NAME}/lib")
    set(installIncDir "${CMAKE_PROJECT_NAME}/include")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs")

    # Deploy Qt
    set(libsToInstall Core;Gui)
    foreach(lib ${libsToInstall})
        install(FILES "$<TARGET_FILE:Qt6::${lib}>" DESTINATION "${installBinDir}")
    endforeach()
    install(FILES "${Qt6Core_DIR}/../../../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll" DESTINATION "${installBinDir}/platforms")

else()
    include(GNUInstallDirs)
    set(shadersDestPath "${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}")
    set(installBinDir "${CMAKE_INSTALL_BINDIR}")
    set(installLibDir "${CMAKE_INSTALL_LIBDIR}")
    set(installConfDir "${CMAKE_INSTALL_LIBDIR}")
    set(installIncDir "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

install(TARGETS skyculture-converter DESTINATION "${installBinDir}")
