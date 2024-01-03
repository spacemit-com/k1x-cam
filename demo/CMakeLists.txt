aux_source_directory(./ SDKTEST_SRC_FILE)
aux_source_directory(utils SDKTEST_SRC_FILE)
aux_source_directory(include SDKTEST_SRC_FILE)
aux_source_directory(include/dmabufheap SDKTEST_SRC_FILE)
add_executable(sdkTest ${SDKTEST_SRC_FILE})

target_link_libraries(sdkTest PUBLIC isp cpp vi cam_sensors tuningtools ispeagle cppfw-2.0)
target_link_directories(sdkTest PRIVATE ../libs/lib64/ ./utils/lib64)

target_include_directories(sdkTest PRIVATE
        ./
        ./include/
        ./include/dmabufheap
        ./utils/
        ../libs/include
    )

target_compile_options(sdkTest PRIVATE -lstdc++;-Wall;-Wno-unused;-Werror;-Wno-implicit-fallthrough)
target_compile_definitions(sdkTest PRIVATE -D_GNU_SOURCE)

install(TARGETS sdkTest RUNTIME DESTINATION usr/bin)
