set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(MotionXUpdater)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

option(WITH_SHARED "Build shared version of hidapi" OFF)
option(WITH_STATIC "Build static version of hidapi" ON)

if (WIN32 OR CYGWIN)
	list(APPEND SRC windows/hid.c)
	list(APPEND HIDAPI_LIBS setupapi)
elseif(APPLE)
	list(APPEND SRC mac/hid.c)
	find_package(IOKit REQUIRED)
	include_directories(${IOKit_INCLUDE_DIR})
	list(APPEND HIDAPI_LIBS ${IOKit_LIBRARIES})
else()
	find_package(LIBUSB REQUIRED)
	list(APPEND SRC linux/hid-libusb.c)
	list(APPEND HIDAPI_LIBS ${LIBUSB_LIBRARIES})
	include_directories(${LIBUSB_INCLUDE_DIR})
endif()
	
include_directories(hidapi)

if (WITH_SHARED)
	add_library(hidapi SHARED ${SRC})
	set_target_properties(hidapi PROPERTIES CLEAN DIRECT OUTPUT 1)
	target_link_libraries(hidapi ${HIDAPI_LIBS})
	list(APPEND BUILT_LIBS hidapi)
endif()

if (WITH_STATIC)
	add_library(hidapi-static ${SRC})
	set_target_properties(hidapi-static PROPERTIES OUTPUT_NAME hidapi CLEAN DIRECT OUTPUT 1)
	if (WIN32 AND NOT CYGWIN)
		set_target_properties(hidapi-static PROPERTIES PREFIX "lib")
	endif()
	target_link_libraries(hidapi-static ${HIDAPI_LIBS})
	list(APPEND BUILT_LIBS hidapi-static)
endif()

install(TARGETS ${BUILT_LIBS} 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	)

install(FILES hidapi/hidapi.h DESTINATION include/)
