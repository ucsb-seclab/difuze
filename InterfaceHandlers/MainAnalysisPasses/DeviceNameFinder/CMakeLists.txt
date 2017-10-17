include_directories(include/ ../Utils/include ../Customizations/include ../LinuxKernelCustomizations/include ../WrappedRangeAnalysis/include)

file(GLOB SAA_SRCS
        "src/*.cpp"
        "include/*.h"
        )

add_library(DeviceNameFinderPass MODULE
        # List your source files here.
        ${SAA_SRCS} include/TypePrintHelper.h src/TypePrintHelper.cpp src/DeviceNameFinder.cpp)
add_library(DeviceNameFinderPassStatic STATIC
        # List your source files here.
        ${SAA_SRCS} include/TypePrintHelper.h src/TypePrintHelper.cpp src/DeviceNameFinder.cpp)

target_link_libraries(DeviceNameFinderPass Utils)

target_link_libraries(DeviceNameFinderPassStatic Utils)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(DeviceNameFinderPass PRIVATE cxx_range_for cxx_auto_type)

target_compile_features(DeviceNameFinderPassStatic PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(DeviceNameFinderPassStatic
        PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(DeviceNameFinderPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

set_target_properties(DeviceNameFinderPassStatic PROPERTIES
        COMPILE_FLAGS "-fno-rtti"
        )

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(DeviceNameFinderPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    set_target_properties(DeviceNameFinderPassStatic PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)


