link_directories("${PROJECT_BINARY_DIR}/libraries/chain")
add_executable( size_checker main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_link_libraries( size_checker
                       PRIVATE graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories(size_checker PUBLIC "${PROJECT_SOURCE_DIR}/libraries/chain/include")

install( TARGETS
   size_checker

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
