cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../aux/cmake/Modules" ${CMAKE_MODULE_PATH})

set(Boost_USE_MULTITHREADED ON )

find_package(PkgConfig)
find_package( Boost 1.36.0 COMPONENTS filesystem system REQUIRED )
find_package( TagLib REQUIRED )
find_package( Sqlite REQUIRED )
find_package( Threads REQUIRED )

add_definitions( ${SQLITE_DEFINITIONS} )

set(SRCS     mediascanner.cpp
             CEventLoop.cpp
             CMediaScanner.cpp
             CFsScanner.cpp
             CStateDbUpdater.cpp
             CMediaColUpdater.cpp
             CMediaItemCategorizer.cpp
             # CMediaItem.cpp 
             CMsgBase.cpp
             CMsgError.cpp
             CMsgQuit.cpp
             CMsgResponse.cpp
             CMsgOpenDb.cpp
             CMsgScanDir.cpp
             CMsgFinished.cpp
             CMsgProgress.cpp
             CMsgCollectionChanged.cpp
             ../msessiond/CStateDbBase.cpp
             ../msessiond/CSubProcess.cpp
             ../msessiond/sessionEx.cpp 
             )

set( HEADER  CMediaScanner.h
             CEventLoop.h
             CFsScanner.h
             CStateDbUpdater.h
             CMediaColUpdater.h
             CMediaItemCategorizer.h
             # CMediaItem.h 
             CMsgBase.h
             CMsgError.h
             CMsgQuit.h
             CMsgResponse.h
             CMsgOpenDb.h
             CMsgScanDir.h
             CMsgFinished.h
             CMsgProgress.h
             CMsgCollectionChanged.h
             ../msessiond/CStateDbBase.h
             ../msessiond/CSubProcess.h
             ../msessiond/sessionEx.h 
             )


include_directories(${SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${TagLib_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIR} 
                    "${CMAKE_CURRENT_SOURCE_DIR}/../msessiond" "${CMAKE_CURRENT_SOURCE_DIR}/../libmuroa++") 

link_directories(${BINARY_DIR} ${SQLITE_LIBRARY_DIRS} "${CMAKE_SOURCE_DIR}/../libmuroa++/build")

ADD_EXECUTABLE(mmscanner ${SRCS} )

set_target_properties( mmscanner PROPERTIES  COMPILE_FLAGS "-std=gnu++0x" ) 

TARGET_LINK_LIBRARIES(mmscanner ${CMAKE_THREAD_LIBS_INIT} 
                                ${Boost_LIBRARIES} 
                                ${TagLib_LIBRARIES}  
                                ${SQLITE_LIBRARIES} 
                                muroa++
                                ${libxdiff_LIBRARIES}
                                ${liblog4cplus_LIBRARIES}
                                )

                              
###################################################################################
# This step merges a gdb_index section into the binary to speed up debugging.
# Doing this by add_custom_command will become obsolete as soon as the linker does it. 
###################################################################################
add_custom_command(TARGET mmscanner
                   POST_BUILD  
                   COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/../aux/scripts/mkgdbindex.sh mmscanner 
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   COMMENT "building gdb index")
                                   
###################################################################################      




if(${unittests})
    find_package( CppUnit REQUIRED )
    find_package ( Qt4 REQUIRED COMPONENTS QtCore QtGui QtNetwork QtXml QtDbus )
    
    include ( ${QT_USE_FILE} )
                                       
    set(unittest_SRCS      unittest/CCategorizerTest.cpp
                           )
    
    set(unittests_HEADERS  unittest/CCategorizerTest.h 
                           )
                                       
    ADD_EXECUTABLE(mmscannertest ${unittests_SRCS} ${unittests_HEADERS} ${SRCS} ${HEADERS} ${CPPUNIT_INCLUDE_DIRS})
    
    set_target_properties( mmscannertest PROPERTIES  COMPILE_FLAGS "-std=gnu++0x" ) 
    
    TARGET_LINK_LIBRARIES(mmscannertest  ${CPPUNIT_LIBRARIES} 
                                         muroa++
                                         ${CMAKE_THREAD_LIBS_INIT}
                                         ${Boost_LIBRARIES} 
                                         ${TagLib_LIBRARIES} 
                                         ${QT_LIBRARIES} 
                                         ${SQLITE_LIBRARIES} 
                                         xdiff
                                         ${ALSA_LIBRARY}
                                         avahi-client
                                         avahi-qt4
                                         avformat
                                         avcodec
                                         avutil  
                                         qxcppunitd 
                                         qxrunnerd
                                         )
                                         
    ################################################################################
    # This step merges a gdb_index section into the binary to speed up debugging.
    # Doing this by add_custom_command will become obsolete as soon as the linker does it. 
    ################################################################################
    
    add_custom_command(TARGET mmscannertest
                       POST_BUILD  
                       COMMAND  ${CMAKE_CURRENT_SOURCE_DIR}/../aux/scripts/mkgdbindex.sh mmscannertest 
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                       COMMENT "building gdb index")
    
    ################################################################################                                     
                                                                    
endif(${unittests})                    
            