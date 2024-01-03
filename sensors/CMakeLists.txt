# Copyright(c) 2022 ASR Micro Limited
# All Rights Reserved.

file(GLOB flash_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "flash/*")
file(GLOB sensor_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "sensor/*")
file(GLOB lens_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "vcm/*")
file(GLOB module_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "module/*")

aux_source_directory(. SRC_FILE)
aux_source_directory(flash SRC_FILE)
aux_source_directory(sensor SRC_FILE)
aux_source_directory(vcm SRC_FILE)
aux_source_directory(vcm/i2c SRC_FILE)
aux_source_directory(module SRC_FILE)
foreach(subdir IN ITEMS ${flash_subdirs} ${sensor_subdurs} ${lens_subdirs} ${module_subdirs})
	aux_source_directory(${subdir}/ SRC_FILE)
endforeach()

add_library(cam_sensors ${SRC_FILE})
target_include_directories(cam_sensors BEFORE PRIVATE ./
		PUBLIC include ../libs/include vcm/i2c)
target_compile_definitions(cam_sensors PRIVATE -DCAM_CONFIG_FILE_PATH=\"/tmp/\")
target_link_libraries(cam_sensors PRIVATE isp cpp vi tuningtools ispeagle cppfw-2.0)
install(TARGETS cam_sensors LIBRARY DESTINATION usr/lib)

add_subdirectory(test)
