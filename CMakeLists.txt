# Copyright (C) 2013 Daniel Pfeifer <daniel@pfeifer-mail.de>
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at
#   http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(BDepend CXX)

find_package(Boost REQUIRED
  filesystem
  system
  regex
  )

include_directories(${Boost_INCLUDE_DIRS})

add_executable(boost_depend
  depend.cpp
  )

target_link_libraries(boost_depend
  ${Boost_LIBRARIES}
  )
