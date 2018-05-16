
add_executable( genesis_update genesis_update.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

target_link_libraries( genesis_update
                       PRIVATE graphene_app graphene_chain graphene_db graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( genesis_update
   PUBLIC "${PROJECT_SOURCE_DIR}/libraries/chain/include"
   PUBLIC "${PROJECT_SOURCE_DIR}/libraries/db/include" )

install( TARGETS
   genesis_update

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( get_dev_key get_dev_key.cpp )

target_link_libraries( get_dev_key
                       PRIVATE graphene_app graphene_chain graphene_db graphene_egenesis_none graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   get_dev_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( convert_address convert_address.cpp )
target_link_libraries( convert_address
                       PRIVATE graphene_chain graphene_db fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
target_include_directories( convert_address
   PUBLIC "${PROJECT_SOURCE_DIR}/libraries/chain/include")
