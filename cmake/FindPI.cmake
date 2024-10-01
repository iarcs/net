#[=======================================================================[.rst:
FindPI
----------

Finds the PI library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``PI::PI``
  The PI library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``PI_FOUND``
  True if the system has the PI library.
``PI_INCLUDE_DIRS``
  Include directories needed to use PI.
``PI_LIBRARIES``
  Libraries needed to link to PI.

#]=======================================================================]

find_path(PI_INCLUDE_DIRS NAMES pi.h proto/pi_server.h PATH_SUFFIXES PI)

find_library(LIB_pi             NAMES libpi.a               pi)
find_library(LIB_pi_dummy       NAMES libpi_dummy.a         pi_dummy)
find_library(LIB_piall          NAMES libpiall.a            piall)
find_library(LIB_piconvertproto NAMES libpiconvertproto.a   piconvertproto)
find_library(LIB_pifecpp        NAMES libpifecpp.a          pifecpp)
find_library(LIB_pifegeneric    NAMES libpifegeneric.a      pifegeneric)
find_library(LIB_pifeproto      NAMES libpifeproto.a        pifeproto)
find_library(LIB_pigrpcserver   NAMES libpigrpcserver.a     pigrpcserver)
find_library(LIB_pip4info       NAMES libpip4info.a         pip4info)
find_library(LIB_piprotobuf     NAMES libpiprotobuf.a       piprotobuf)
find_library(LIB_piprotogrpc    NAMES libpiprotogrpc.a      piprotogrpc)

set(PI_LIBRARIES
    "${LIB_pi}"
    "${LIB_pi_dummy}"
    "${LIB_piall}"
    "${LIB_piconvertproto}"
    "${LIB_pifecpp}"
    "${LIB_pifegeneric}"
    "${LIB_pifeproto}"
    "${LIB_pigrpcserver}"
    "${LIB_pip4info}"
    "${LIB_piprotobuf}"
    "${LIB_piprotogrpc}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PI
    FOUND_VAR PI_FOUND
    REQUIRED_VARS PI_INCLUDE_DIRS PI_LIBRARIES)
string(REPLACE ";" " " PI_LIBRARIES "${PI_LIBRARIES}")

if(PI_FOUND)
    add_library(PI::pi              UNKNOWN IMPORTED)
    add_library(PI::pi_dummy        UNKNOWN IMPORTED)
    add_library(PI::piall           UNKNOWN IMPORTED)
    add_library(PI::piconvertproto  UNKNOWN IMPORTED)
    add_library(PI::pifecpp         UNKNOWN IMPORTED)
    add_library(PI::pifegeneric     UNKNOWN IMPORTED)
    add_library(PI::pifeproto       UNKNOWN IMPORTED)
    add_library(PI::pigrpcserver    UNKNOWN IMPORTED)
    add_library(PI::pip4info        UNKNOWN IMPORTED)
    add_library(PI::piprotobuf      UNKNOWN IMPORTED)
    add_library(PI::piprotogrpc     UNKNOWN IMPORTED)
    target_link_libraries(PI::pi              INTERFACE ${LIB_pi})
    target_link_libraries(PI::pi_dummy        INTERFACE ${LIB_pi_dummy})
    target_link_libraries(PI::piall           INTERFACE ${LIB_piall})
    target_link_libraries(PI::piconvertproto  INTERFACE ${LIB_piconvertproto})
    target_link_libraries(PI::pifecpp         INTERFACE ${LIB_pifecpp})
    target_link_libraries(PI::pifegeneric     INTERFACE ${LIB_pifegeneric})
    target_link_libraries(PI::pifeproto       INTERFACE ${LIB_pifeproto})
    target_link_libraries(PI::pigrpcserver    INTERFACE ${LIB_pigrpcserver})
    target_link_libraries(PI::pip4info        INTERFACE ${LIB_pip4info})
    target_link_libraries(PI::piprotobuf      INTERFACE ${LIB_piprotobuf})
    target_link_libraries(PI::piprotogrpc     INTERFACE ${LIB_piprotogrpc})
    set_target_properties(PI::pi              PROPERTIES IMPORTED_LOCATION ${LIB_pi})
    set_target_properties(PI::pi_dummy        PROPERTIES IMPORTED_LOCATION ${LIB_pi_dummy})
    set_target_properties(PI::piall           PROPERTIES IMPORTED_LOCATION ${LIB_piall})
    set_target_properties(PI::piconvertproto  PROPERTIES IMPORTED_LOCATION ${LIB_piconvertproto})
    set_target_properties(PI::pifecpp         PROPERTIES IMPORTED_LOCATION ${LIB_pifecpp})
    set_target_properties(PI::pifegeneric     PROPERTIES IMPORTED_LOCATION ${LIB_pifegeneric})
    set_target_properties(PI::pifeproto       PROPERTIES IMPORTED_LOCATION ${LIB_pifeproto})
    set_target_properties(PI::pigrpcserver    PROPERTIES IMPORTED_LOCATION ${LIB_pigrpcserver})
    set_target_properties(PI::pip4info        PROPERTIES IMPORTED_LOCATION ${LIB_pip4info})
    set_target_properties(PI::piprotobuf      PROPERTIES IMPORTED_LOCATION ${LIB_piprotobuf})
    set_target_properties(PI::piprotogrpc     PROPERTIES IMPORTED_LOCATION ${LIB_piprotogrpc})

    add_library(PI::PI INTERFACE IMPORTED)
    target_include_directories(PI::PI SYSTEM INTERFACE ${PI_INCLUDE_DIRS})
    target_link_libraries(PI::PI INTERFACE
        PI::pi
        PI::pi_dummy
        PI::piall
        PI::piconvertproto
        PI::pifecpp
        PI::pifegeneric
        PI::pifeproto
        PI::pigrpcserver
        PI::pip4info
        PI::piprotobuf
        PI::piprotogrpc
    )
endif()
