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
    Response compile_code(void *context, const char *code, const char *type, const char *name);
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

    m.def("compile", [](const std::string& script, const std::string& type, const std::string& name) {
        void* ctx = get_context();
        Response response = compile_code(ctx, script.c_str(), type.c_str(), name.c_str());
        delete_context(ctx);
        return response;
    }, py::arg("script"), py::arg("type") = "weapon", py::arg("name") = "npc",
    R"pbdoc(
    Compile GS2 script to bytecode.

    Args:
        script: GS2 source code
        type: Script type (weapon/npc/default)
        name: Script name

    Returns:
        CompilerResponse with Success flag and ByteCode bytes
    )pbdoc");

    m.def("compile_script", [](const std::string& script) {
        void* ctx = get_context();
        Response response = compile_code(ctx, script.c_str(), "weapon", "npc");
        delete_context(ctx);
        return response;
    }, py::arg("script"),
    R"pbdoc(
    Simple GS2 compilation shortcut.

    Args:
        script: GS2 source code

    Returns:
        CompilerResponse with Success flag and ByteCode bytes
    )pbdoc");
}
