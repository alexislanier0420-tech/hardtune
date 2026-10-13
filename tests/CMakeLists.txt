find_package(Threads REQUIRED)

add_executable(hardtune_tests
    test_engine.cpp
    test_fixed_hop_scheduler.cpp
    test_ring_buffer.cpp
    test_scale.cpp
    test_spsc_queue.cpp)

target_link_libraries(hardtune_tests PRIVATE hardtune_dsp GTest::gtest_main Threads::Threads)

include(GoogleTest)
gtest_discover_tests(hardtune_tests)
