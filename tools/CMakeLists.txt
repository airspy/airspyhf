# Copyright 2012 Jared Boone
# Copyright 2013/2014 Benjamin Vernoux
#
# This file is part of AirSpy (based on HackRF project).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# Based heavily upon the libftdi cmake setup.

cmake_minimum_required(VERSION 3.12)
project(airspyhf-tools C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PACKAGE airspyhf-tools)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_BUILD_TYPE Release)
add_definitions(-D_FILE_OFFSET_BITS=64)

if(MSVC)
include_directories(getopt)
add_definitions(/D _CRT_SECURE_NO_WARNINGS)
else()
add_definitions(-Wall)
endif()

if(NOT libairspyhf_SOURCE_DIR)
find_package(LIBAIRSPYHF REQUIRED)
include_directories(${LIBAIRSPYHF_INCLUDE_DIR})
else()
include_directories(${libairspyhf_SOURCE_DIR}/src)
endif()

add_subdirectory(src)

########################################################################
# Create uninstall target
########################################################################

if(NOT airspyhf_all_SOURCE_DIR)
configure_file(
    ${PROJECT_SOURCE_DIR}/../cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)


add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
endif()

########################################################################
# Install udev rules
########################################################################
set(REMOTE_USER_GROUP "plugdev")
set(RULE_TEXT "ATTR{idVendor}==\"03eb\", ATTR{idProduct}==\"800c\", SYMLINK+=\"airspyhf-%k\", MODE=\"660\", GROUP=\"${REMOTE_USER_GROUP}\"")
option(USE_UACCESS_RULES "Install udev uaccess rules" OFF)
option(INSTALL_UDEV_RULES "Install udev rules for AirSpy HF+" OFF)

if (USE_UACCESS_RULES)
    string (REPLACE "MODE=\"660\", GROUP=\"${REMOTE_USER_GROUP}\"" "TAG+=\"uaccess\"" RULE_TEXT ${RULE_TEXT})
    set (INSTALL_UDEV_RULES ON)
    message (STATUS "Udev rules created for using uaccess")
else (USE_UACCESS_RULES)
    message (STATUS "Uaccess rules are not being used, set them with -DUSE_UACCESS_RULES=ON")
endif (USE_UACCESS_RULES)

file (WRITE "52-airspyhf.rules" "${RULE_TEXT}")

if (INSTALL_UDEV_RULES)

#   Create REMOTE_USER_GROUP if it doesn't already exist
    if (NOT (USE_UACCESS_RULES))
        if (TARGET ${REMOTE_USER_GROUP})
            message (STATUS "Group ${REMOTE_USER_GROUP} already exists")
        else (TARGET ${REMOTE_USER_GROUP})
            if (NOT (EXISTS "/etc/sysusers.d/${REMOTE_USER_GROUP}.conf"))
                file (WRITE "${REMOTE_USER_GROUP}.conf" "g ${REMOTE_USER_GROUP} -")
                install (
                    FILES ${REMOTE_USER_GROUP}.conf
                    DESTINATION "/etc/sysusers.d"
                    COMPONENT "udev"
                )
                install (
                    CODE "execute_process (COMMAND systemd-sysusers)"
                )
                message (STATUS "Group ${REMOTE_USER_GROUP} did not exist and will be created during the install process")
            endif (NOT (EXISTS "/etc/sysusers.d/${REMOTE_USER_GROUP}.conf"))
        endif (TARGET ${REMOTE_USER_GROUP})
    endif (NOT (USE_UACCESS_RULES))

#   Install rules file
    install (
        FILES 52-airspyhf.rules
        DESTINATION "/etc/udev/rules.d"
        COMPONENT "udev"
        )
else (INSTALL_UDEV_RULES)
    message (STATUS "Udev rules not being installed, install them with -DINSTALL_UDEV_RULES=ON")
endif (INSTALL_UDEV_RULES)
