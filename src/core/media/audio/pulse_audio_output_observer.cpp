/*
 * Copyright © 2014 Canonical Ltd.
 *
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
 *              Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#include <core/media/audio/pulse_audio_output_observer.h>

#include <pulse/pulseaudio.h>

#include <cstdint>

#include <map>
#include <regex>
#include <string>

namespace audio = core::ubuntu::media::audio;

namespace
{
// We wrap calls to the pulseaudio client api into its
// own namespace and make sure that only managed types
// can be passed to calls to pulseaudio. In addition,
// we add guards to the function calls to ensure that
// they are conly called on the correct thread.
namespace pa
{
typedef std::shared_ptr<pa_threaded_mainloop> ThreadedMainLoopPtr;
ThreadedMainLoopPtr make_threaded_main_loop()
{
    return ThreadedMainLoopPtr
    {
        pa_threaded_mainloop_new(),
        [](pa_threaded_mainloop* ml)
        {
            pa_threaded_mainloop_stop(ml);
            pa_threaded_mainloop_free(ml);
        }
    };
}

void start_main_loop(ThreadedMainLoopPtr ml)
{
    pa_threaded_mainloop_start(ml.get());
}

typedef std::shared_ptr<pa_context> ContextPtr;
ContextPtr make_context(ThreadedMainLoopPtr main_loop)
{
    return ContextPtr
    {
        pa_context_new(pa_threaded_mainloop_get_api(main_loop.get()), "MediaHubPulseContext"),
        pa_context_unref
    };
}

void set_state_callback(ContextPtr ctxt, pa_context_notify_cb_t cb, void* cookie)
{
    pa_context_set_state_callback(ctxt.get(), cb, cookie);
}

void set_subscribe_callback(ContextPtr ctxt, pa_context_subscribe_cb_t cb, void* cookie)
{
    pa_context_set_subscribe_callback(ctxt.get(), cb, cookie);
}

void throw_if_not_on_main_loop(ThreadedMainLoopPtr ml)
{
    if (not pa_threaded_mainloop_in_thread(ml.get())) throw std::logic_error
    {
        "Attempted to call into a pulseaudio object from another"
        "thread than the pulseaudio mainloop thread."
    };
}

void throw_if_not_connected(ContextPtr ctxt)
{
    if (pa_context_get_state(ctxt.get()) != PA_CONTEXT_READY ) throw std::logic_error
    {
        "Attempted to issue a call against pulseaudio via a non-connected context."
    };
}

void get_server_info_async(ContextPtr ctxt, ThreadedMainLoopPtr ml, pa_server_info_cb_t cb, void* cookie)
{
    throw_if_not_on_main_loop(ml); throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_server_info(ctxt.get(), cb, cookie));
}

void subscribe_to_events(ContextPtr ctxt, ThreadedMainLoopPtr ml, pa_subscription_mask mask)
{
    throw_if_not_on_main_loop(ml); throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_subscribe(ctxt.get(), mask, nullptr, nullptr));
}

void get_index_of_sink_by_name_async(ContextPtr ctxt, ThreadedMainLoopPtr ml, const std::string& name, pa_sink_info_cb_t cb, void* cookie)
{
    throw_if_not_on_main_loop(ml); throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_sink_info_by_name(ctxt.get(), name.c_str(), cb, cookie));
}

void get_sink_info_by_index_async(ContextPtr ctxt, ThreadedMainLoopPtr ml, std::uint32_t index, pa_sink_info_cb_t cb, void* cookie)
{
    throw_if_not_on_main_loop(ml); throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_sink_info_by_index(ctxt.get(), index, cb, cookie));
}

void connect_async(ContextPtr ctxt)
{
    pa_context_connect(ctxt.get(), nullptr, static_cast<pa_context_flags_t>(PA_CONTEXT_NOAUTOSPAWN | PA_CONTEXT_NOFAIL), nullptr);
}

bool is_port_available_on_sink(const pa_sink_info* info, const std::regex& port_pattern)
{
    if (not info)
        return false;

    for (std::uint32_t i = 0; i < info->n_ports; i++)
    {
        if (info->ports[i]->available == PA_PORT_AVAILABLE_NO)
            continue;

        if (std::regex_match(std::string{info->ports[i]->name}, port_pattern))
            return true;
    }

    return false;
}
}
}

struct audio::PulseAudioOutputObserver::Private
{
    static void context_notification_cb(pa_context* ctxt, void* cookie)
    {
        if (auto thiz = static_cast<Private*>(cookie))
        {
            // Better safe than sorry: Check if we got signaled for the
            // context we are actually interested in.
            if (thiz->context.get() != ctxt)
                return;

            switch (pa_context_get_state(ctxt))
            {
            case PA_CONTEXT_READY:
                thiz->on_context_ready();
                break;
            case PA_CONTEXT_FAILED:
                thiz->on_context_failed();
                break;
            default:
                break;
            }
        }
    }

    static void context_subscription_cb(pa_context* ctxt, pa_subscription_event_type_t ev, uint32_t idx, void* cookie)
    {
        (void) idx;

        if (auto thiz = static_cast<Private*>(cookie))
        {
            // Better safe than sorry: Check if we got signaled for the
            // context we are actually interested in.
            if (thiz->context.get() != ctxt)
                return;

            if ((ev & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK)
                thiz->on_sink_event_with_index(idx);
        }
    }

    static void query_for_primary_sink_finished(pa_context* ctxt, const pa_sink_info* si, int eol, void* cookie)
    {
        if (eol)
            return;

        if (auto thiz = static_cast<Private*>(cookie))
        {
            // Better safe than sorry: Check if we got signaled for the
            // context we are actually interested in.
            if (thiz->context.get() != ctxt)
                return;

            thiz->on_query_for_sink_finished(si);
        }
    }

    static void query_for_server_info_finished(pa_context* ctxt, const pa_server_info* si, void* cookie)
    {
        if (not si)
            return;

        if (auto thiz = static_cast<Private*>(cookie))
        {
            // Better safe than sorry: Check if we got signaled for the
            // context we are actually interested in.
            if (thiz->context.get() != ctxt)
                return;

            thiz->on_query_for_server_info_finished(si);
        }
    }

    Private(const audio::PulseAudioOutputObserver::Configuration& config)
        : config(config),
          main_loop{pa::make_threaded_main_loop()},
          context{pa::make_context(main_loop)}
    {
        for (const auto& pattern : config.output_port_patterns)
        {
            outputs.emplace_back(pattern, core::Property<media::audio::OutputState>{media::audio::OutputState::disconnected});
            std::get<1>(outputs.back()) | properties.external_output_state;
        }

        pa::set_state_callback(context, Private::context_notification_cb, this);
        pa::set_subscribe_callback(context, Private::context_subscription_cb, this);

        pa::connect_async(context);
        pa::start_main_loop(main_loop);
    }

    // The connection attempt has been successful and we are connected
    // to pulseaudio now.
    void on_context_ready()
    {
        config.reporter->connected_to_pulse_audio();

        pa::subscribe_to_events(context, main_loop, PA_SUBSCRIPTION_MASK_SINK);

        if (config.sink == "query.from.server")
        {
            pa::get_server_info_async(context, main_loop, Private::query_for_server_info_finished, this);
        }
        else
        {
            properties.sink = config.sink;
            pa::get_index_of_sink_by_name_async(context, main_loop, config.sink, Private::query_for_primary_sink_finished, this);
        }
    }

    // Either a connection attempt failed, or an existing connection
    // was unexpectedly terminated.
    void on_context_failed()
    {
        pa::connect_async(context);
    }

    // Something changed on the sink with index idx.
    void on_sink_event_with_index(std::int32_t index)
    {
        config.reporter->sink_event_with_index(index);

        if (index != sink_index)
            return;

        pa::get_sink_info_by_index_async(context, main_loop, index, Private::query_for_primary_sink_finished, this);
    }

    // Query for primary sink finished.
    void on_query_for_sink_finished(const pa_sink_info* info)
    {
        for (auto& element : outputs)
        {
            std::get<1>(element) = pa::is_port_available_on_sink(info, std::get<0>(element))
                    ? media::audio::OutputState::connected
                    : media::audio::OutputState::disconnected;
        }

        std::set<std::tuple<bool, std::string>> known_ports;
        for (std::uint32_t i = 0; i < info->n_ports; i++)
        {
            bool is_monitored = false;

            for (auto& element : outputs)
                is_monitored = std::regex_match(info->ports[i]->name, std::get<0>(element));

            known_ports.insert(std::make_tuple(is_monitored, std::string{info->ports[i]->name}));
        }

        properties.known_ports = known_ports;

        sink_index = info->index;

        config.reporter->query_for_sink_info_finished(info->name, info->index, known_ports);
    }

    void on_query_for_server_info_finished(const pa_server_info* info)
    {
        // We bail out if we could not determine the default sink name.
        // In this case, we are not able to carry out audio output observation.
        if (not info->default_sink_name)
        {
            config.reporter->query_for_default_sink_failed();
            return;
        }

        config.reporter->query_for_default_sink_finished(info->default_sink_name);

        properties.sink = config.sink = info->default_sink_name;
        pa::get_index_of_sink_by_name_async(context, main_loop, config.sink, Private::query_for_primary_sink_finished, this);
    }

    PulseAudioOutputObserver::Configuration config;    
    pa::ThreadedMainLoopPtr main_loop;
    pa::ContextPtr context;
    std::uint32_t sink_index;
    std::vector<std::tuple<std::regex, core::Property<media::audio::OutputState>>> outputs;

    struct
    {
        core::Property<std::string> sink;
        core::Property<std::set<std::tuple<bool, std::string>>> known_ports;
        core::Property<audio::OutputState> external_output_state{audio::OutputState::disconnected};
    } properties;
};

audio::PulseAudioOutputObserver::Reporter::~Reporter()
{
}

void audio::PulseAudioOutputObserver::Reporter::connected_to_pulse_audio()
{
}

void audio::PulseAudioOutputObserver::Reporter::query_for_default_sink_failed()
{
}

void audio::PulseAudioOutputObserver::Reporter::query_for_default_sink_finished(const std::string&)
{
}

void audio::PulseAudioOutputObserver::Reporter::query_for_sink_info_finished(const std::string&, std::uint32_t, const std::set<std::tuple<bool, std::string>>&)
{
}

void audio::PulseAudioOutputObserver::Reporter::sink_event_with_index(std::uint32_t)
{
}

// Constructs a new instance, or throws std::runtime_error
// if connection to pulseaudio fails.
audio::PulseAudioOutputObserver::PulseAudioOutputObserver(const Configuration& config) : d{new Private{config}}
{
    if (not d->config.reporter) throw std::runtime_error
    {
        "PulseAudioOutputObserver: Cannot construct for invalid reporter instance."
    };
}

// We provide the name of the sink we are connecting to as a
// getable/observable property. This is specifically meant for
// consumption by test code.
const core::Property<std::string>& audio::PulseAudioOutputObserver::sink() const
{
    return d->properties.sink;
}

// The set of ports that have been identified on the configured sink.
// Specifically meant for consumption by test code.
const core::Property<std::set<std::tuple<bool, std::string>>>& audio::PulseAudioOutputObserver::known_ports() const
{
    return d->properties.known_ports;
}

// Getable/observable property holding the state of external outputs.
const core::Property<audio::OutputState>& audio::PulseAudioOutputObserver::external_output_state() const
{
    return d->properties.external_output_state;
}
