//  osc responder class
//  Copyright (C) 2008 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#pragma once

#include <thread>

// AppleClang workaround
#if defined(__apple_build_version__) && __apple_build_version__ > 10000000
#    define BOOST_ASIO_HAS_STD_STRING_VIEW 1
#endif

// libc++ workaround
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 7000 && _LIBCPP_VERSION < 9000
#    define BOOST_ASIO_HAS_STD_STRING_VIEW 1
#endif

#include <boost/asio/ip/udp.hpp>

#include "branch_hints.hpp"

#include "nova-tt/semaphore.hpp"
#include "nova-tt/thread_priority.hpp"
#include "nova-tt/name_thread.hpp"

namespace nova {

namespace detail {
using namespace boost::asio;
using namespace boost::asio::ip;

class network_thread {
public:
    void start_receive(void) {
        thread_ = std::thread([this] {
        /* NB: on macOS we just keep the default thread priority */
#ifdef NOVA_TT_PRIORITY_RT
            thread_set_priority_rt(thread_priority_interval_rt().first);
#endif
            name_thread("Network Receive");

            sem.post();
            boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work =
                boost::asio::make_work_guard(io_context_);
            io_context_.run();
        });
        sem.wait();
    }

    ~network_thread(void) {
        if (!thread_.joinable())
            return;
        io_context_.stop();
        thread_.join();
    }

    io_context& get_io_context(void) { return io_context_; }

    void send_udp(const char* data, unsigned int size, udp::endpoint const& receiver) {
        udp::socket socket(io_context_);
        socket.open(udp::v4());
        socket.send_to(boost::asio::buffer(data, size), receiver);
    }

protected:
    io_context io_context_;

private:
    semaphore sem;
    std::thread thread_;
};


} /* namespace */

using detail::network_thread;

} /* namespace nova */
