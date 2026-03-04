add_library(fair-opencmw INTERFACE)
target_include_directories(
  fair-opencmw INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/>)

target_link_libraries(fair-opencmw INTERFACE gr-digitizers-options client)
set_target_properties(
  fair-opencmw
  PROPERTIES
    PUBLIC_HEADER
    "include/fair/opencmw/cmwlight/DirectoryLightClient.hpp;include/fair/opencmw/cmwlight/CmwLightClient.hpp;include/fair/opencmw/OpenCmwSource.hpp"
)

if(ENABLE_GR_DIGITIZERS_TESTING)
  add_subdirectory(test)
endif()
