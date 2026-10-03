# LabX data

`legacy_converted/` contains deterministic, reusable conversions of the original
course assets. Run `python tools/convert_legacy_assets.py` from the repository
root to rebuild it.

- `meshes/`: glTF 2.0 JSON plus external binary buffers converted from OBJ/ASC.
- `scenes/`: normalized JSON command streams converted from legacy `.in` files.
- `manifest.json`: source provenance, geometry counts, output paths, and any
  unresolved object references.

Same-name ASC and OBJ inputs use `_from_asc` and `_from_obj` suffixes. Keeping
both allows tests to detect conversion discrepancies. Generated data is checked
in so CPU, CUDA, and OptiX tests do not depend on the legacy parsers at runtime.
