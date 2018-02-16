cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR) #CPACK_DEBIAN_<COMPONENT>_PACKAGE_NAME

find_package(IRODS 4.2.2 EXACT REQUIRED)

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
  set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
  message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'. This is the correct setting for normal builds.")
endif()

if (NOT AZURE_EXTERNALS_LIBDIR)
  set(AZURE_EXTERNALS_LIBDIR "/usr/local/lib" CACHE STRING "Choose the location of the Azure libraries." FORCE)
  message(STATUS "Setting unspecified AZURE_EXTERNALS_LIBDIR to '${AZURE_EXTERNALS_LIBDIR}'.")
endif()

if (NOT CRYPTO_LIBDIR)
  set(CRYPTO_LIBDIR "/usr/lib/x86_64-linux-gnu" CACHE STRING "Choose the location of the crypto libraries." FORCE)
  message(STATUS "Setting unspecified CRYPTO_LIBDIR to '${CRYPTO_LIBDIR}'.")
endif()

 message(STATUS "CRYPTO_LIBDIR is '${CRYPTO_LIBDIR}'.")

#set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
project(irods-azure-wrapper C CXX)
  message("-- Setting gcc options")

  set(WARNINGS "-Wall -Wextra -Wunused-parameter -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winit-self -Winvalid-pch -Wmissing-format-attribute -Wmissing-include-dirs -Wpacked -Wredundant-decls -Wunreachable-code")
  set(LINUX_SUPPRESSIONS "-Wno-deprecated -Wno-unknown-pragmas -Wno-unused-function -Wno-reorder -Wno-char-subscripts -Wno-switch -Wno-unused-but-set-parameter -Wno-unused-value -Wno-unused-local-typedefs -Wno-unused-parameter")
  set(WARNINGS "${WARNINGS} ${LINUX_SUPPRESSIONS}")

  set(LD_FLAGS "${LD_FLAGS} -Wl,-z,defs")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-strict-aliasing")

  set(STRICT_CXX_FLAGS ${WARNINGS} "-Werror -pedantic")

include(${IRODS_TARGETS_PATH})

set(IRODS_PLUGIN_VERSION_MAJOR "2")
set(IRODS_PLUGIN_VERSION_MINOR "2")
set(IRODS_PLUGIN_VERSION_PATCH "0")
set(IRODS_PLUGIN_VERSION "${IRODS_PLUGIN_VERSION_MAJOR}.${IRODS_PLUGIN_VERSION_MINOR}.${IRODS_PLUGIN_VERSION_PATCH}")

if (NOT CPACK_PACKAGING_INSTALL_PREFIX)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/" CACHE STRING "Package root path. \"/\" is correct for normal package builds.." FORCE)
  message(STATUS "Setting unspecified CPACK_PACKAGING_INSTALL_PREFIX to '${CPACK_PACKAGING_INSTALL_PREFIX}'. This is the correct setting for normal builds.")
endif()

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
  message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'")
endif()

#set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++ -Wl,-z,defs")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -Wl,-z,defs")
add_compile_options(-nostdinc++ -Wall -Wextra -Werror)
add_compile_options(-Wno-unused-parameter) # Until fix of irods_re_plugin.hpp
add_compile_options(-Wno-unneeded-internal-declaration) # Until fix of https://github.com/irods/irods/issues/3396
#link_libraries(c++abi
#               ${IRODS_EXTERNALS_PACKAGE_ROOT}/jansson2.7-0/lib/libjansson.so)
#include_directories(${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
#                    ${IRODS_EXTERNALS_PACKAGE_ROOT}/jansson2.7-0/include)
include_directories(SYSTEM /usr/include/c++/4.8.5 /usr/include/c++/4.8.5/x86_64-redhat-linux)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

find_package(OpenSSL REQUIRED)

add_library(
  irods_azure_plugin_wrapper
  MODULE
  ${CMAKE_SOURCE_DIR}/azurewrapper/libazurewrapper.cpp
  )
target_include_directories(
  irods_azure_plugin_wrapper
  PRIVATE
  ${BOOST_ROOT}/include
  )
target_link_libraries(
  irods_azure_plugin_wrapper
  PRIVATE
  ${AZURE_EXTERNALS_LIBDIR}/libazurestorage.so
  ${AZURE_EXTERNALS_LIBDIR}/libcpprest.so
  ${OPENSSL_CRYPTO_LIBRARY}
  )
target_compile_definitions(irods_azure_plugin_wrapper PRIVATE)
#set_property(TARGET irods_azure_plugin_wrapper PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
set_property(TARGET irods_azure_plugin_wrapper PROPERTY OUTPUT_NAME azure_wrapper)
install(
  TARGETS
  irods_azure_plugin_wrapper
  LIBRARY
  DESTINATION ${IRODS_PLUGINS_DIRECTORY}/resources
  )

install(
  FILES
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  )


if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "ubuntu" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "debian")
  if (NOT IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME)
    execute_process(
      COMMAND "lsb_release" "-s" "-c"
      RESULT_VARIABLE IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME
      OUTPUT_VARIABLE IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME
      )
    string(STRIP ${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME} IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME)
    if (NOT ${IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME} STREQUAL "0")
      message(FATAL_ERROR "Linux lsb_release shortname detection failed\n${IRODS_PLUGIN_EXECUTE_PROCESS_RESULT_LINUX_DISTRIBUTION_VERSION_CODENAME}")
    endif()
    set(IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME ${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME} CACHE STRING "Linux distribution version codename, e.g. {precise, wheezy, trusty, jessie, ...}." FORCE)
    message(STATUS "Setting unspecified IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME to '${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME}'")
  endif()
endif()

if (NOT CPACK_GENERATOR)
  set(CPACK_GENERATOR ${IRODS_CPACK_GENERATOR} CACHE STRING "CPack generator to use, e.g. {DEB, RPM, TGZ}." FORCE)
  message(STATUS "Setting unspecified CPACK_GENERATOR to ${CPACK_GENERATOR}. This is the correct setting for normal builds.")
endif()

if (CPACK_GENERATOR STREQUAL "RPM")
  set(IRODS_PLUGIN_PACKAGE_FILE_NAME_SUFFIX "-${IRODS_PLUGIN_VERSION}-1.${CMAKE_SYSTEM_PROCESSOR}")
elseif(CPACK_GENERATOR STREQUAL "DEB")
  if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR "amd64")
  endif()
  set(IRODS_PLUGIN_PACKAGE_FILE_NAME_SUFFIX "_${IRODS_PLUGIN_VERSION}~${IRODS_PLUGIN_LINUX_DISTRIBUTION_VERSION_CODENAME}_${CMAKE_SYSTEM_PROCESSOR}")
else()
  set(IRODS_PLUGIN_PACKAGE_FILE_NAME_SUFFIX "-${IRODS_PLUGIN_VERSION}-${IRODS_LINUX_DISTRIBUTION_NAME}${IRODS_LINUX_DISTRIBUTION_VERSION_MAJOR}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_PACKAGE_FILE_NAME "irods-resource-plugin-azure-wrapper${IRODS_PLUGIN_PACKAGE_FILE_NAME_SUFFIX}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_PACKAGE_VERSION ${IRODS_PLUGIN_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${IRODS_PLUGIN_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${IRODS_PLUGIN_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${IRODS_PLUGIN_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Renaissance Computing Institute <info@irods.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The integrated Rule-Oriented Data System")
set(CPACK_PACKAGE_VENDOR "Renaissance Computing Institute <info@irods.org>")

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_DEBIAN_PACKAGE_SECTION "contrib/science")
set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://irods.org")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/postinst;")

set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_VENDOR "iRODS Consortium")
set(CPACK_RPM_PACKAGE_URL "https://irods.org")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOPROV 0)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/postinst")

set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

set(CPACK_DEBIAN_PACKAGE_NAME "irods-resource-plugin-azure-wrapper")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime (= ${IRODS_CPACK_DEBIAN_PACKAGE_VERSION}), libc6")

set(CPACK_RPM_PACKAGE_NAME "irods-resource-plugin-azure")
if (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos" OR IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "centos linux")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}")
elseif (IRODS_LINUX_DISTRIBUTION_NAME STREQUAL "opensuse")
  set(CPACK_RPM_PACKAGE_REQUIRES "${IRODS_PACKAGE_DEPENDENCIES_STRING}, irods-runtime = ${IRODS_VERSION}, libopenssl1_0_0")
endif()

include(CPack)
