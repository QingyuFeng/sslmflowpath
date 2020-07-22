cmake_minimum_required(VERSION 3.1.0)
project(sslmfp)

set (CMAKE_CXX_STANDARD 11)

set (common_srcs commonLib.cpp tiffIO.cpp)

set (D8DIST2SUBOLT dist2suboltmn.cpp dist2subolt.cpp ${common_srcs})
set (D8DIST2WSOLT dist2wsoltmn.cpp dist2wsolt.cpp ${common_srcs})
set (LORENZFPSUB lorenzfpsubmn.cpp lorenzfpsub.cpp ${common_srcs})
set (LORENZFPWS lurenzfpwsmn.cpp lurenzfpws.cpp ${common_srcs})
set (SUBINDEXMAP subindexmapmn.cpp subindexmap.cpp ${common_srcs})

# MPI is required
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAG "${CMAKE_CXX_FLAG} ${MPI_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS}")

# GDAL is required
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})

add_executable (dist2subolt ${D8DIST2SUBOLT})
add_executable (dist2wsolt ${D8DIST2WSOLT})
add_executable (lorenzfpsub ${LORENZFPSUB})
add_executable (lorenzfpws ${LORENZFPWS})
add_executable (subindexmap ${SUBINDEXMAP})


set (MY_TARGETS dist2subolt 
                dist2wsolt    
                lorenzfpsub
                lorenzfpws
				subindexmap)

foreach( c_target ${MY_TARGETS} )
    target_link_libraries(${c_target} ${MPI_LIBRARIES} ${GDAL_LIBRARY})
    install(TARGETS ${c_target} DESTINATION sslmfp)
endforeach( c_target ${MY_TARGETS} )
