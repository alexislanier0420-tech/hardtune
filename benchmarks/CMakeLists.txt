add_executable(hardtune_benchmarks
    bench_dsp.cpp)

target_link_libraries(hardtune_benchmarks PRIVATE hardtune_dsp benchmark::benchmark_main)
