# Copyright(c) 2021 ASR Micro Limited
# All Rights Reserved.

aux_source_directory(. SENSORS_TEST_SRC_FILES)

add_executable(cam_sensors_test ${SENSORS_TEST_SRC_FILES})
target_include_directories(cam_sensors_test PRIVATE ../ ../include ../../include ../../../common)
target_link_directories(cam_sensors_test PRIVATE ../../libs/lib64/)
target_link_libraries(cam_sensors_test PRIVATE cam_sensors isp cpp vi tuningtools ispeagle cppfw-2.0)
#target_link_libraries(cam_sensors_test PRIVATE ispeagle cam_sensors)
# target_link_directories(cam_sensors_test PRIVATE ../../libs/lib64/)
install(TARGETS cam_sensors_test RUNTIME DESTINATION usr/bin)
