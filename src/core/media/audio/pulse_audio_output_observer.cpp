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
#include <pulse/glib-mainloop.h>

#include "core/media/logging.h"

#include <QString>
#include <QStringList>
#include <QRegularExpression>

#include <cstdint>

#include <map>
#include <string>

namespace audio = core::ubuntu::media::audio;

namespace
{
// We wrap calls to the pulseaudio client api into its
// own namespace and make sure that only managed types
// can be passed to calls to pulseaudio.
namespace pa
{
typedef std::shared_ptr<pa_glib_mainloop> MainLoopPtr;
MainLoopPtr make_main_loop()
{
    return MainLoopPtr
    {
        pa_glib_mainloop_new(g_main_context_default()),
        [](pa_glib_mainloop *ml)
        {
            pa_glib_mainloop_free(ml);
        }
    };
}

typedef std::shared_ptr<pa_context> ContextPtr;
ContextPtr make_context(MainLoopPtr main_loop)
{
    return ContextPtr
    {
        pa_context_new(pa_glib_mainloop_get_api(main_loop.get()), "MediaHubPulseContext"),
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

void throw_if_not_connected(ContextPtr ctxt)
{
    if (pa_context_get_state(ctxt.get()) != PA_CONTEXT_READY ) throw std::logic_error
    {
        "Attempted to issue a call against pulseaudio via a non-connected context."
    };
}

void get_server_info_async(ContextPtr ctxt, pa_server_info_cb_t cb, void* cookie)
{
    throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_server_info(ctxt.get(), cb, cookie));
}

void subscribe_to_events(ContextPtr ctxt, pa_subscription_mask mask)
{
    throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_subscribe(ctxt.get(), mask, nullptr, nullptr));
}

void get_index_of_sink_by_name_async(ContextPtr ctxt, const QString &name, pa_sink_info_cb_t cb, void* cookie)
{
    throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_sink_info_by_name(ctxt.get(), qUtf8Printable(name), cb, cookie));
}

void get_sink_info_by_index_async(ContextPtr ctxt, std::int32_t index, pa_sink_info_cb_t cb, void* cookie)
{
    throw_if_not_connected(ctxt);
    pa_operation_unref(pa_context_get_sink_info_by_index(ctxt.get(), index, cb, cookie));
}

void connect_async(ContextPtr ctxt)
{
    pa_context_connect(ctxt.get(), nullptr, static_cast<pa_context_flags_t>(PA_CONTEXT_NOAUTOSPAWN | PA_CONTEXT_NOFAIL), nullptr);
}

bool is_port_available_on_sink(const pa_sink_info* info, const QRegularExpression& port_pattern)
{
    if (not info)
        return false;

    for (std::uint32_t i = 0; i < info->n_ports; i++)
    {
        if (info->ports[i]->available == PA_PORT_AVAILABLE_NO ||
            info->ports[i]->available == PA_PORT_AVAILABLE_UNKNOWN)
            continue;

        if (port_pattern.match(QString(info->ports[i]->name)).hasMatch())
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

    static void query_for_active_sink_finished(pa_context* ctxt, const pa_sink_info* si, int eol, void* cookie)
    {
        if (eol)
            return;

        if (auto thiz = static_cast<Private*>(cookie))
        {
            // Better safe than sorry: Check if we got signaled for the
            // context we are actually interested in.
            if (thiz->context.get() != ctxt)
                return;

            thiz->on_query_for_active_sink_finished(si);
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

            thiz->on_query_for_primary_sink_finished(si);
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

    Private(const QString &sink,
            const QStringList &outputPortPatterns,
            Reporter::Ptr reporter,
            PulseAudioOutputObserver *q):
        main_loop{pa::make_main_loop()},
        context{pa::make_context(main_loop)},
        primary_sink_index(-1),
        active_sink(std::make_tuple(-1, "")),
        m_requestedSink(sink),
        m_reporter(reporter),
        q(q)
    {
        q->setOutputState(audio::OutputState::Speaker);
        for (const auto& pattern : outputPortPatterns)
        {
            outputs.emplace_back(QRegularExpression(pattern),
                                 media::audio::OutputState::Speaker);
        }

        pa::set_state_callback(context, Private::context_notification_cb, this);
        pa::set_subscribe_callback(context, Private::context_subscription_cb, this);

        pa::connect_async(context);
    }

    void setOutputState(std::tuple<QRegularExpression, media::audio::OutputState> &element,
                        media::audio::OutputState state)
    {
        MH_DEBUG("Connection state for port changed to: %d", state);
        std::get<1>(element) = state;
        q->setOutputState(state);
    }

    // The connection attempt has been successful and we are connected
    // to pulseaudio now.
    void on_context_ready()
    {
        m_reporter->connected_to_pulse_audio();

        pa::subscribe_to_events(context, PA_SUBSCRIPTION_MASK_SINK);

        if (m_requestedSink == "query.from.server")
        {
            pa::get_server_info_async(context, Private::query_for_server_info_finished, this);
        }
        else
        {
            // Get primary sink index (default)
            pa::get_index_of_sink_by_name_async(context, m_requestedSink,
                                                Private::query_for_primary_sink_finished, this);
            // Update active sink (could be == default)
            pa::get_server_info_async(context, Private::query_for_server_info_finished, this);
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
        m_reporter->sink_event_with_index(index);

        // Update server info (active sink)
        pa::get_server_info_async(context, Private::query_for_server_info_finished, this);

    }

    void on_query_for_active_sink_finished(const pa_sink_info* info)
    {
        // Update active sink if a change is registered.
        if (std::get<0>(active_sink) != info->index)
        {
            std::get<0>(active_sink) = info->index;
            std::get<1>(active_sink) = info->name;
            if (info->index != static_cast<std::uint32_t>(primary_sink_index))
                for (auto& element : outputs)
                    setOutputState(element, audio::OutputState::External);
        }
    }

    // Query for primary sink finished.
    void on_query_for_primary_sink_finished(const pa_sink_info* info)
    {
        for (auto& element : outputs)
        {
            // Only issue state change if the change happened on the active index.
            if (std::get<0>(active_sink) != info->index)
                continue;

            MH_INFO("Checking if port is available -> %d",
                    pa::is_port_available_on_sink(info, std::get<0>(element)));
            const bool available = pa::is_port_available_on_sink(info, std::get<0>(element));
            if (available)
            {
                std::get<1>(element) = audio::OutputState::Earpiece;
                continue;
            }

            audio::OutputState state;
            if (info->index == static_cast<std::uint32_t>(primary_sink_index))
                state = audio::OutputState::Speaker;
            else
                state = audio::OutputState::External;

            setOutputState(element, state);
        }

        std::set<Reporter::Port> known_ports;
        for (std::uint32_t i = 0; i < info->n_ports; i++)
        {
            bool is_monitored = false;

            for (auto& element : outputs)
                is_monitored |= std::get<0>(element).match(info->ports[i]->name).hasMatch();

            known_ports.insert(Reporter::Port
            {
                info->ports[i]->name,
                info->ports[i]->description,
                info->ports[i]->available == PA_PORT_AVAILABLE_YES,
                is_monitored
            });
        }

        m_knownPorts = known_ports;

        // Initialize sink of primary index (onboard)
        if (primary_sink_index == -1) 
            primary_sink_index = info->index;

        m_reporter->query_for_sink_info_finished(info->name, info->index, known_ports);
    }

    void on_query_for_server_info_finished(const pa_server_info* info)
    {
        // We bail out if we could not determine the default sink name.
        // In this case, we are not able to carry out audio output observation.
        if (not info->default_sink_name)
        {
            m_reporter->query_for_default_sink_failed();
            return;
        }

        // Update active sink
        if (info->default_sink_name != std::get<1>(active_sink))
            pa::get_index_of_sink_by_name_async(context, info->default_sink_name, Private::query_for_active_sink_finished, this);

        // Update wired output for primary sink (onboard)
        pa::get_sink_info_by_index_async(context, primary_sink_index, Private::query_for_primary_sink_finished, this);

        if (m_sink != m_requestedSink)
        {
            m_reporter->query_for_default_sink_finished(info->default_sink_name);
            m_sink = m_requestedSink = info->default_sink_name;
            pa::get_index_of_sink_by_name_async(context, m_sink, Private::query_for_primary_sink_finished, this);
        }
    }

    pa::MainLoopPtr main_loop;
    pa::ContextPtr context;
    std::int32_t primary_sink_index;
    std::tuple<uint32_t, std::string> active_sink;
    std::vector<std::tuple<QRegularExpression, media::audio::OutputState>> outputs;

    QString m_sink;
    QString m_requestedSink;
    Reporter::Ptr m_reporter;
    std::set<audio::PulseAudioOutputObserver::Reporter::Port> m_knownPorts;
    PulseAudioOutputObserver *q;
};

bool audio::PulseAudioOutputObserver::Reporter::Port::operator==(const audio::PulseAudioOutputObserver::Reporter::Port& rhs) const
{
    return name == rhs.name;
}

bool audio::PulseAudioOutputObserver::Reporter::Port::operator<(const audio::PulseAudioOutputObserver::Reporter::Port& rhs) const
{
    return name < rhs.name;
}

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

void audio::PulseAudioOutputObserver::Reporter::query_for_sink_info_finished(const std::string&, std::uint32_t, const std::set<Port>&)
{
}

void audio::PulseAudioOutputObserver::Reporter::sink_event_with_index(std::uint32_t)
{
}

// Constructs a new instance, or throws std::runtime_error
// if connection to pulseaudio fails.
audio::PulseAudioOutputObserver::PulseAudioOutputObserver(
        const QString &sink,
        const QStringList &outputPortPatterns,
        Reporter::Ptr reporter,
        OutputObserver *q):
    OutputObserverPrivate(q),
    d(new Private(sink, outputPortPatterns, reporter, this))
{
}

QString audio::PulseAudioOutputObserver::sink() const
{
    return d->m_sink;
}

std::set<audio::PulseAudioOutputObserver::Reporter::Port>& audio::PulseAudioOutputObserver::knownPorts() const
{
    return d->m_knownPorts;
}
