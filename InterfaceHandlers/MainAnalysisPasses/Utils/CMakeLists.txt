project(Utils C CXX)

add_library(${PROJECT_NAME} STATIC include/CFGUtils.h src/CFGUtils.cpp include/InstructionUtils.h src/InstructionUtils.cpp include/FileUtils.h src/FileUtils.cpp)

include_directories(include/)
# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

set_target_properties(${PROJECT_NAME}
        PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)

target_link_libraries(${PROJECT_NAME})

set(${PROJECT_NAME}_INCLUDE_DIRS include
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)