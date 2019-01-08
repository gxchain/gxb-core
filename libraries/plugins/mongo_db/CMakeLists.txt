if(BUILD_MONGO_DB_PLUGIN)
    file(GLOB HEADERS "include/graphene/mongo_db/*.hpp")
    add_library( graphene_mongo_db
                mongo_db_plugin.cpp
                )    
    find_package(libmongoc-1.0 1.8)
    if (libmongoc-1.0_FOUND)

        find_package(libbsoncxx-static REQUIRED)
        message(STATUS "Found bsoncxx headers: ${LIBBSONCXX_STATIC_INCLUDE_DIRS}")
        if((LIBBSONCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBBSONCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBBSONCXX_STATIC_VERSION_MINOR LESS 2)))
            find_library(GRAPHENE_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES}
                PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(GRAPHENE_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found bsoncxx library: ${GRAPHENE_LIBBSONCXX}")

        find_package(libmongocxx-static REQUIRED)
        message(STATUS "Found mongocxx headers: ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}")
        if((LIBMONGOCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBMONGOCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBMONGOCXX_STATIC_VERSION_MINOR LESS 2)))
        find_library(GRAPHENE_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES}
            PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(GRAPHENE_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found mongocxx library: ${GRAPHENE_LIBMONGOCXX}")
    else()
        message("Could NOT find MongoDB. graphene_mongo_db with MongoDB support will not be included.")
    endif()

    target_link_libraries( graphene_mongo_db graphene_chain graphene_app ${GRAPHENE_LIBBSONCXX} ${GRAPHENE_LIBMONGOCXX} )

    target_include_directories( graphene_mongo_db PRIVATE ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

    if(MSVC)
        set_source_files_properties( mongo_db_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
    endif(MSVC)

    install( TARGETS
            graphene_mongo_db

            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            )
else()
    message("mongo_db_plugin not selected and will be omitted.")
endif()

