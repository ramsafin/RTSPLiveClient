set(Live555_SRC_DIR /usr/lib/live)
set(Live555_LIBRARIES "")
set(Live555_INCLUDE_DIRS "")
set(Live555_FOUND FALSE)

# LIBS
foreach(Live555_module liveMedia groupsock BasicUsageEnvironment UsageEnvironment)
    find_library(${Live555_module}_LIB ${Live555_module} PATHS ${Live555_SRC_DIR}/${Live555_module})
    if (${Live555_module}_LIB)
        list(APPEND Live555_LIBRARIES ${${Live555_module}_LIB})
    elseif(NOT ${Live555_module}_LIB)
        set(Live555_FOUND FALSE)
        break()
    endif()
endforeach(Live555_module)

# INCLUDES
foreach(Live555_module UsageEnvironment groupsock liveMedia BasicUsageEnvironment)
    if (EXISTS ${Live555_SRC_DIR}/${Live555_module})
        list(APPEND Live555_INCLUDE_DIRS ${Live555_SRC_DIR}/${Live555_module}/include)
    else()
        set(Live555_FOUND FALSE)
        break()
    endif()
endforeach()

if (Live555_INCLUDE_DIRS AND Live555_LIBRARIES)
    set(Live555_FOUND TRUE)
endif()