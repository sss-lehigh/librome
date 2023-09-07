
function(add_test_executable TARGET SOURCE)
    set(options DISABLE_TEST)
    set(oneValueArgs "")
    set(multiValueArgs LIBS)

    cmake_parse_arguments(_ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_executable(${TARGET} ${SOURCE})
   
    foreach(NAME "${TARGET}")
        target_link_libraries(${NAME} PRIVATE rome GTest::gtest GTest::gtest_main GTest::gmock ${__LIBS})
    endforeach()
    
    if(NOT ${__DISABLE_TEST})
        add_test(NAME ${TARGET} COMMAND ${TARGET})
    endif()
endfunction()

