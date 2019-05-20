file(GLOB HEADERS "include/graphene/query_txid/*.hpp")

add_library( graphene_query_txid
             query_txid_plugin.cpp
           )

target_link_libraries( graphene_query_txid graphene_chain graphene_app leveldb)
target_include_directories( graphene_query_txid
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_query_txid

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)