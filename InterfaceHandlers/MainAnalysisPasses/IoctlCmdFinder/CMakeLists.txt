include_directories(include/ ../Utils/include ../WrappedRangeAnalysis/include)

file(GLOB SAA_SRCS
        "src/*.cpp"
        "include/*.h"
        )

add_library(IoctlCmdPass MODULE
        # List your source files here.
        ${SAA_SRCS} include/IOInstVisitor.h src/IOInstVisitor.cpp include/TypePrintHelper.h src/TypePrintHelper.cpp src/NewIoctlCmdParser.cpp)
add_library(IoctlCmdPassStatic STATIC
        # List your source files here.
        ${SAA_SRCS} include/IOInstVisitor.h src/IOInstVisitor.cpp include/TypePrintHelper.h src/TypePrintHelper.cpp src/NewIoctlCmdParser.cpp)

target_link_libraries(IoctlCmdPass Utils)

target_link_libraries(IoctlCmdPassStatic Utils)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(IoctlCmdPass PRIVATE cxx_range_for cxx_auto_type)

target_compile_features(IoctlCmdPassStatic PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(IoctlCmdPassStatic
        PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(IoctlCmdPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

set_target_properties(IoctlCmdPassStatic PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(IoctlCmdPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    set_target_properties(IoctlCmdPassStatic PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)


