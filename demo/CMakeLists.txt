add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
add_definitions(-DNDEBUG)

aux_source_directory(./ SDKTEST_SRC_FILE)
aux_source_directory(utils SDKTEST_SRC_FILE)
aux_source_directory(include SDKTEST_SRC_FILE)
aux_source_directory(include/dmabufheap SDKTEST_SRC_FILE)
aux_source_directory(include/opengles SDKTEST_SRC_FILE)
aux_source_directory(extern SDKTEST_SRC_FILE)
aux_source_directory(gst_api SDKTEST_SRC_FILE)
aux_source_directory(v4l2_test SDKTEST_SRC_FILE)

include_directories(./)
include_directories(./include)
include_directories(./include/dmabufheap)
include_directories(./include/opengles/)
include_directories(./utils)
include_directories(./extern)
include_directories(./gst_api/)
include_directories(./v4l2_test/)
include_directories(../libs/include)

# message("test ------------------- ${CMAKE_INSTALL_PREFIX}, ${PROJECT_SOURCE_DIR}, ${CMAKE_BINARY_DIR} ")

add_library(sdkcam SHARED ${SDKTEST_SRC_FILE})
target_compile_options(sdkcam PUBLIC -lstdc++;-Wall;-Wno-unused;-Werror;-Wno-implicit-fallthrough;-Wno-error=pointer-sign;-Wno-error=implicit-function-declaration;-Wno-error=int-conversion)
target_compile_definitions(sdkcam PUBLIC -D_GNU_SOURCE)

# Add Link Library
set(LINK_GPU_LIBRARIES
    EGL
    GLESv2
    wayland-client
    wayland-server
    wayland-egl
    m
    wayland-cursor
    gbm
)

target_link_directories(sdkcam PUBLIC ../libs/lib64/)
target_link_libraries(sdkcam PUBLIC isp cpp vi cam_sensors tuningtools ispeagle cppfw-2.0 ccic ${LINK_GPU_LIBRARIES})
install(TARGETS sdkcam LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

set(SRC_LIST ./main.c)
add_executable(cam-test ${SRC_LIST})
target_link_directories(cam-test PRIVATE ./ ../libs/lib64/)
target_link_libraries(cam-test PUBLIC sdkcam isp cpp vi cam_sensors tuningtools ispeagle cppfw-2.0 ccic)

target_compile_options(cam-test PRIVATE -lstdc++;-Wall;-Wno-unused;-Werror;-Wno-implicit-fallthrough;-Wno-error=pointer-sign)
target_compile_definitions(cam-test PRIVATE -D_GNU_SOURCE)

install(TARGETS cam-test RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(DIRECTORY ../libs/lib64/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        FILES_MATCHING PATTERN "*.so"
        )

install(DIRECTORY ./
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )

install(DIRECTORY ../libs/include/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )

install(DIRECTORY ../sensors/include/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )

install(DIRECTORY ./include/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
install(DIRECTORY ./include/dmabufheap/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
install(DIRECTORY ./include/opengles/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
install(DIRECTORY ./utils/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
install(DIRECTORY ./extern/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
install(DIRECTORY ./gst_api/
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
        FILES_MATCHING PATTERN "*.h"
        )
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/pkgconfig/k1x-cam.pc.cmake"
        "${CMAKE_BINARY_DIR}/k1x-cam.pc" @ONLY)

install(FILES "${CMAKE_BINARY_DIR}/k1x-cam.pc"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/")

