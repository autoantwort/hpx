# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Compatibility with using Boost.FileSystem, introduced in V1.4.0
hpx_option(
  HPX_DATASTRUCTURES_WITH_ADAPT_STD_TUPLE BOOL
  "Enable compatibility of hpx::tuple with std::tuple. (default: ON)" ON
  ADVANCED
  CATEGORY "Modules"
  MODULE DATASTRUCTURES
)

if(HPX_DATASTRUCTURES_WITH_ADAPT_STD_TUPLE)
  hpx_add_config_define_namespace(
    DEFINE HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE NAMESPACE DATASTRUCTURES
  )
endif()

set(datastructures_headers
    hpx/datastructures/any.hpp
    hpx/datastructures/detail/small_vector.hpp
    hpx/datastructures/detail/variant.hpp
    hpx/datastructures/member_pack.hpp
    hpx/datastructures/optional.hpp
    hpx/datastructures/tuple.hpp
    hpx/datastructures/traits/supports_streaming_with_any.hpp
    hpx/datastructures/traits/is_tuple_like.hpp
    hpx/datastructures/variant.hpp
    hpx/datastructures/variant_helper.hpp
)

# cmake-format: off
set(datastructures_compat_headers
    hpx/datastructures.hpp => hpx/modules/datastructures.hpp
    hpx/traits/supports_streaming_with_any.hpp => hpx/modules/datastructures.hpp
    hpx/traits/is_tuple_like.hpp => hpx/modules/datastructures.hpp
    hpx/util/any.hpp => hpx/any.hpp
    hpx/util/optional.hpp => hpx/optional.hpp
    hpx/util/tuple.hpp => hpx/tuple.hpp
)
# cmake-format: on

set(datastructures_sources)

include(HPX_AddModule)
add_hpx_module(
  core datastructures
  GLOBAL_HEADER_GEN ON
  SOURCES ${datastructures_sources}
  HEADERS ${datastructures_headers}
  COMPAT_HEADERS ${datastructures_compat_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/datastructures/variant_helper.hpp"
  MODULE_DEPENDENCIES hpx_assertion hpx_config hpx_concepts hpx_type_support
  CMAKE_SUBDIRS examples tests
)
