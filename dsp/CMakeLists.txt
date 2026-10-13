add_library(hardtune_dsp STATIC
    src/Engine.cpp
    src/Scale.cpp)

target_include_directories(hardtune_dsp PUBLIC include)
target_compile_features(hardtune_dsp PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hardtune_dsp PRIVATE /W4 /permissive-)
else()
    target_compile_options(hardtune_dsp PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
