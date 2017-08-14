link_directories("${PROJECT_SOURCE_DIR}/libraries/chain/lib")

file(GLOB HEADERS "include/graphene/chain/*.hpp")
file(GLOB PROTOCOL_HEADERS "include/graphene/chain/protocol/*.hpp")

if(APPLE)
    MESSAGE (STATUS "------ OS X --------")
    file(GLOB GRAPHENE_CHAIN_LIB "lib/libgraphene_chain.dylib")
elseif(LINUX)
    MESSAGE (STATUS "------ linux --------")
    file(GLOB GRAPHENE_CHAIN_LIB "lib/libgraphene_chain.so")
endif()

INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/chain" )
INSTALL( FILES ${PROTOCOL_HEADERS} DESTINATION "include/graphene/chain/protocol" )
FILE ( COPY ${GRAPHENE_CHAIN_LIB} DESTINATION
    ${CMAKE_BINARY_DIR}/libraries/chain )
