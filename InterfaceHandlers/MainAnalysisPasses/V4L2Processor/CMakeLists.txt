include_directories(include/ ../Utils/include ../Customizations/include ../LinuxKernelCustomizations/include ../WrappedRangeAnalysis/include)

file(GLOB SAA_SRCS
        "src/*.cpp"
        "include/*.h"
        )

add_library(V4L2ProcessorPass MODULE
        # List your source files here.
        ${SAA_SRCS} include/IOInstVisitor.h src/IOInstVisitor.cpp include/TypePrintHelper.h src/TypePrintHelper.cpp src/NewIoctlCmdParser.cpp src/V4L2ConfigProcessor.cpp src/DefIoctlInstVisitor.cpp include/DefIoctlInstVisitor.h)
add_library(V4L2ProcessorPassStatic STATIC
        # List your source files here.
        ${SAA_SRCS} include/IOInstVisitor.h src/IOInstVisitor.cpp include/TypePrintHelper.h src/TypePrintHelper.cpp src/NewIoctlCmdParser.cpp src/V4L2ConfigProcessor.cpp src/DefIoctlInstVisitor.cpp include/DefIoctlInstVisitor.h)

target_link_libraries(V4L2ProcessorPass Utils)

target_link_libraries(V4L2ProcessorPassStatic Utils)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(V4L2ProcessorPass PRIVATE cxx_range_for cxx_auto_type)

target_compile_features(V4L2ProcessorPassStatic PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(V4L2ProcessorPassStatic
        PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(V4L2ProcessorPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

set_target_properties(V4L2ProcessorPassStatic PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(V4L2ProcessorPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    set_target_properties(V4L2ProcessorPassStatic PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)


