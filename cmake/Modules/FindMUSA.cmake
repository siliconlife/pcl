# - Try to find MUSA (Moore Threads GPU)
# Once done this will define
#  MUSA_FOUND - System has MUSA
#  MUSA_INCLUDE_DIR - MUSA include directory
#  MUSA_LIBRARY_DIR - MUSA library directory
#  MUSA_LIBRARIES - Libraries needed to use MUSA
#  MUSA_MCC_BIN - Path to mcc compiler

find_path(MUSA_INCLUDE_DIR musa_runtime.h
    /usr/local/musa/include
    /opt/musa/include
)

find_library(MUSA_RUNTIME_LIBRARY musart
    /usr/local/musa/lib64
    /usr/local/musa/lib
    /opt/musa/lib64
)

find_file(MUSA_MCC_BIN mcc
    /usr/local/musa/bin
    /opt/musa/bin
)

if (MUSA_INCLUDE_DIR AND MUSA_RUNTIME_LIBRARY)
    set(MUSA_FOUND TRUE)
    set(MUSA_LIBRARIES ${MUSA_RUNTIME_LIBRARY})
    set(MUSA_LIBRARY_DIR "/usr/local/musa/lib" CACHE PATH "MUSA library directory")
else ()
    set(MUSA_FOUND FALSE)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MUSA
    DEFAULT_MSG
    MUSA_INCLUDE_DIR
    MUSA_RUNTIME_LIBRARY
)
