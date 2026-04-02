# Find and set Boost flags
# Bypass FindBoost and directly set variables

set(BOOST_FOUND TRUE)
set(BOOST_INCLUDE_DIRS "/usr/include")
set(BOOST_LIBRARY_DIRS "/usr/lib/aarch64-linux-gnu;/lib/aarch64-linux-gnu")

# Set individual library variables
set(Boost_INCLUDE_DIRS "/usr/include")
set(Boost_LIBRARY_DIRS "/usr/lib/aarch64-linux-gnu;/lib/aarch64-linux-gnu")

# Define found components
set(Boost_SYSTEM_FOUND TRUE)
set(Boost_FILESYSTEM_FOUND TRUE)
set(Boost_THREAD_FOUND TRUE)
set(Boost_DATE_TIME_FOUND TRUE)
set(Boost_IOSTREAMS_FOUND TRUE)

# Library names with version
set(Boost_SYSTEM_LIBRARY "/usr/lib/aarch64-linux-gnu/libboost_system.so.1.74.0")
set(Boost_FILESYSTEM_LIBRARY "/usr/lib/aarch64-linux-gnu/libboost_filesystem.so.1.74.0")
set(Boost_THREAD_LIBRARY "/usr/lib/aarch64-linux-gnu/libboost_thread.so.1.74.0")
set(Boost_DATE_TIME_LIBRARY "/usr/lib/aarch64-linux-gnu/libboost_date_time.so.1.74.0")
set(Boost_IOSTREAMS_LIBRARY "/usr/lib/aarch64-linux-gnu/libboost_iostreams.so.1.74.0")

# For compatibility
set(Boost_LIBRARIES 
    "${Boost_SYSTEM_LIBRARY}"
    "${Boost_FILESYSTEM_LIBRARY}"
    "${Boost_THREAD_LIBRARY}"
    "${Boost_DATE_TIME_LIBRARY}"
    "${Boost_IOSTREAMS_LIBRARY}"
)

# Add diagnostic definitions
set(Boost_LIB_DIAGNOSTIC_DEFINITIONS "-DBOOST_ALL_DYN_LINK")

message(STATUS "Boost found (manual configuration)")
message(STATUS "  Includes: ${Boost_INCLUDE_DIRS}")
message(STATUS " ")