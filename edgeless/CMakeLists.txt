target_sources(misc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)

if(WITH_EDB)
  target_sources(basic-t PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
  target_sources(connection PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
  target_sources(ps_bugs PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
endif()

if(WITH_EROCKS)
  # These targets would need to be correctly configured for erocks, but we don't need them. So just exclude from all.
  set_target_properties(mariadb-ldb rocksdb_tools PROPERTIES EXCLUDE_FROM_ALL ON)
endif()
