//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/plain_actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "components/managed_refcnt_checker.hpp"
#include "components/simple_refcnt_checker.hpp"

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::finalize;
using hpx::find_here;
using hpx::init;

using std::chrono::milliseconds;

using hpx::naming::get_locality_id_from_id;
using hpx::naming::get_management_type_name;
using hpx::naming::id_type;
using hpx::naming::detail::get_credit_from_gid;

using hpx::components::component_type;
using hpx::components::get_component_type;

using hpx::applier::get_applier;

using hpx::agas::garbage_collect;

using hpx::async;

using hpx::test::managed_refcnt_monitor;
using hpx::test::simple_refcnt_monitor;

using hpx::util::report_errors;

using hpx::cout;
using hpx::find_here;
using hpx::flush;

void split(id_type const& from, id_type const& target, std::int64_t old_credit);

HPX_PLAIN_ACTION(split)

///////////////////////////////////////////////////////////////////////////////
// Helper functions.
inline std::int64_t get_credit(id_type const& id)
{
    return get_credit_from_gid(id.get_gid());
}

///////////////////////////////////////////////////////////////////////////////
void split(id_type const& from, id_type const& target, std::int64_t old_credit)
{
    cout << "[" << find_here() << "/" << target << "]: " << old_credit << ", "
         << get_credit(target) << ", "
         << get_management_type_name(target.get_management_type()) << "\n"
         << flush;

    // If we have more credits than the sender, then we're done.
    if (old_credit < get_credit(target))
        return;

    id_type const here = find_here();

    if (get_locality_id_from_id(from) == get_locality_id_from_id(here))
    {
        throw std::logic_error("infinite recursion detected, split was "
                               "invoked locally");
    }

    // Recursively call split on the sender locality.
    async<split_action>(from, here, target, get_credit(target)).get();

    cout << "  after split: " << get_credit(target) << "\n" << flush;
}

///////////////////////////////////////////////////////////////////////////////
template <typename Client>
void hpx_test_main(variables_map& vm)
{
    std::uint64_t const delay = vm["delay"].as<std::uint64_t>();

    typedef typename Client::server_type server_type;

    component_type ctype = get_component_type<server_type>();
    std::vector<id_type> remote_localities = hpx::find_remote_localities(ctype);

    if (remote_localities.empty())
        throw std::logic_error("this test cannot be run on one locality");

    id_type const here = find_here();

    Client monitor(here);

    {
        id_type id = monitor.detach().get();

        cout << "id: " << id << " "
             << get_management_type_name(id.get_management_type()) << "\n"
             << flush;

        async<split_action>(remote_localities[0], here, id, get_credit(id))
            .get();

        cout << "after split: " << get_credit(id) << "\n" << flush;
    }

    // Flush pending reference counting operations.
    garbage_collect();
    garbage_collect(remote_localities[0]);
    garbage_collect();
    garbage_collect(remote_localities[0]);

    // The component should be out of scope now.
    HPX_TEST_EQ(true, monitor.is_ready(milliseconds(delay)));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    {
        cout << std::string(80, '#') << "\n"
             << "simple component test\n"
             << std::string(80, '#') << "\n"
             << flush;

        hpx_test_main<simple_refcnt_monitor>(vm);

        cout << std::string(80, '#') << "\n"
             << "managed component test\n"
             << std::string(80, '#') << "\n"
             << flush;

        hpx_test_main<managed_refcnt_monitor>(vm);
    }

    finalize();
    return report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("delay", value<std::uint64_t>()->default_value(1000),
        "number of milliseconds to wait for object destruction");

    // We need to explicitly enable the test components used by this test.
    std::vector<std::string> const cfg = {
        "hpx.components.simple_refcnt_checker.enabled! = 1",
        "hpx.components.managed_refcnt_checker.enabled! = 1"};

    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
#endif
