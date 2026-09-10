cmake_minimum_required(VERSION 3.21)

project(ScopeHTTP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Crow CONFIG REQUIRED)

if(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$" OR
   CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]64$")
    set(SCOPEHTTP_SOURCE arm64.cpp)
else()
    set(SCOPEHTTP_SOURCE x64.cpp)
endif()

add_executable(ScopeHTTP ${SCOPEHTTP_SOURCE})

target_link_libraries(ScopeHTTP PRIVATE Crow::Crow)

if(WIN32)
    # Asio uses Winsock on Windows. Crow's package supplies the Asio headers;
    # ws2_32 supplies the operating-system networking symbols.
    target_link_libraries(ScopeHTTP PRIVATE ws2_32)
endif()

target_compile_definitions(ScopeHTTP PRIVATE
    _WIN32_WINNT=0x0601
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(ScopeHTTP PRIVATE /W4 /permissive-)
    if(SCOPEHTTP_SOURCE STREQUAL "arm64.cpp")
        target_compile_options(ScopeHTTP PRIVATE
            "$<$<CONFIG:Release>:/O2>"
            "$<$<CONFIG:Release>:/Oi>"
            "$<$<CONFIG:Release>:/Ot>"
        )
    endif()
else()
    target_compile_options(ScopeHTTP PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Keep the document root beside the executable in both single- and
# multi-configuration generators (Visual Studio, Ninja Multi-Config, etc.).
add_custom_command(TARGET ScopeHTTP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/public"
            "$<TARGET_FILE_DIR:ScopeHTTP>/public"
    COMMENT "Copying ScopeHTTP public assets"
)

install(TARGETS ScopeHTTP RUNTIME DESTINATION .)
install(DIRECTORY public/ DESTINATION public)
