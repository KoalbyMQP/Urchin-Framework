//
// Created by gabri on 6/22/2026.
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
#include <pybind11/stl.h>
#include <vector>
#include <span>
#include <cstdint>
#include <string_view>


namespace py = pybind11;
using namespace DBOXProtocol;
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

        .def(
            "__eq__",
            &DBOX::operator==
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


    // DBOXprotocol class
    py::class_<DBOXprotocol>(m, "DBOXprotocol")
        .def(
            py::init<IPort*, std::string_view>(),
            py::arg("port"),
            py::arg("dump_path")
        )

        .def(
            "send",
            &DBOXprotocol::send,
            py::arg("packet")
        )

        .def(
            "receive",
            &DBOXprotocol::receive,
            py::arg("packet")
        )

        .def(
            "connect",
            &DBOXprotocol::connect,
            py::arg("port")
        )

        .def(
            "disconnect",
            &DBOXprotocol::disconnect
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