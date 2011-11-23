# CoreFoundation_INCLUDE_DIR
# CoreFoundation_LIBRARIES
# CoreFoundation_FOUND
include(LibFindMacros)

message("Finding CF headers")
find_path(CoreFoundation_INCLUDE_DIR
	CoreFoundation.h
	PATH_SUFFIXES CoreFoundation
)

#message("Finding CF lib")
find_library(CoreFoundation_LIBRARY
	NAMES CoreFoundation
)

#message("Processing CF")
set(CoreFoundation_PROCESS_INCLUDES CoreFoundation_INCLUDE_DIR)
set(CoreFoundation_PROCESS_LIBS CoreFoundation_LIBRARY)
libfind_process(CoreFoundation)
