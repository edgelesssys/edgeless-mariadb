target_sources(misc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)

if(EDG_WITH_EDB)
  target_sources(sql PRIVATE ${EDG_EDBDIR}/src/mysqld_edb.cc)
  target_sources(basic-t PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
  target_sources(connection PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
  target_sources(ps_bugs PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmariadb_skip_tests.c)
endif()

if(EDG_WITH_EROCKS)
  # These targets would need to be correctly configured for erocks, but we don't need them. So just exclude from all.
  set_target_properties(mariadb-ldb rocksdb_tools PROPERTIES EXCLUDE_FROM_ALL ON)
endif()
