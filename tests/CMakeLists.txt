add_executable(hardtune_tests
    test_scale.cpp)

target_link_libraries(hardtune_tests PRIVATE hardtune_dsp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hardtune_tests)
