//  Copyright (c) 2007-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/server/component_base.hpp>
#include <hpx/functional/function.hpp>
#include <hpx/performance_counters/server/base_performance_counter.hpp>

#include <cstdint>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters { namespace server {

    class HPX_EXPORT raw_values_counter
      : public base_performance_counter
      , public components::component_base<raw_values_counter>
    {
        using base_type = components::component_base<raw_values_counter>;

    public:
        using type_holder = raw_values_counter;
        using base_type_holder = base_performance_counter;

        raw_values_counter();

        raw_values_counter(counter_info const& info,
            hpx::util::function_nonser<std::vector<std::int64_t>(bool)> f);

        hpx::performance_counters::counter_values_array
        get_counter_values_array(bool reset = false) override;

        void reset_counter_value() override;

        // finalize() will be called just before the instance gets destructed
        void finalize();

        naming::address get_current_address() const;

    private:
        hpx::util::function_nonser<std::vector<std::int64_t>(bool)> f_;
        bool reset_;
    };
}}}    // namespace hpx::performance_counters::server
