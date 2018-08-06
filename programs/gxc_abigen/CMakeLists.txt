cmake_minimum_required( VERSION 2.8.12 )
set(SOURCES main.cpp)
find_package(LLVM 4.0 REQUIRED CONFIG)

link_directories(${LLVM_LIBRARY_DIR})

add_executable(gxc-abigen ${SOURCES})

set( CMAKE_CXX_STANDARD 14 )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries(gxc-abigen abi_generator)


install( TARGETS
   gxc-abigen

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
