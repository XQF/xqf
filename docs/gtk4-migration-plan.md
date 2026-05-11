# XQF GTK4 Modernization Plan

## Background

XQF started with GTK+1, was ported to GTK+2, and has a partial GTK+3 port
guarded by `#ifdef GUI_GTK2` / `#ifdef GUI_GTK3` blocks throughout the code.
The GTK+3 path is incomplete. The goal of this plan is to bring the project
onto a clean GTK 4 baseline, while respecting that XQF is **not a GNOME
project**: solutions that pull in GNOME-specific libraries or enforce GNOME
HIG conventions should be avoided.

## Current State Summary

- **Build system**: CMake, dual GTK2/GTK3 support via `GTK_TARGET` variable
  (default: 2).
- **Removed-in-GTK3 widgets still in use**: `GtkCList`, `GtkCTree` — core UI
  panels (server list, player list, server info tree).
- **Deprecated container API**: `GtkVBox`, `GtkHBox`, `GtkTable` throughout
  dialogs (50+ locations).
- **Styling**: Direct `GtkStyle` struct access (`style->bg[]`, `style->fg[]`).
  Incompatible with GTK3+.
- **Icons**: 101 XPM files in `src/xpm/`, included into C source as
  `static char*` arrays and loaded via `gdk_pixbuf_new_from_xpm_data()`.
- **Two `.ui` files**: `xqf-gtk2.ui` and `xqf-gtk3.ui` maintained in parallel.
- **CSS**: None. All styling is programmatic.
- **Signal API**: Already modern (`g_signal_connect`), no `gtk_signal_connect`
  legacy calls.
- **GtkBuilder**: Already in use for the main window.

## Non-Goals

- **libadwaita**: GNOME-specific widget set on top of GTK4. Not required, not
  used.
- **Meson**: Build system churn with no functional benefit. Stay on CMake.
- **GSettings**: Preference storage migration is a separate, unrelated concern.
- **GNOME HIG compliance**: XQF has its own UI conventions shaped by its
  gaming audience.

---

## Phase 1 — Drop GTK2/GTK3, target GTK4 directly

**Goal**: Single build target (GTK4), no conditional compilation for GTK
version, all `GtkCList`/`GtkCTree` usage replaced with the GTK4 list API.

**Why skip GTK3 as a deployment target**: GTK3 is in maintenance mode; writing
GTK3-compatible code (e.g. `gtk_box_pack_start`, `GtkContainer`,
`gtk_dialog_run`) would mean writing code that needs changing *again* for GTK4.
Since `GtkCList`/`GtkCTree` are being torn out entirely regardless of target
version, and the replacement work is the same either way, there is no
meaningful benefit to an intermediate GTK3 shipping target.

GTK3 can still serve as a *compile-time sanity check* during development (the
`GtkTreeView`-based fallback for the server info tree, described below, works
on both), but it is not a deployment goal.

**Why the new GTK4 list API, not `GtkTreeView`**: `GtkTreeView` is explicitly
legacy in GTK4. Choosing it now would invite a third migration later. Since the
`GtkCList` code is a complete rewrite regardless, the incremental cost of
targeting `GtkColumnView` is modest, and the benefits are real:
- Virtual rendering (only widgets for visible rows) — meaningful for large
  server lists.
- First-class `GtkSortListModel` and `GtkFilterListModel` — XQF does
  significant server filtering, so this composes well.

### Tasks

Legend: ✅ done · 🔲 pending

1. ✅ **Replace `GtkCList` (server list, player list)** with `GtkColumnView` +
   `GtkColumnViewColumn` backed by `GListStore` of `XqfServerItem` /
   `XqfPlayerItem` objects. Full implementation in `src/xqf-lists.c` (415 lines):
   - Server list: `GtkMultiSelection`, columns with `GtkCustomSorter` per column
   - Player list: `GtkSingleSelection`, with same factory/sorter pattern
   - Both use `GtkSortListModel` to compose sorting with the base store.
   - Implemented in `src/xqf-lists.[ch]`; GObject wrappers in
     `src/xqf-server-item.[ch]`, `src/xqf-player-item.[ch]`.

2. ✅ **Replace `GtkCTree` (server info tree panel)** — stubs in
   `src/gtk4-compat.h`; no `gtk_ctree_new` / `GTK_CTREE` calls remain in
   production source.

3. ✅ **Replace `GtkVBox` / `GtkHBox`** — no `gtk_vbox_new` / `gtk_hbox_new`
   calls remain in production source; compat shims in `gtk4-compat.h` map
   them to `gtk_box_new()`.

4. ✅ **Replace `GtkTable`** — no `gtk_table_new` / `gtk_table_attach` calls
   remain in production source; compat shims in `gtk4-compat.h` map them to
   `GtkGrid`.

5. ✅ **Remove `gdk_window_set_decorations()` / `gdk_window_set_functions()`**
   — no such calls exist in `src/dialogs.c`.

6. ✅ **Remove `#ifdef GUI_GTK2` / `#ifdef GUI_GTK3` blocks** — zero such
   blocks remain in source; only `GUI_GTK4` is defined (in `CMakeLists.txt`).

7. ✅ **Drop `xqf-gtk2.ui`**, rename `xqf-gtk3.ui` to `xqf.ui`, update it for
   GTK4 widget/property names. Main window rewritten to use `GtkBox` layout,
   `GtkPopoverMenuBar` + `GMenuModel`, and GTK4 pane/scroll structure.

8. ✅ **Update `CMakeLists.txt`**: single `pkg_check_modules(GTK REQUIRED gtk4)`;
   no `GTK_TARGET` variable or dual-target logic remains.

9. ✅ **Replace `gtk_builder_connect_signals()`** — stubbed out as a no-op in
   `gtk4-compat.h`; all signal connections are explicit `g_signal_connect`
   calls.

10. 🔲 **Replace `gtk_dialog_run()`** (removed in GTK4) with async `response`
    signal handling throughout `src/dialogs.c` and callers.

11. ✅ **Replace `GtkContainer` API** — `gtk_container_add()` and
    `gtk_container_set_border_width()` shimmed in `gtk4-compat.h` to dispatch
    to the correct GTK4 child-setter per container type.

---

## Phase 2 — Replace XPM icons with PNGs

**Goal**: No XPM files in the source tree; icons are PNG files installed to
a data directory and loaded at runtime.

**Why**: XPM is a 1990s format that requires recompiling to change an icon,
makes the binary larger, and is invisible to icon themes. `loadpixmap.c`
already has a PNG fallback code path.

### Tasks

1. **Convert all 101 files** in `src/xpm/` from XPM to PNG. This is a batch
   operation (`convert *.xpm` via ImageMagick).

2. **Install PNGs** to `${datadir}/xqf/pixmaps/` (added to `CMakeLists.txt`).

3. **Update `src/loadpixmap.c`**: remove `gdk_pixbuf_new_from_xpm_data()`
   path; load from the installed data directory only.

4. **Remove the `dlsym()` hack** in `src/pixmaps.c` that exists because XPM
   data is exported from the executable as symbols. With external files, this
   mechanism is no longer needed. (See the `CMAKE_EXECUTABLE_ENABLE_EXPORTS`
   note in `CMakeLists.txt` — that flag can be removed too.)

5. **Replace `pixmaps/xqf.xpm`** (the application icon) with an SVG. This one
   file is worth the artistic effort since it is used at multiple sizes and
   matters on HiDPI displays.

6. **Update `CMakeLists.txt`**: remove XPM install rules, add PNG install
   rules, remove `CMAKE_EXECUTABLE_ENABLE_EXPORTS`.

---

## Phase 3 — Replace direct `GtkStyle` manipulation with CSS

**Goal**: No direct access to `GtkStyle` struct fields; colors are set via
CSS or list model item properties.

**Why**: `GtkStyle` struct access was removed in GTK3 and does not exist in
GTK4.

### Tasks

1. **`src/skin.c`**: Replace `style->bg[]` and `style->fg[]` array access
   with CSS class-based coloring via a `GtkCssProvider`.

2. **`src/srv-prop.c`** and **`src/srv-list.c`**: Per-row server coloring —
   wire foreground/background color as properties on the `GListModel` item
   objects, bound to cell widget properties in the `GtkListItemFactory`.
   (Depends on Phase 1 completing the column view migration.)

3. For any remaining programmatic styling needs, introduce `src/style.css`
   loaded via `GtkCssProvider` at startup.

---

## Phase 4 — Adopt `GtkApplication`

**Goal**: Startup is managed by `GtkApplication`; `main()` calls
`g_application_run()`.

**Why**: `GtkApplication` is standard GTK infrastructure (not GNOME-specific).
It handles `SIGTERM`/`SIGHUP`, integrates with the platform session manager,
and is required by GTK4's recommended startup pattern.

### Tasks

1. Create a `GtkApplication` instance in `src/xqf.c` (or `src/main.c` if
   that exists).

2. Move window creation into the `activate` signal handler.

3. Keep the existing `getopt` argument parsing in a `handle-local-options`
   or `command-line` signal handler, or as a pre-`activate` step.

4. Verify single-instance behaviour if XQF has any existing logic for that.

---

## Phase 5 — Replace custom config parser with `GKeyFile`

**Goal**: `src/config.c` (~850 lines of custom INI parser) is deleted and
replaced by GLib's `GKeyFile` API throughout.

**Why**: The existing format is INI-style and `GKeyFile` reads it without
migration. Replacing the custom parser removes ~850 lines of code that
duplicates functionality GLib already provides, and gets correct handling of
edge cases (encoding, escaping, concurrent writes) for free. `GKeyFile` is
pure GLib — no GNOME dependency.

**Why last**: The config system has no GTK dependency and works fine through
Phases 1–4. Deferring it keeps the earlier phases focused and avoids mixing
a risky data-layer change with UI changes.

### Tasks

1. **Audit format differences** between `src/config.c` and `GKeyFile`:
   - Verify that all existing `~/.config/xqf/config` files parse correctly
     under `GKeyFile` without modification.
   - Check escape sequence handling (`\n`, `\r`, `\\` in values) — GKeyFile
     uses the same conventions but confirm edge cases.
   - The `servers` file uses server addresses (e.g. `192.168.1.1:27960`) as
     section headers; confirm `GKeyFile` accepts `:` in section names.

2. **Replace `config_get_*()` / `config_set_*()` call sites** in `src/pref.c`,
   `src/rc.c`, `src/source.c`, `src/srv-prop.c`, and other callers with
   `g_key_file_get_*()` / `g_key_file_set_*()` equivalents.

3. **Replace `config_sync()`** with `g_key_file_save_to_file()` (or
   `g_key_file_to_data()` + atomic write via `g_file_set_contents()`).

4. **Delete `src/config.c` and `src/config.h`**; remove from
   `CMakeLists.txt`.

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
| `src/config.c` → `GKeyFile` (Phase 5) | Round-trip: write keys, read back, verify values; escape sequences; `:` in section names (`servers` file) |
| `src/server.c`, `src/stat.c` | Server response packet parsing; address/port parsing |
| `src/filter.c`, `src/flt-player.c` | Filter rule evaluation against known server/player data |
| `src/host.c` | Host string parsing and validation |
| `src/rcon.c` (`BUILD_RCON`) | Packet construction and parsing for each supported protocol variant (Quake, HalfLife challenge, HexenWorld Huffman encoding) |
| Phase 1 `GListModel` implementations | Model item count, insert/remove, sort/filter composition |

**The config tests are particularly valuable for Phase 5**: write them against
the *existing* `src/config.c` API first, then use them as a regression harness
when switching to `GKeyFile`.

**Integration tests — `rcon` CLI binary:**

The `rcon` binary (`BUILD_RCON`, links only GLib + readline, no GTK) can be
tested end-to-end against a local UDP fixture or a real game server. A minimal
test fixture that speaks the Quake RCON protocol over UDP is straightforward
to write in Python or C and would let CI verify the full send/receive path
without a display.

### What not to test

GTK widget construction, dialog behaviour, list rendering — validated by
running the application. Attempting to unit-test GTK UI code requires a
display, GApplication initialization, and produces brittle tests with low
signal-to-noise.

### CMake wiring

Add a `tests/` subdirectory with its own `CMakeLists.txt`. Gate it behind a
`BUILD_TESTING` option (CMake's standard convention) so packagers can skip
tests at build time:

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
- **Icon theme integration**: Installing icons into the hicolor icon theme
  hierarchy (`share/icons/hicolor/…`) would be a natural follow-on to Phase 2
  but requires agreeing on icon naming conventions.

---

## Risk Notes

- **Phase 1 is the gate and the bulk of the work.** The `GtkCList` →
  `GtkColumnView` migration is a complete rewrite of the core data display.
  Column definitions, sorting, selection, row coloring, and model lifetime all
  need careful attention. `pref.c` (151 KB) should be tackled incrementally
  (task 3) rather than in one commit.
- **`GtkColumnView` learning curve**: The factory/model pattern is more
  verbose than `GtkTreeView`'s cell renderer approach. Budget time for
  understanding `GtkSignalListItemFactory` bind/setup lifecycle before writing
  production code.
- **The `dlsym()` / `CMAKE_EXECUTABLE_ENABLE_EXPORTS` mechanism** (Phase 2,
  task 4) is unusual and fragile. Removing it is a clear win, but requires
  verifying no other part of the code relies on exported symbols from the
  main executable.
- **`gtk_dialog_run()` replacement** (Phase 1, task 10) requires async
  refactoring of all modal dialogs. In GTK4 this is unavoidable; plan for
  it rather than discovering it late.
