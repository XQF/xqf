# XQF GTK4 Modernization Plan

## Background

XQF started with GTK+1, was ported to GTK+2, and has a partial GTK+3 port
guarded by `#ifdef GUI_GTK2` / `#ifdef GUI_GTK3` blocks throughout the code.
The GTK+3 path is incomplete. The goal of this plan is to bring the project
onto a clean GTK 4 baseline, while respecting that XQF is **not a GNOME
project**: solutions that pull in GNOME-specific libraries or enforce GNOME
HIG conventions should be avoided.

## Current State Summary

Phases 1–3 are complete. The app builds and runs on GTK4 with no known
crashes or regressions. What remains:

- **`gtk4-compat.h`** (265 lines, down from 780): all dead stubs removed.
  What stays are shims for APIs still actively called from un-migrated files
  (see Phase 6).
- **Deprecated tree widgets** (`GtkTreeView`, `GtkListStore`, `GtkTreeStore`,
  `GtkCellRenderer*`): still in use in filter.c, flt-player.c, srv-info.c,
  xqf-ui.c, scripts.c, pref.c (see Phase 5).
- **Compat shims still called** (~500 active call sites across 13 source
  files): `gtk_box_pack_start` (300), `gtk_entry_get/set_text` (86),
  `gtk_widget_destroy` (40), `gtk_frame_set_shadow_type` (13),
  `gtk_scrolled_window_new(h,v)` (12), and others (see Phase 6).
- **No `GtkApplication`**: WM shows "GTK application" instead of "XQF";
  no single-instance handling (see Phase 4).
- **Custom INI parser** (`src/config.c`, ~850 lines): duplicates `GKeyFile`
  functionality (see Phase 7).

---

## Build & Run (development / source-tree)

Build is done inside the `xqf-dev` Docker container (Dockerfile at `tools/docker/Dockerfile`,
image built from `debian:trixie-slim`). The container mounts the repo at `/src`, which maps
to `~/git/xqf` on the host. `CMAKE_INSTALL_PREFIX=/src` so installed files land at
`~/git/xqf/bin/xqf` and `~/git/xqf/share/xqf/`.

```bash
# Build and install (run from host)
docker run --rm -v ~/git/xqf:/src xqf-dev bash -c \
    "cmake --build /src/build -j\$(nproc) && cmake --install /src/build"
```

`WITH_QSTAT=quakestat` is already in `build/CMakeCache.txt` — on this machine qstat is
installed as `quakestat`. No need to pass it again unless reconfiguring from scratch.

## Commit discipline

One commit per logical fix or migration step. Keep commits focused so each one is
reviewable on its own. Verify a fix builds and runs before committing it.

---

## Current Known Bugs

No known bugs. The app builds, runs, and all main UI paths work:
- Server/player column views with full interaction (Phase 1 complete)
- Preferences dialog: color popovers, sound player entry, sound file pickers
  (modal `GtkFileChooserDialog` with `audio/*` filter), all save correctly
- Server Filters menu: radio items reflect current filter, update after config changes

---

## Deprecated API still in use

Several GTK APIs were deprecated in GTK 4.10. They work today but will
eventually be removed. Phase 5 covers the tree-widget subset.

| API | Deprecated since | Where used in XQF |
|-----|-----------------|-------------------|
| `GtkCellRendererPixbuf`, `GtkCellRendererText` | 4.10 | source treeview, games list, country filter, player filter |
| `GtkTreeView`, `GtkTreeStore`, `GtkListStore`, `GtkTreeViewColumn` | 4.10 | source treeview (`src/xqf-ui.c`), server info tree (`src/srv-info.c`), scripts list (`src/scripts.c`), games pref list (`src/pref.c`), country/player filter lists (`src/filter.c`, `src/flt-player.c`) |
| `gdk_texture_new_for_pixbuf` | 4.20 | `src/loadpixmap.c`, `src/pixmaps.c` — bridges PNG-loaded `GdkPixbuf` into `GdkTexture` for cell renderers; resolves when `GtkTreeView` usage is gone (Phase 5) |

## Removed API still shimmed in `gtk4-compat.h`

These APIs were fully removed in GTK4. They currently work through `gtk4-compat.h`
shims but should be replaced at the call site (Phase 6).

| Shim | Replacement | Active call sites |
|------|-------------|-------------------|
| `gtk_box_pack_start/end` | `gtk_box_append` | ~300 in 13 files |
| `gtk_entry_get/set_text` | `gtk_editable_get/set_text` | ~86 |
| `gtk_widget_destroy` | `gtk_window_destroy` (for windows), unreffing otherwise | ~40 |
| `gtk_frame_set_shadow_type` | CSS / `gtk_frame_set_child` | ~13 |
| `gtk_scrolled_window_new(h,v)` | `gtk_scrolled_window_new()` | ~12 |
| `gtk_scrolled_window_add_with_viewport` | `gtk_scrolled_window_set_child` | ~5 |
| `gtk_widget_set_can_default` / `gtk_widget_grab_default` | `gtk_window_set_default_widget` | ~20 |
| `GtkButtonBox` / `gtk_h/vbutton_box_new` | `GtkBox` directly | ~8 |
| `gtk_hseparator_new` | `gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)` | ~3 |
| `gtk_misc_set_alignment` | `gtk_label_set_xalign/yalign` | ~4 |
| `GdkEventButton` | `GtkGestureClick` | ~2 |
| `gtk_file_chooser_get/set_filename` | `gtk_file_chooser_get/set_file` | ~6 |

---

## Phase 1 — Drop GTK2/GTK3, target GTK4 directly ✅

**Goal**: Single build target (GTK4), no conditional compilation for GTK
version, all `GtkCList`/`GtkCTree` usage replaced with the GTK4 list API.

### Tasks

Legend: ✅ done · 🔲 pending

1. ✅ **Replace `GtkCList` (server list, player list)** with `GtkColumnView`.
   Data model, display, and full interaction layer are complete:
   - ✅ **1a** Double-click server to connect (`GtkGestureClick`)
   - ✅ **1b** Keyboard handling — Space=refresh, Enter=connect, Delete=remove
     (`GtkEventControllerKey`)
   - ✅ **1c** Right-click context menu on server list
     (`GtkGestureClick` + `GtkPopoverMenu`)
   - ✅ **1d** Right-click context menu on player list
   - ✅ **1e** Server filter radio items — _Server Filters_ menu now has a dynamic
     section (None / Filter 1 … Filter N) backed by a stateful `GSimpleAction`
     (`win.server-filter-select`, `G_VARIANT_TYPE_INT32`); section is rebuilt
     whenever the filter config changes.

2. ✅ **Replace `GtkCTree` (server info tree panel)** — implemented in
   `src/srv-info.c` using `GtkTreeView` + `GtkTreeStore` (the pragmatic GTK4
   option; see Phase 5 note).

3. ✅ **Replace `GtkVBox` / `GtkHBox`** — no `gtk_vbox_new` / `gtk_hbox_new`
   calls remain; code uses `gtk_box_new(GTK_ORIENTATION_*)` directly.

4. ✅ **Replace `GtkTable`** — no `gtk_table_new` / `gtk_table_attach` calls
   remain in production source.

5. ✅ **Remove `gdk_window_set_decorations()` / `gdk_window_set_functions()`**
   — no such calls exist.

6. ✅ **Remove `#ifdef GUI_GTK2` / `#ifdef GUI_GTK3` blocks** — zero such
   blocks remain in source; only `GUI_GTK4` is defined.

7. ✅ **Drop `xqf-gtk2.ui`**, rename `xqf-gtk3.ui` to `xqf.ui`, update it for
   GTK4 widget/property names.

8. ✅ **Update `CMakeLists.txt`**: single `pkg_check_modules(GTK REQUIRED gtk4)`.

9. ✅ **Replace `gtk_builder_connect_signals()`** — all signal connections are
   explicit `g_signal_connect` calls.

10. ✅ **Replace `gtk_dialog_run()`** — replaced with `dialog_run_modal(window)`
    (proper nested `GMainLoop`). App loop replaced with explicit `GMainLoop`.

11. ✅ **Replace `GtkContainer` API** — `gtk_container_add()` shimmed; remaining
    call sites use GTK4 child-setters directly or via the shim.

---

## Phase 2 — Replace XPM icons with PNGs ✅

**Goal**: No XPM files in the source tree; icons are PNG files installed to
a data directory and loaded at runtime.

### Tasks

1. ✅ **Convert all 101 files** in `src/xpm/` from XPM to PNG (ImageMagick
   batch conversion). PNG files placed in `pixmaps/default/`.

2. ✅ **Install PNGs** to `${PACKAGE_DATA_DIR}/default/` via CMakeLists.txt.

3. ✅ **Update `src/loadpixmap.c`**: removed `gdk_pixbuf_new_from_xpm_data()`
   path and `dlsym` fallback; loads only from installed data directory.

4. ✅ **Remove the `dlsym()` hack** in `src/pixmaps.c` and
   `CMAKE_EXECUTABLE_ENABLE_EXPORTS` from CMakeLists.txt.

5. ✅ **Replace application icon** with SVG (`pixmaps/scalable/xqf.svg`);
   PNG sizes generated at 22×22, 32×32, 48×48, 128×128 from the SVG.
   Icon theme search path registered at startup so the About dialog icon works.

---

## Phase 3 — Replace direct `GtkStyle` manipulation with CSS ✅

**Goal**: No direct access to `GtkStyle` struct fields; colors set via CSS.

### Tasks

1. ✅ **`src/gtk4-compat.h`**: Removed `GdkColor` / `GtkStyle` shims.

2. ✅ **`src/srv-prop.c`**: Stale timestamp coloring replaced with
   `gtk_widget_add_css_class(label, "xqf-stale")`.

3. ✅ **`src/xqf-lists.c`**: `SERVER_INCOMPATIBLE` greyed-out display restored
   via `"xqf-incompatible"` CSS class in `server_col_bind()`.

4. ✅ **`src/xqf.c`**: `GtkCssProvider` at startup with
   `.xqf-incompatible { color: alpha(currentColor, 0.45); }` and
   `.xqf-stale { color: red; }`.

---

## Phase 4 — Adopt `GtkApplication`

**Goal**: Startup managed by `GtkApplication`; `main()` calls
`g_application_run()`. Fixes WM showing "GTK application" instead of "XQF".

**Why**: `GtkApplication` sets `WM_CLASS` correctly, handles
`SIGTERM`/`SIGHUP`, integrates with the session manager, and is the
recommended GTK4 startup pattern.

### Tasks

1. Create a `GtkApplication` instance in `src/xqf.c`.

2. Move window creation into the `activate` signal handler.

3. Keep the existing `getopt` argument parsing as a pre-`activate` step or
   in a `handle-local-options` handler.

4. Verify single-instance behaviour (check whether XQF has any existing
   logic for that).

---

## Phase 5 — Replace deprecated `GtkTreeView` / `GtkListStore`

**Goal**: No use of `GtkTreeView`, `GtkTreeStore`, `GtkListStore`,
`GtkTreeViewColumn`, or `GtkCellRenderer*`. These were deprecated in GTK 4.10.

**Why now, not earlier**: The server/player lists (the hard case) were done
in Phase 1. What remains is all the filter, game-list, source-tree, scripts,
and server-info panels — each a self-contained migration that can be done
independently.

### Call sites

| File | Widget | Replacement |
|------|--------|-------------|
| `src/filter.c` | `GtkTreeView` + `GtkListStore` (country filter left/right lists) | `GtkColumnView` + `GListStore` |
| `src/flt-player.c` | `GtkTreeView` + `GtkListStore` (player filter list) | `GtkColumnView` + `GListStore` |
| `src/srv-info.c` | `GtkTreeView` + `GtkTreeStore` (server info panel) | Keep `GtkTreeView` — it is the correct fit for a small read-only tree; revisit only if GTK actually removes it |
| `src/xqf-ui.c` | `GtkTreeView` + `GtkTreeStore` (source/group tree) | `GtkTreeListModel` + `GtkColumnView`, or keep `GtkTreeView` same as srv-info |
| `src/scripts.c` | `GtkTreeView` + `GtkListStore` (scripts list) | `GtkColumnView` + `GListStore` |
| `src/pref.c` | `GtkTreeView` + `GtkListStore` (games list) | `GtkColumnView` + `GListStore` |

Once all `GtkTreeView` / `GtkListStore` uses are gone, `GtkCellRenderer*` and
`gdk_texture_new_for_pixbuf` disappear with them.

### Notes

- The filter dialogs (`filter.c`) also contain `GdkEventButton` call sites
  (two raw event-handler signatures) that should be replaced with
  `GtkGestureClick` at the same time.
- `src/xqf-ui.c` source tree uses `GTK_SELECTION_EXTENDED` and
  `gtk_paned_get_child1/2` — both shimmed; clean up when migrating that file.

---

## Phase 6 — Migrate remaining compat shims to native GTK4 API

**Goal**: `gtk4-compat.h` shrinks to near-zero (or is deleted); all call
sites use native GTK4 API directly.

**Why**: The shims in `gtk4-compat.h` are correctness patches, not
correctness guarantees — `gtk_box_pack_start` silently drops `expand`,
`fill`, and `padding` semantics that some dialogs may rely on for proper
layout. Migrating to `gtk_box_append` (and using `hexpand`/`vexpand`
properties where needed) also lets the compiler catch new regressions.

### Tasks

These are largely mechanical and can be done file-by-file or function-by-function:

1. **`gtk_box_pack_start` / `gtk_box_pack_end` → `gtk_box_append`**
   (~300 call sites in `addmaster.c`, `addserver.c`, `dialogs.c`, `filter.c`,
   `flt-player.c`, `game.c`, `pref.c`, `psearch.c`, `rcon.c`, `redial.c`,
   `scripts.c`, `srv-prop.c`, `statistics.c`).
   Where `expand=TRUE` / `fill=TRUE` was set, add `gtk_widget_set_hexpand`
   or `gtk_widget_set_vexpand` as appropriate.

2. **`gtk_entry_get/set_text` → `gtk_editable_get/set_text`** (~86 call sites).
   Already working via macro shim; mechanical replacement.

3. **`gtk_widget_destroy`** (~40 call sites): windows → `gtk_window_destroy`;
   non-window widgets → just unreffing or `gtk_widget_unparent` as appropriate.

4. **`gtk_widget_set_can_default` / `gtk_widget_grab_default`** (~20 call
   sites): use `gtk_window_set_default_widget (GTK_WINDOW (window), button)`.

5. **`GtkButtonBox` / `gtk_h/vbutton_box_new`** (filter.c, redial.c):
   replace with plain `GtkBox`; remove layout hints (no GTK4 equivalent).

6. **`gtk_frame_set_shadow_type`** (~13 call sites): remove the call entirely
   (shadow type is CSS-only in GTK4; the default frame appearance is fine).

7. **`gtk_scrolled_window_new(h,v)`** (~12 call sites): replace with
   `gtk_scrolled_window_new()`.

8. **`gtk_scrolled_window_add_with_viewport`** (~5 call sites): replace with
   `gtk_scrolled_window_set_child`.

9. **`gtk_hseparator_new`** (~3 call sites):
   `gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)`.

10. **`gtk_misc_set_alignment`** / `GTK_MISC` (~4 call sites):
    `gtk_label_set_xalign` / `gtk_label_set_yalign` directly.

11. **`gtk_misc_set_padding` / `gtk_bin_get_child` / `GtkBin`** (1 call each
    in `pref.c`): inline the margin-setting and `gtk_widget_get_first_child`.

12. **`gtk_file_chooser_get/set_filename`** (~6 call sites): replace with
    `gtk_file_chooser_get/set_file` + `g_file_get_path`.

13. **`gtk_container_add`** (~2 call sites): use the appropriate
    `gtk_box_append` / `gtk_window_set_child` / `gtk_frame_set_child` call.

14. **`GdkEventButton`** in `filter.c`: replace the two raw `GdkEventButton *`
    event-handler signatures with `GtkGestureClick` controllers.

Once all call sites are migrated, `gtk4-compat.h` can be deleted.

---

## Phase 7 — Replace custom config parser with `GKeyFile`

**Goal**: `src/config.c` (~850 lines of custom INI parser) is deleted and
replaced by GLib's `GKeyFile` API throughout.

**Why**: The existing format is INI-style and `GKeyFile` reads it without
migration. Replacing the custom parser removes ~850 lines of code that
duplicates functionality GLib already provides, and gets correct handling of
edge cases (encoding, escaping, concurrent writes) for free. `GKeyFile` is
pure GLib — no GNOME dependency.

**Why last**: The config system has no GTK dependency and works fine through
Phases 1–6. Deferring it keeps the earlier phases focused and avoids mixing
a risky data-layer change with UI changes.

### Tasks

1. **Audit format differences** between `src/config.c` and `GKeyFile`:
   - Verify that all existing `~/.config/xqf/config` files parse correctly
     under `GKeyFile` without modification.
   - Check escape sequence handling (`\n`, `\r`, `\\` in values).
   - The `servers` file uses server addresses (e.g. `192.168.1.1:27960`) as
     section headers; confirm `GKeyFile` accepts `:` in section names.

2. **Replace `config_get_*()` / `config_set_*()` call sites** in `src/pref.c`,
   `src/rc.c`, `src/source.c`, `src/srv-prop.c`, and other callers with
   `g_key_file_get_*()` / `g_key_file_set_*()` equivalents.

3. **Replace `config_sync()`** with `g_key_file_save_to_file()` (or
   `g_key_file_to_data()` + atomic write via `g_file_set_contents()`).

4. **Delete `src/config.c` and `src/config.h`**; remove from `CMakeLists.txt`.

5. **Keep `src/rc.c`** for legacy `~/.qf/` migration logic and `qfrc` import
   — but the runtime read/write path moves to `GKeyFile`.

---

## Testing Strategy

XQF has no tests. Tests should be added **incrementally alongside the phases**
rather than in a dedicated pass, targeting code that can be exercised without
a display.

### Framework

**GLib GTest** (`g_test_*`). Zero new dependencies — GLib is already required.
Integrates with CMake via `enable_testing()` + `add_test()`. This is what GTK
itself uses.

### What to test

**Unit tests — pure logic, no GTK, no display required:**

| Module | What to cover |
|---|---|
| `src/config.c` → `GKeyFile` (Phase 7) | Round-trip: write keys, read back, verify values; escape sequences; `:` in section names (`servers` file) |
| `src/server.c`, `src/stat.c` | Server response packet parsing; address/port parsing |
| `src/filter.c`, `src/flt-player.c` | Filter rule evaluation against known server/player data |
| `src/host.c` | Host string parsing and validation |
| `src/rcon.c` (`BUILD_RCON`) | Packet construction and parsing for each supported protocol variant (Quake, HalfLife challenge, HexenWorld Huffman encoding) |

**Integration tests — `rcon` CLI binary:**

The `rcon` binary (`BUILD_RCON`, links only GLib + readline, no GTK) can be
tested end-to-end against a local UDP fixture or a real game server.

### What not to test

GTK widget construction, dialog behaviour, list rendering — validated by
running the application.

### CMake wiring

```cmake
option(BUILD_TESTING "Build test suite" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Deferred / Out of Scope for Now

- **`.ui` file split into per-dialog files**: `pref.c` at 151 KB builds its
  UI programmatically. Moving it to GtkBuilder `.ui` files would be a large
  refactor with limited functional benefit.
- **`src/xpm/` deletion**: The old XPM files are still in the tree (Phase 2
  stopped short of deleting them). Remove with a single `git rm src/xpm/`.
