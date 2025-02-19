//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// The purpose of this example is to show how you can access the command line
// arguments from inside a startup function of a shared library which is loaded
// for a HPX component.
//
// The startup function is called as a HPX-thread during runtime startup. It
// is guaranteed to be executed before hpx_main() is invoked.
//
// The HPX API function retrieve_commandline_arguments() demonstrated below
// expects 2 arguments: a) an options_description object describing any
// special options to be recognized by this component and b) a variables_map
// object which on return will be initialized with all found command line
// options and arguments passed while invoking the application on the locality
// where this component got loaded.

// The actual component of this module (see the subdirectory /server) does not
// expose any useful functionality. It has been added for the sole purpose of
// turning this module into a HPX component, as otherwise the startup function
// would not be executed by the runtime system.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality
HPX_REGISTER_COMPONENT_MODULE()     // create entry point for component factory

///////////////////////////////////////////////////////////////////////////////
namespace startup_shutdown
{
    // This function will be registered as a startup function for HPX below.
    //
    // That means it will be executed in a HPX-thread before hpx_main, but
    // after the runtime has been initialized and started.
    void startup()
    {
        using hpx::program_options::options_description;
        using hpx::program_options::variables_map;

        options_description desc_commandline("startup_shutdown_component");
        desc_commandline.add_options()
            ("additional",
             "this is an additional option for the startup_shutdown_compnent")
            ;

        // Retrieve command line using the Boost.ProgramOptions library.
        variables_map vm;
        if (!hpx::util::retrieve_commandline_arguments(desc_commandline, vm)) {
            HPX_THROW_EXCEPTION(hpx::not_implemented,
                "startup_shutdown::startup",
                "Failed to handle command line options");
        }

        if (vm.count("additional")) {
            std::cout << "Found additional option on command line!"
                      << std::endl;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool get_startup(hpx::startup_function_type& startup_func, bool& pre_startup)
    {
        startup_func = startup;     // return our startup-function
        pre_startup = true;         // run 'startup' as pre-startup function
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Register the startup function which will be called as a HPX-thread during
// runtime startup.
//
// Note that this macro can be used not more than once in one module. Either
// of the 2 parameters for the macro below can be zero (0), which means no
// function will be called. We leave the shutdown function out as we don't need
// it for the purpose of this example.
HPX_REGISTER_STARTUP_MODULE(::startup_shutdown::get_startup)

#endif
