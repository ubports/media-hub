/*
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#ifndef TESTING_WEB_SERVER_H_
#define TESTING_WEB_SERVER_H_

#include <core/posix/exit.h>
#include <core/posix/signal.h>

#include <core/testing/cross_process_sync.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <functional>
#include <thread>
#include <future>

#include "mongoose.h"

namespace testing
{
namespace web
{
namespace server
{
// Configuration options for creating a testing web server.
struct Configuration
{
    // The port to expose the web-server on.
    std::uint16_t port;
    // indicates the server must stop
    int * stop;
    // Function that is invoked for individual client requests.
    std::function<void(struct mg_connection*, struct http_message*)> request_handler;
};
}
}
// Returns an executable web-server for the given configuration.
inline std::function<core::posix::exit::Status(core::testing::CrossProcessSync& cps)> a_web_server(const web::server::Configuration& configuration)
{

    return [configuration](core::testing::CrossProcessSync& cps)
    {
        bool terminated = false;

        // Register for SIG_TERM
        auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
        // On SIG_TERM, we set terminated to false and request a clean shutdown
        // of the polling loop.
        trap->signal_raised().connect([trap, &terminated](core::posix::Signal)
        {
            trap->stop();
            terminated = true;
        });

        struct Context
        {
            static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
		auto thiz = static_cast<struct Context*>(nc->user_data);
                if (ev == MG_EV_HTTP_REQUEST) {
                   thiz->handle_request(nc, ev_data);
                }
            }

            void handle_request(mg_connection* conn, void *ev_data)
            {
                struct http_message *hm = (struct http_message *) ev_data;
		auto thiz = static_cast<struct Context*>(conn->user_data);
                thiz->configuration.request_handler(conn, hm);
            }

            const testing::web::server::Configuration& configuration;
        } context{configuration};

        std::thread trap_worker
        {
            [trap]()
            {
                trap->run();
            }
        };

        struct mg_mgr mgr;
        struct mg_connection *nc;
        const char *s_http_port = std::to_string(configuration.port).c_str();

        std::cerr << "Starting web server on port " << s_http_port << std::endl;
        mg_mgr_init(&mgr, &context);
        nc = mg_bind(&mgr, s_http_port, Context::ev_handler);
        if (nc == NULL) {
            std::cerr << "Failed to create listener" << std::endl;
            return core::posix::exit::Status::failure;
        }

        // Don't know what is going on but mongoose destroys the user_data pointer
        // so restore it. 
        // See https://github.com/cesanta/mongoose/issues/756
        // and https://github.com/cesanta/mongoose/issues/857
        nc->user_data = mgr.user_data = &context;

        // Set up HTTP server parameters
        mg_set_protocol_http_websocket(nc);

        // Notify framework that we are good to go
        cps.try_signal_ready_for(std::chrono::milliseconds{500});

        // Start the polling loop
        for (;;) {
            mg_mgr_poll(&mgr, 1000);
            if (terminated)
                break;
            if (*configuration.stop > 0)
                break;
        }
        mg_mgr_free(&mgr);

        trap->stop();
        if (trap_worker.joinable())
            trap_worker.join();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };
}
}

#endif // TESTING_WEB_SERVER_H_


