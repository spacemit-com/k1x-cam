# Copyright(c) 2023 Spacemit Limited
# All Rights Reserved.

file(GLOB flash_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "flash/*")
file(GLOB sensor_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "sensor/*")
file(GLOB lens_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "vcm/*")
file(GLOB module_subdirs LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "module/*")

aux_source_directory(. SRC_FILE)
aux_source_directory(flash SRC_FILE)
aux_source_directory(sensor SRC_FILE)
aux_source_directory(vcm SRC_FILE)
# aux_source_directory(vcm/i2c SRC_FILE)
aux_source_directory(module SRC_FILE)
foreach(subdir IN ITEMS ${flash_subdirs} ${sensor_subdirs} ${lens_subdirs} ${module_subdirs})
	if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
		aux_source_directory(${subdir}/ SRC_FILE)
	endif()
endforeach()

add_compile_options(-Wno-implicit-function-declaration)
add_library(cam_sensors SHARED ${SRC_FILE})
target_include_directories(cam_sensors BEFORE PRIVATE ./
		PUBLIC include ../libs/include)
# target_include_directories(cam_sensors BEFORE PRIVATE ./
# 		PUBLIC include ../libs/include vcm/i2c)
target_compile_definitions(cam_sensors PRIVATE -DCAM_CONFIG_FILE_PATH=\"/tmp/\")
install(TARGETS cam_sensors LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_subdirectory(test)
