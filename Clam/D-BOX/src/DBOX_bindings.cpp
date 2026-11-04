//
// Created by gabri on 6/22/2026.
// Extended 2026-11-03: ports, discovery and config are now exposed, so Python
// can actually open a port and talk D-BOX (previously only the packet types
// and DBOXprotocol were bound, and the module failed to import because it
// referenced the unimplemented DBOXprotocol::connect()).
//
// Python usage (module name: DBOXpy):
//
//   import DBOXpy as d
//   cfg   = d.DconfigUart("Urchin", "", 115200, 20)
//   disco = d.UartDiscovery(True, cfg)
//   port  = d.LinuxSerialPort(disco)          # Linux / Raspberry Pi / WSL
//   proto = d.DBOXprotocol(port, "/dev/null")
//   if disco.discover():                       # finds the ESP32, leaves port open
//       proto.send(d.DBOX(0, "N", "Info"))
//       reply = d.DBOX()
//       if proto.receive(reply, 2000):
//           print(reply.stream, reply.text)
//
// Objects passed into constructors (config -> discovery -> port -> protocol)
// are kept alive by the objects that use them. Calls that can wait on the
// port (send, receive, discover, ...) release the GIL, so other Python
// threads keep running while they wait.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <DBOXprotocol.h>
#include <Transport-Interface/UART/Fake/FakeSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>
#include <Transport-Interface/UART/Fake/FakeSeralHost.h>
#include <Discovery/UartDiscovery.h>
#include <Discovery/Discovery.h>
#include <Dconfig/DconfigUart.h>
#ifdef __linux__
#include <Transport-Interface/UART/Linux/linux-sys-seral.h>
#endif
#include <pybind11/stl.h>
#include <memory>
#include <vector>
#include <span>
#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>


namespace py = pybind11;
using namespace DBOXProtocol;

namespace {
    // Streams are single characters ('N', 'E', 'S', 'D'); accept "N" or 78.
    uint8_t streamFrom(py::handle h) {
        if (py::isinstance<py::str>(h)) {
            std::string s = h.cast<std::string>();
            if (s.size() != 1) throw std::invalid_argument("stream must be a single character");
            return static_cast<uint8_t>(s[0]);
        }
        return h.cast<uint8_t>();
    }

    py::bytes toBytes(const std::vector<uint8_t>& v) {
        return py::bytes(reinterpret_cast<const char*>(v.data()), v.size());
    }

    using Release = py::call_guard<py::gil_scoped_release>;
}

PYBIND11_MODULE(DBOXpy, m) {
    m.doc() = "Python bindings for DBOXProtocol";



    // DBOX-protocal.h file

    // DBOX struct
    py::class_<DBOX>(m, "DBOX")
        .def(
            py::init<uint8_t, uint8_t, std::string_view>(),
            py::arg("vpid"),
            py::arg("stream"),
            py::arg("text")
        )

        // DBOX(0, "N", "Spin 200") -- stream as a one-character string
        .def(
            py::init([](uint8_t vpid, py::str stream, std::string_view text) {
                return DBOX(vpid, streamFrom(stream), text);
            }),
            py::arg("vpid"),
            py::arg("stream"),
            py::arg("text")
        )

        .def(py::init<>())

        .def_readwrite(
            "VPID",
            &DBOX::VPID
        )

        .def_readwrite(
            "Stream",
            &DBOX::Stream
        )

        .def_readwrite(
            "Payload",
            &DBOX::Payload
        )

        // Convenience views
        .def_property(
            "stream",
            [](const DBOX& p) { return std::string(1, static_cast<char>(p.Stream)); },
            [](DBOX& p, py::object s) { p.Stream = streamFrom(s); }
        )

        .def_property(
            "text",
            [](const DBOX& p) {
                return py::str(reinterpret_cast<const char*>(p.Payload.data()), p.Payload.size());
            },
            [](DBOX& p, std::string_view t) { p.Payload.assign(t.begin(), t.end()); }
        )

        .def_property(
            "payload",
            [](const DBOX& p) { return toBytes(p.Payload); },
            [](DBOX& p, py::bytes b) {
                std::string s = b;
                p.Payload.assign(s.begin(), s.end());
            }
        )

        .def(
            "__eq__",
            &DBOX::operator==
        )

        .def(
            "__repr__",
            [](const DBOX& p) {
                return "DBOX(" + std::to_string(p.VPID) + ", '" + std::string(1, (char)p.Stream) + "', " +
                       py::repr(py::bytes(reinterpret_cast<const char*>(p.Payload.data()), p.Payload.size()))
                           .cast<std::string>() + ")";
            }
        );



    // DBOXraw struct
    py::class_<DBOXraw>(m, "DBOXraw")
        .def(py::init<>())

        .def(
            py::init<const DBOX&>(),
            py::arg("packet")
        )

        .def_readwrite(
            "VPID",
            &DBOXraw::VPID
        )

        .def_readwrite(
            "Stream",
            &DBOXraw::Stream
        )

        .def_readwrite(
            "PayloadSize",
            &DBOXraw::PayloadSize
        )

        .def_readwrite(
            "ChecksumHeaderCRC",
            &DBOXraw::ChecksumHeaderCRC
        )

        .def_readwrite(
            "Payload",
            &DBOXraw::Payload
        )

        .def(
            "__bytes__",
            [](const DBOXraw& packet) {
                std::vector<uint8_t> bytes = packet;
                return py::bytes(
                    reinterpret_cast<const char*>(bytes.data()),
                    bytes.size()
                );
            }
        );


    // --------------------------------------------------------------
    // Config + discovery
    // --------------------------------------------------------------

    py::class_<Dconfig>(m, "Dconfig")
        .def_readwrite("identity", &Dconfig::identity);

    py::class_<DconfigUart, Dconfig>(m, "DconfigUart")
        .def(
            py::init<std::string, std::string, uint32_t, uint32_t>(),
            py::arg("identity"),
            py::arg("port"),
            py::arg("baud"),
            py::arg("timeout")
        )
        .def_readwrite("port", &DconfigUart::Port)
        .def_readwrite("baud", &DconfigUart::baud)
        .def_readwrite("timeout", &DconfigUart::timeout);

    py::class_<UartDiscovery>(m, "UartDiscovery")
        .def(
            py::init<bool, DconfigUart*>(),
            py::arg("active"),
            py::arg("config"),
            py::keep_alive<1, 3>()           // discovery keeps its config alive
        )
        .def("discover", &UartDiscovery::discover, Release(),
             "Active: try every port, keep the one that answers (1 found / 0 not). "
             "Passive: wait on config.port until the identity arrives, then answer.")
        .def("active_discover_single", &UartDiscovery::ActiveDiscoverSingle, Release(),
             py::arg("port"), py::arg("attempts") = 3,
             "Try one port: say the identity, wait for it back.")
        .def_readwrite("handshake_timeout_ms", &UartDiscovery::handshakeTimeoutMs)
        .def_readwrite("attempts_per_port", &UartDiscovery::attemptsPerPort);


    // --------------------------------------------------------------
    // Ports
    // --------------------------------------------------------------

    py::class_<IPort>(m, "IPort")
        .def("is_open", &IPort::isOpen)
        .def("close", &IPort::close, Release())
        .def("find", &IPort::find, "List the ports this backend can see.")
        .def("flush", &IPort::flush, Release())
        .def(
            "write",
            [](IPort& p, py::bytes data) {
                std::string s = data;
                py::gil_scoped_release nogil;
                return p.write(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(s.data()), s.size()), s.size());
            },
            py::arg("data"),
            "Write raw bytes (not D-BOX framed)."
        );

    py::class_<UARTSerialPort<UartDiscovery>, IPort>(m, "UARTSerialPort")
        .def("openraw", &UARTSerialPort<UartDiscovery>::openraw, Release(),
             py::arg("device"), py::arg("baud"), py::arg("timeout_ms"));

#ifdef __linux__
    py::class_<LinuxSerialPort, UARTSerialPort<UartDiscovery>>(m, "LinuxSerialPort")
        .def(py::init<>())
        .def(
            py::init<UartDiscovery*>(),
            py::arg("discovery"),
            py::keep_alive<1, 2>()           // port keeps its discovery alive
        );
#endif

    // Fake serial ports, for tests without hardware.
    py::class_<FakeSerialLink, std::shared_ptr<FakeSerialLink>>(m, "FakeSerialLink")
        .def(py::init<std::string, std::string>(), py::arg("name_a"), py::arg("name_b"));

    py::class_<FakeSeralHost> host(m, "FakeSeralHost");
    py::enum_<FakeSeralHost::Side>(host, "Side")
        .value("A", FakeSeralHost::Side::A)
        .value("B", FakeSeralHost::Side::B);
    host.def(py::init<>())
        .def("add_plug", &FakeSeralHost::AddPlug, py::arg("link"), py::arg("side"),
             py::keep_alive<1, 2>())
        .def("list", &FakeSeralHost::list);

    py::class_<FakeSerialPort, UARTSerialPort<UartDiscovery>>(m, "FakeSerialPort")
        .def(
            py::init<std::shared_ptr<FakeSerialLink>, FakeSeralHost::Side>(),
            py::arg("link"),
            py::arg("side")
        )
        .def(
            py::init<UartDiscovery*, FakeSeralHost*>(),
            py::arg("discovery"),
            py::arg("host"),
            py::keep_alive<1, 2>(),
            py::keep_alive<1, 3>()
        );


    // DBOXprotocol class
    py::class_<DBOXprotocol>(m, "DBOXprotocol")
        .def(
            py::init<IPort*, std::string_view>(),
            py::arg("port"),
            py::arg("dump_path"),
            py::keep_alive<1, 2>(),          // protocol keeps its port alive
            py::keep_alive<2, 1>()           // and the port keeps the protocol alive:
                                             // discovery reaches it via port.protocol,
                                             // so `DBOXprotocol(port, ...)` without
                                             // storing the result must still be safe
        )

        .def(
            "send",
            &DBOXprotocol::send,
            py::arg("packet"),
            Release()
        )

        .def(
            "receive",
            &DBOXprotocol::receive,
            py::arg("packet"),
            py::arg("timeout_ms") = -1,
            Release(),
            "Fill `packet` with the next valid D-BOX packet. timeout_ms < 0 waits forever; "
            "otherwise returns False if nothing arrives in time."
        )

        // "connect" is declared in DBOXprotocol.h but was never implemented;
        // binding it made the module fail to import (undefined symbol).
        // Re-add once DBOXprotocol::connect() exists.

        .def(
            "disconnect",
            &DBOXprotocol::disconnect,
            Release()
        )

        .def(
            "start",
            &DBOXprotocol::start
        )

        .def(
            "end",
            &DBOXprotocol::end
        );
}
