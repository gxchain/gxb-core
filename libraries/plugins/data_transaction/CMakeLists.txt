file(GLOB HEADERS "include/graphene/data_transaction/*.hpp")

add_library( graphene_data_transaction
             data_transaction_plugin.cpp
           )

target_link_libraries( graphene_data_transaction graphene_chain graphene_app )
target_include_directories( graphene_data_transaction
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_data_transaction

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
