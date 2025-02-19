//  Copyright (c) 2016 Christopher Hinz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// verify #2150 is fixed (Unable to use component clients as action return types)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>

#include <utility>

class foo_server : public hpx::components::component_base<foo_server>
{
};

typedef hpx::components::component<foo_server> server_type;
HPX_REGISTER_COMPONENT(server_type, foo_server)

class foo : public hpx::components::client_base<foo, foo_server>
{
public:
    typedef hpx::components::client_base<foo, foo_server> base_type;

    foo() {}
    foo(hpx::future<hpx::id_type>&& id) : base_type(std::move(id)) {}
};

foo get_foo()
{
    return hpx::new_<foo_server>(hpx::find_here());
}

HPX_PLAIN_ACTION(get_foo, get_foo_action)

hpx::future<hpx::id_type> get_future_id()
{
    return hpx::new_<foo_server>(hpx::find_here());
}

HPX_PLAIN_ACTION(get_future_id, get_future_id_action)

int hpx_main()
{
    {
        auto result1 = get_foo_action()(hpx::find_here());
        (void) result1;
        auto result2 = get_future_id_action()(hpx::find_here());
        (void) result2;

        auto result3 = hpx::async<get_foo_action>(hpx::find_here()).get();
        (void) result3;
        auto result4 = hpx::async<get_future_id_action>(hpx::find_here()).get();
        (void) result4;
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
#endif
