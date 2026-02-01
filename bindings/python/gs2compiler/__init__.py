"""GS2 Script Compiler

This module provides functionality to compile Graal Script 2 (GS2) source code
into bytecode that can be executed by Graal servers.

Example usage:
    from gs2compiler import compile

    bytecode = compile_script("player.chat = \"Hello\";")
    if bytecode.Success:
        with open("script.gs2bc", "wb") as f:
            f.write(bytecode.ByteCode)
"""

try:
    from .gs2compiler import compile, compile_script, CompilerResponse
except ImportError:
    raise ImportError(
        "GS2 C++ extension not found. Make sure you've installed gs2compiler correctly."
    )

__all__ = ["compile", "compile_script", "CompilerResponse"]
__version__ = "1.1.0"
