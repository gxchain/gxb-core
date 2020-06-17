file(GLOB HEADERS "include/graphene/account_history_leveldb/*.hpp")

add_library( graphene_account_history_leveldb
             account_history_leveldb_plugin.cpp
           )
find_path(RocksDB_INCLUDE_PATH NAMES rocksdb/db.h rocksdb/write_batch_base.h rocksdb/write_batch.h)
find_library(RocksDB_LIBRARY NAMES librocksdb.a)
find_library(Snappy_LIBRARY NAMES libsnappy.a)

if(RocksDB_INCLUDE_PATH AND RocksDB_LIBRARY AND Snappy_LIBRARY)
   target_link_libraries( graphene_account_history_leveldb graphene_chain graphene_app ${RocksDB_LIBRARY} ${Snappy_LIBRARY})
   target_include_directories( graphene_account_history_leveldb
                              PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${RocksDB_INCLUDE_PATH})
   install( TARGETS
      graphene_account_history_leveldb

      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
   )
else(RocksDB_INCLUDE_PATH AND RocksDB_LIBRARY AND Snappy_LIBRARY)
   message(FATAL_ERROR "You need rocksdb and snappy")
endif()

