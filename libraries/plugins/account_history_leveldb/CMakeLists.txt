file(GLOB HEADERS "include/graphene/account_history_leveldb/*.hpp")

add_library( graphene_account_history_leveldb
             account_history_leveldb_plugin.cpp
           )
find_path(LevelDB_INCLUDE_PATH NAMES leveldb/db.h leveldb/write_batch.h)
find_library(LevelDB_LIBRARY NAMES libleveldb.a)
find_library(Snappy_LIBRARY NAMES libsnappy.a)

if(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY AND Snappy_LIBRARY)
   target_link_libraries( graphene_account_history_leveldb graphene_chain graphene_app ${LevelDB_LIBRARY} ${Snappy_LIBRARY})
   target_include_directories( graphene_account_history_leveldb
                              PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${LevelDB_INCLUDE_PATH})
   install( TARGETS
      graphene_account_history_leveldb

      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
   )
else(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY AND Snappy_LIBRARY)
   message(FATAL_ERROR "You need leveldb and snappy")
endif()

