#include <pybind11/pybind11.h>
#include <string>
#include <vector>

extern "C" {
    struct Response {
        bool Success;
        const char *ErrMsg;
        unsigned char *ByteCode;
        uint32_t ByteCodeSize;
    };

    void *get_context();
    Response compile_code(void *context, const char *code, const char *type, const char *name, bool with_header);
    void delete_context(void *context);
}

namespace py = pybind11;

PYBIND11_MODULE(gs2compiler, m) {
    m.doc() = "GS2 Script Compiler - Compiles Graal Script 2 to bytecode";

    py::class_<Response>(m, "CompilerResponse")
        .def(py::init<>())
        .def_readwrite("Success", &Response::Success)
        .def_readwrite("ErrMsg", &Response::ErrMsg)
        .def_property_readonly("ByteCode",
            [](const Response& self) {
                return py::bytes(reinterpret_cast<const char*>(self.ByteCode), self.ByteCodeSize);
            }
        );

    m.def("compile", [](const std::string& script, const std::string& type, const std::string& name, bool with_header) {
        void* ctx = get_context();
        Response response = compile_code(ctx, script.c_str(), type.c_str(), name.c_str(), with_header);
        delete_context(ctx);
        return response;
    }, py::arg("script"), py::arg("type") = "weapon", py::arg("name") = "npc", py::arg("with_header") = false,
    R"pbdoc(
    Compile GS2 script to bytecode.

    Args:
        script: GS2 source code
        type: Script type (weapon/npc/default)
        name: Script name
        with_header: Include Graal file header (default: false, use true for .gs2bc files)

    Returns:
        CompilerResponse with Success flag and ByteCode bytes
    )pbdoc");

    m.def("compile_script", [](const std::string& script) {
        void* ctx = get_context();
        Response response = compile_code(ctx, script.c_str(), "weapon", "npc", false);
        delete_context(ctx);
        return response;
    }, py::arg("script"),
    R"pbdoc(
    Simple GS2 compilation shortcut (raw bytecode, no header).

    Args:
        script: GS2 source code

    Returns:
        CompilerResponse with Success flag and ByteCode bytes
    )pbdoc");
}
