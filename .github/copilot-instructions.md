# Copilot Instructions

## Project Guidelines
- Retain pre-existing project header dependencies such as #include "FuncionesJuego.h" when editing headers; do not remove them unless explicitly requested.
- For required game structures and algorithms, avoid all prohibited STL containers, std::sort, and smart pointers; restrict non-container standard library use to std::string, iostream, fstream, sstream, cmath, cstdint, iomanip, and string helpers. Keep Allegro add-ons to primitives, font, ttf, image, audio, and acodec. Use new/delete for dynamic memory, and source gameplay randomness only from the specified section 3.6 generator (visual particles may use another source).
- Do not remove previously added library includes when editing project files; preserve existing dependencies unless the user explicitly requests their removal.
- In this project, omit explicit public/private/protected access labels in structs and rely on struct's default-public access.
- Prefer removing trailing `const` qualifiers from member function declarations and definitions in this project, including getters; leave const parameter references unless specifically requested.