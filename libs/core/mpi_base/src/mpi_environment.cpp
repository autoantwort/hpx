//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//  Copyright (c)      2022 Patrick Diehl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/modules/logging.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/util.hpp>

#include <boost/tokenizer.hpp>

#include <cstddef>
#include <cstdlib>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util {

    namespace detail {

        bool detect_mpi_environment(
            util::runtime_configuration const& cfg, char const* default_env)
        {
#if defined(__bgq__)
            // If running on BG/Q, we can safely assume to always run in an
            // MPI environment
            return true;
#else
            std::string mpi_environment_strings =
                cfg.get_entry("hpx.parcel.mpi.env", default_env);

            boost::char_separator<char> sep(";,: ");
            boost::tokenizer<boost::char_separator<char>> tokens(
                mpi_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug)
                        << "Found MPI environment variable: " << tok << "="
                        << std::string(env) << ", enabling MPI support\n";
                    return true;
                }
            }

            LBT_(info) << "No known MPI environment variable found, disabling "
                          "MPI support\n";
            return false;
#endif
        }
    }    // namespace detail

    bool mpi_environment::check_mpi_environment(
        util::runtime_configuration const& cfg)
    {
#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)
        // We disable the MPI parcelport if any of these hold:
        //
        // - The parcelport is explicitly disabled
        // - The application is not run in an MPI environment
        // - The TCP parcelport is enabled and has higher priority
        if (get_entry_as(cfg, "hpx.parcel.mpi.enable", 1) == 0 ||
            (get_entry_as(cfg, "hpx.parcel.tcp.enable", 1) &&
                (get_entry_as(cfg, "hpx.parcel.tcp.priority", 1) >
                    get_entry_as(cfg, "hpx.parcel.mpi.priority", 0))))
        {
            LBT_(info) << "MPI support disabled via configuration settings\n";
            return false;
        }

        if (!detail::detect_mpi_environment(cfg, HPX_HAVE_PARCELPORT_MPI_ENV))
        {
            // log message was already generated
            return false;
        }

        return true;
#elif defined(HPX_HAVE_MODULE_MPI_BASE)
        // if MPI futures are enabled while networking is off we need to
        // check whether we were run using mpirun
        return detail::detect_mpi_environment(cfg, HPX_HAVE_PARCELPORT_MPI_ENV);
#else
        return false;
#endif
    }
}}    // namespace hpx::util

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)) ||      \
    defined(HPX_HAVE_MODULE_MPI_BASE)

namespace hpx { namespace util {

    mpi_environment::mutex_type mpi_environment::mtx_;
    bool mpi_environment::enabled_ = false;
    bool mpi_environment::has_called_init_ = false;
    int mpi_environment::provided_threading_flag_ = MPI_THREAD_SINGLE;
    MPI_Comm mpi_environment::communicator_ = MPI_COMM_NULL;

    int mpi_environment::is_initialized_ = -1;

    ///////////////////////////////////////////////////////////////////////////
    int mpi_environment::init(
        int*, char***, const int minimal, const int required, int& provided)
    {
        has_called_init_ = false;

        // Check if MPI_Init has been called previously
        int is_initialized = 0;
        int retval = MPI_Initialized(&is_initialized);
        if (MPI_SUCCESS != retval)
        {
            return retval;
        }
        if (!is_initialized)
        {
            retval = MPI_Init_thread(nullptr, nullptr, required, &provided);
            if (MPI_SUCCESS != retval)
            {
                return retval;
            }

            if (provided < minimal)
            {
                HPX_THROW_EXCEPTION(invalid_status,
                    "hpx::util::mpi_environment::init",
                    "MPI doesn't provide minimal requested thread level");
            }
            has_called_init_ = true;
        }
        return retval;
    }

    ///////////////////////////////////////////////////////////////////////////
    void mpi_environment::init(
        int* argc, char*** argv, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        int this_rank = -1;
        has_called_init_ = false;

        // We assume to use the MPI parcelport if it is not explicitly disabled
        enabled_ = check_mpi_environment(rtcfg);
        if (!enabled_)
        {
            rtcfg.add_entry("hpx.parcel.mpi.enable", "0");
            return;
        }

        rtcfg.add_entry("hpx.parcel.bootstrap", "mpi");

        int required = MPI_THREAD_SINGLE;
#if defined(HPX_HAVE_PARCELPORT_MPI_MULTITHREADED)
        required =
            (get_entry_as(rtcfg, "hpx.parcel.mpi.multithreaded", 1) != 0) ?
            MPI_THREAD_MULTIPLE :
            MPI_THREAD_SINGLE;

#if defined(MVAPICH2_VERSION) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MVAPICH2 if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MV2_ENABLE_AFFINITY", "0", 1);
#endif

#if defined(MPICH) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MPICH if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MPICH_MAX_THREAD_SAFETY", "multiple", 1);
#endif

#endif

        int retval =
            init(argc, argv, required, required, provided_threading_flag_);
        if (MPI_SUCCESS != retval && MPI_ERR_OTHER != retval)
        {
            // explicitly disable mpi if not run by mpirun
            rtcfg.add_entry("hpx.parcel.mpi.enable", "0");

            enabled_ = false;

            int msglen = 0;
            char message[MPI_MAX_ERROR_STRING + 1];
            MPI_Error_string(retval, message, &msglen);
            message[msglen] = '\0';

            std::string msg("mpi_environment::init: MPI_Init_thread failed: ");
            msg = msg + message + ".";
            throw std::runtime_error(msg.c_str());
        }

        MPI_Comm_dup(MPI_COMM_WORLD, &communicator_);

        if (provided_threading_flag_ < MPI_THREAD_SERIALIZED)
        {
            // explicitly disable mpi if not run by mpirun
            rtcfg.add_entry("hpx.parcel.mpi.multithreaded", "0");
        }

        if (provided_threading_flag_ == MPI_THREAD_FUNNELED)
        {
            enabled_ = false;
            has_called_init_ = false;
            throw std::runtime_error(
                "mpi_environment::init: MPI_Init_thread: "
                "The underlying MPI implementation only supports "
                "MPI_THREAD_FUNNELED. This mode is not supported by HPX. "
                "Please pass -Ihpx.parcel.mpi.multithreaded=0 to explicitly "
                "disable MPI multi-threading.");
        }

        this_rank = rank();

#if defined(HPX_HAVE_NETWORKING)
        if (this_rank == 0)
        {
            rtcfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = hpx::runtime_mode::worker;
        }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
        rtcfg.mode_ = hpx::runtime_mode::console;
#else
        rtcfg.mode_ = hpx::runtime_mode::local;
#endif

        rtcfg.add_entry("hpx.parcel.mpi.rank", std::to_string(this_rank));
        rtcfg.add_entry("hpx.parcel.mpi.processorname", get_processor_name());
    }

    std::string mpi_environment::get_processor_name()
    {
        char name[MPI_MAX_PROCESSOR_NAME + 1] = {'\0'};
        int len = 0;
        MPI_Get_processor_name(name, &len);

        return name;
    }

    void mpi_environment::finalize()
    {
        if (enabled() && has_called_init())
        {
            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (!is_finalized)
            {
                MPI_Finalize();
            }
        }
    }

    bool mpi_environment::enabled()
    {
        return enabled_;
    }

    bool mpi_environment::multi_threaded()
    {
        return provided_threading_flag_ >= MPI_THREAD_SERIALIZED;
    }

    bool mpi_environment::has_called_init()
    {
        return has_called_init_;
    }

    int mpi_environment::size()
    {
        int res(-1);
        if (enabled())
            MPI_Comm_size(communicator(), &res);
        return res;
    }

    int mpi_environment::rank()
    {
        int res(-1);
        if (enabled())
            MPI_Comm_rank(communicator(), &res);
        return res;
    }

    MPI_Comm& mpi_environment::communicator()
    {
        return communicator_;
    }

    mpi_environment::scoped_lock::scoped_lock()
    {
        if (!multi_threaded())
            mtx_.lock();
    }

    mpi_environment::scoped_lock::~scoped_lock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    void mpi_environment::scoped_lock::unlock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    mpi_environment::scoped_try_lock::scoped_try_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            locked = mtx_.try_lock();
        }
    }

    mpi_environment::scoped_try_lock::~scoped_try_lock()
    {
        if (!multi_threaded() && locked)
            mtx_.unlock();
    }

    void mpi_environment::scoped_try_lock::unlock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }
}}    // namespace hpx::util

#endif
