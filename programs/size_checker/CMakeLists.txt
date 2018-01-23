link_directories("${PROJECT_BINARY_DIR}/libraries/chain")
link_libraries(graphene_chain)
add_executable( size_checker main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

link_libraries("${PROJECT_SOURCE_DIR}/libraries/chain/")
target_link_libraries( size_checker
                       PRIVATE graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   size_checker

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
