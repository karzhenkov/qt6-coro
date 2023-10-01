link_libraries(Qt::Test coro)

foreach(TEST_NAME
  test1
  test2
  test3
  test4
  test5
  test6
)
  qt_add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

endforeach()
