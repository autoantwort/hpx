//  Copyright (c) 2013 Mario Mulansky
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #775: runtime error with
// local dataflow (copying futures?).

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>

#include <hpx/async_local/dataflow.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/pack_traversal/unwrap.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using hpx::unwrapping;

typedef hpx::shared_future<double> future_type;

struct mul
{
    double operator()(double x1, double x2) const
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10000));
        hpx::util::format_to(hpx::cout, "func: {}, {}\n", x1, x2) << hpx::flush;
        return x1 * x2;
    }
};

double dummy(double x, double)
{
    std::cout << "dummy: " << x << "\n";
    return x;
}

void future_swap(future_type& f1, future_type& f2)
{
    future_type tmp = f1;
    f1 = hpx::dataflow(unwrapping(&dummy), f2, f1);
    f2 = hpx::dataflow(unwrapping(&dummy), tmp, f1);
}

int main()
{
    future_type f1 = hpx::make_ready_future(2.0);
    future_type f2 = hpx::make_ready_future(3.0);

    f1 = hpx::dataflow(unwrapping(mul()), f1, f2);

    future_swap(f1, f2);

    hpx::util::format_to(hpx::cout, "f1: {}\n", f1.get()) << hpx::flush;
    hpx::util::format_to(hpx::cout, "f2: {}\n", f2.get()) << hpx::flush;

    return 0;
}
#endif
