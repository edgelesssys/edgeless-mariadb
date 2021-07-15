if(EDG_WITH_EDB)
  target_sources(sql PRIVATE ${EDG_EDBDIR}/src/mysqld_edb.cc)
endif()
