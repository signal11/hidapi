# IOKit_INCLUDE_DIR
# IOKit_LIBRARIES
# IOKit_FOUND
include(LibFindMacros)

message("Finding CoreFoundation")
# IOKit depends on CoreFoundation
find_package(CoreFoundation REQUIRED)

#message("Finding IOKit headers")
find_path(IOKit_INCLUDE_DIR
	IOKitLib.h
	PATH_SUFFIXES IOKit
)

#message("Finding IOKit library")
find_library(IOKit_LIBRARY
	NAMES IOKit
)

#message("Processing IOKit")
set(IOKit_PROCESS_INCLUDES IOKit_INCLUDE_DIR CoreFoundation_INCLUDE_DIR)
set(IOKit_PROCESS_LIBS IOKit_LIBRARY CoreFoundation_LIBRARIES)
libfind_process(IOKit)
