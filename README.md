# REDasm Analyzers
This repository hosts the post-disassembly static analysis and metadata recovery plugins for **[REDasm Core Engine](https://github.com/redasm-dev/core)**.

These modules run after the initial disassembly phase to automatically recover high-level structures, objects, and symbol metadata from stripped or legacy binaries (they may trigger main analysis again).

## Included Modules
*   **Visual Basic (`compiler_vb`)**: Recovery of VB5/VB6 project structures, object tables, and event maps from legacy binaries.
*   **MSVC RTTI (`compiler_msvc_rtti`)**: Decodes Microsoft Visual C++ Run-Time Type Information to rebuild vtables and class hierarchies.
*   **MSVC EH (`compiler_msvc_eh`)**: Parses MSVC Exception Handling tables to reconstruct try/catch code blocks.
*   **Debug Symbols**: Automatically loads and maps Microsoft PDB debug symbols into the disassembly listing.
