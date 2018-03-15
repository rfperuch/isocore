#.rst:
# FindCUnit
# -----------
#
# Find the CUnit headers and libraries
#
# ::
#
#  CUNIT_INCLUDE_DIRS - The CUnit include directory (directory where CUnit/CUnit.h was found)
#  CUNIT_LIBRARIES    - The libraries needed to use CUnit
#  CUNIT_FOUND        - True if CUnit found in system


find_path(CUNIT_INCLUDE_DIR NAMES CUnit/CUnit.h)
find_library(CUNIT_LIBRARY NAMES
    cunit
    libcunit
    cunitlib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUnit DEFAULT_MSG CUNIT_LIBRARY CUNIT_INCLUDE_DIR)

if (CUNIT_FOUND)
    set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
    set(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})

  if(NOT TARGET CUnit::CUnit)
      add_library(CUnit::CUnit UNKNOWN IMPORTED)
      set_target_properties(CUnit::CUnit PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CUNIT_INCLUDE_DIR}")
      set_property(TARGET CUnit::CUnit APPEND PROPERTY
        IMPORTED_LOCATION "${CUNIT_LIBRARY}")
    endif()
endif()

mark_as_advanced(CUNIT_LIBRARY CUNIT_INCLUDE_DIR)
