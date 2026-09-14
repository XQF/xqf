# XQF GTK4 Modernization Plan

## Background

XQF started with GTK+1, was ported to GTK+2, and has a partial GTK+3 port
guarded by `#ifdef GUI_GTK2` / `#ifdef GUI_GTK3` blocks throughout the code.
The GTK+3 path is incomplete. The goal of this plan is to bring the project
onto a clean GTK 4 baseline, while respecting that XQF is **not a GNOME
project**: solutions that pull in GNOME-specific libraries or enforce GNOME
HIG conventions should be avoided.

## Current State Summary

Phases 1–9 are complete. The app builds and runs on GTK4 with no known
crashes or regressions. No deprecated APIs remain in use except the
intentionally-kept tree widgets (see below).

- **Deprecated tree widgets** (`GtkTreeView`, `GtkTreeStore`, `GtkCellRenderer*`):
  intentionally kept in `src/srv-info.c` and `src/xqf-ui.c`. See the note in
  Phase 5 for the rationale.
- **Follows the desktop's light/dark preference**: xqf never set
  `gtk-application-prefer-dark-theme`, so it always rendered in GTK's light
  default regardless of the desktop. Now queries
  `org.freedesktop.portal.Settings`' `Read` method for
  `org.freedesktop.appearance`'s `color-scheme` key at startup, and listens
  for `SettingChanged` to pick up live theme switches — the
  freedesktop.org-standard, desktop-agnostic mechanism (GNOME, KDE, etc.),
  not GNOME-specific `org.gnome.desktop.interface`. Async and best-effort:
  no portal running means xqf silently stays on the GTK default.
- **Default main window size bumped to 1200x800** (was smaller), and the
  source-tree pane on the left widened to 260px so game names like "Medal
  of Honor: Allied Assault" (30 chars, the longest) fit without a
  horizontal scrollbar or truncation.

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

The app builds, runs, and all main UI paths work:
- Server/player column views with full interaction (Phase 1 complete)
- Preferences dialog: color popovers, sound player entry, sound file pickers
  (modal `GtkFileChooserDialog` with `audio/*` filter), all save correctly
- Server Filters menu: radio items reflect current filter, update after config changes

Known limitations:
- **Help menu shows extra empty space below "About"**, unlike every
  other menu bar entry. Confirmed via live testing (X11 window capture
  of the actual popover) that this is not caused by xqf's menu markup:
  `xqf.ui`'s Help submenu is now structured identically to every other
  menu (single `<section>` wrapping its `<item>`), and a CSS override
  (`popover.menu { min-height: 0; }`) had zero effect on the popover's
  measured size. The popover is a fixed 168x128px regardless of content
  — "About" only needs the top ~45px. Every other menu's real content
  already exceeds that floor (confirmed: `Edit`'s 14-item, 1040px-tall
  popover has no trailing gap), so it's only visible on this app's one
  single-item menu. Looks like a GTK4/Adwaita popover minimum-size
  behavior outside the app's control; not pursued further.

Fixed since first identified (kept here as a pointer to the pattern, in
case more turn up):
- **Map level-shot and player-skin previews**, lost when clicking the map
  or Colors/Skin column. Both used the same now-gone GTK2/3 mechanism
  (`GTK_WINDOW_POPUP` + `gdk_pointer_grab()` on a `GtkCList`
  button-press handler) and were stubbed to no-ops during the
  `GtkColumnView` migration rather than reworked, with the underlying
  data path (`get_mapshot()`/`renderMemToGtkPixbuf()`,
  `get_qw_skin()`/`draw_qw_skin()`) left intact and unused. Both are now
  persistent rows (server-info panel / player list) updated on
  selection change, per maintainer preference for panel rows over
  popups. Worth checking for a third instance of this same pattern
  (stubbed-and-orphaned rather than reworked) if any other feature is
  reported missing.

---

## Deprecated API still in use

Several GTK APIs were deprecated in GTK 4.10. They work today but will
eventually be removed. Phase 5 covers the tree-widget subset.

| API | Deprecated since | Replacement available | Where used in XQF |
|-----|-----------------|----------------------|-------------------|
| `GtkComboBox` / `GtkComboBoxText` | 4.10 | 4.0 (`GtkDropDown`) | ✅ Phase 8 — replaced |
| `gtk_image_new/set_from_pixbuf` | 4.12 | 4.0 (`from_paintable`) | ✅ Phase 9 — replaced |
| `gtk_widget_get_allocation` | 4.12 | 4.0 (`get_width/height`) | ✅ Phase 9 — replaced |
| `GtkCellRendererPixbuf`, `GtkCellRendererText` | 4.10 | 4.0 | source treeview, server info — intentionally kept |
| `GtkTreeView`, `GtkTreeStore`, `GtkListStore`, `GtkTreeViewColumn` | 4.10 | 4.0 | `src/xqf-ui.c`, `src/srv-info.c` — intentionally kept |
| `gdk_texture_new_for_pixbuf` | 4.20 | — | kept with the GtkTreeView code above |

## Removed API previously shimmed in `gtk4-compat.h`

`gtk4-compat.h` bridged APIs GTK4 removed outright (`gtk_box_pack_start/end`,
`gtk_entry_get/set_text`, `gtk_widget_destroy`, `gtk_frame_set_shadow_type`,
`gtk_scrolled_window_new(h,v)`, `gtk_scrolled_window_add_with_viewport`,
`gtk_widget_set_can_default`/`gtk_widget_grab_default`, `GtkButtonBox`,
`gtk_hseparator_new`, `gtk_misc_set_alignment`, `GdkEventButton`,
`gtk_file_chooser_get/set_filename`) while call sites were migrated
incrementally. Phase 6 finished migrating every call site to native GTK4
API; the file itself was deleted once nothing referenced it — see Phase 6
below.
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

6. ✅ **Delete `src/xpm/`** — 101 XPM source files removed; PNGs in
   `pixmaps/default/` are the sole icon source.

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

## Phase 4 — Adopt `GtkApplication` ✅

**Goal**: Startup managed by `GtkApplication`; `main()` calls
`g_application_run()`. Fixes WM showing "GTK application" instead of "XQF".

### Status

Complete. `src/xqf.c` creates `GtkApplication("io.github.xqf",
G_APPLICATION_NON_UNIQUE)`, connects `xqf_activate` to the `activate`
signal, and calls `g_application_run(app, 0, NULL)`. `getopt` argument
parsing runs before `g_application_run` so there are no conflicts with
GApplication's own option handling. `gtk_application_add_window` is called
in `create_main_window`.

---

## Phase 5 — Replace deprecated `GtkTreeView` / `GtkListStore` ✅

**Goal**: No use of `GtkTreeView`, `GtkTreeStore`, `GtkListStore`,
`GtkTreeViewColumn`, or `GtkCellRenderer*`. These were deprecated in GTK 4.10.

**Why now, not earlier**: The server/player lists (the hard case) were done
in Phase 1. What remains is all the filter, game-list, source-tree, scripts,
and server-info panels — each a self-contained migration that can be done
independently.

### Call sites

| File | Widget | Status |
|------|--------|--------|
| `src/filter.c` | `GtkTreeView` + `GtkListStore` (country filter lists) | ✅ Migrated to `GtkColumnView` + `GListStore` |
| `src/flt-player.c` | `GtkTreeView` + `GtkListStore` (player filter list) | ✅ Migrated to `GtkColumnView` + `GListStore` |
| `src/scripts.c` | `GtkTreeView` + `GtkListStore` (scripts list) | ✅ Migrated to `GtkListView` + `GtkStringList` |
| `src/pref.c` (games) | `GtkTreeView` + `GtkListStore` (games list) | ✅ Migrated to `GtkListView` + `GListStore` |
| `src/pref.c` (args) | `GtkTreeView` + `GtkListStore` (custom args list) | ✅ Migrated to `GtkColumnView` + `GListStore` |
| `src/srv-info.c` | `GtkTreeView` + `GtkTreeStore` (server info panel) | Keep — see note below |
| `src/xqf-ui.c` | `GtkTreeView` + `GtkTreeStore` (source/group tree) | Keep — see note below |

`GtkCellRenderer*` and `gdk_texture_new_for_pixbuf` are gone from all
migrated files. They remain only in `srv-info.c` and `xqf-ui.c` (kept trees).

### Why the two tree widgets are intentionally kept

Both display genuinely hierarchical data: the source panel has collapsible
groups containing individual masters; the server info panel has nested
key/value categories. `GtkTreeView` + `GtkTreeStore` is the natural fit for
that shape.

The GTK4 replacement for trees is `GtkTreeListModel` + `GtkColumnView` /
`GtkListView`. It requires a `GtkTreeListModelCreateModelFunc` that returns a
child `GListModel` for every node, a `GtkTreeListRow` wrapper in the factory
bind callback, and manual expand/collapse handling. For flat lists (migrated
in Phases 1 and 5) the new API is genuinely cleaner. For trees it is the
opposite: substantially more boilerplate, same user-visible result.

`GtkTreeView` is deprecated since 4.10 but not removed, and GTK has a history
of keeping deprecated widgets around for a long time. The cost of migrating
now (non-trivial complexity, no functional improvement) outweighs the benefit
(suppressing deprecation warnings for something with no removal timeline).
Revisit only if GTK schedules an actual removal.

---

## Phase 6 — Migrate remaining compat shims to native GTK4 API ✅

**Goal**: `gtk4-compat.h` shrinks to near-zero (or is deleted); all call
sites use native GTK4 API directly.

**Status**: Complete. `gtk4-compat.h` deleted.

### Tasks

1. ✅ **`gtk_box_pack_start` / `gtk_box_pack_end` → `gtk_box_append`**
2. ✅ **`gtk_entry_get/set_text` → `gtk_editable_get/set_text`**
3. ✅ **`gtk_widget_destroy` → `gtk_window_destroy`**
4. ✅ **`gtk_widget_set_can_default` / `gtk_widget_grab_default`**
5. ✅ **`GtkButtonBox` / `gtk_h/vbutton_box_new`**
6. ✅ **`gtk_frame_set_shadow_type`**
7. ✅ **`gtk_scrolled_window_new(h,v)` → `gtk_scrolled_window_new()`**
8. ✅ **`gtk_scrolled_window_add_with_viewport`**
9. ✅ **`gtk_hseparator_new`**
10. ✅ **`gtk_misc_set_alignment`** / `GTK_MISC`
11. ✅ **`gtk_misc_set_padding` / `gtk_bin_get_child` / `GtkBin`**
12. ✅ **`gtk_file_chooser_get/set_filename`** — replaced with `GtkFileDialog`
    (GTK ≥ 4.10) / `GtkFileChooserDialog` fallback; `gtk4-compat.h` deleted.
13. ✅ **`gtk_container_add`**
14. ✅ **`GdkEventButton`** in `filter.c`

---

## Phase 7 — Replace custom config parser with `GKeyFile` ✅

**Goal**: `src/config.c` (~850 lines of custom INI parser) is deleted and
replaced by GLib's `GKeyFile` API throughout.

**Status**: Complete. `src/config.c` rewritten to ~340 lines backed by
`GKeyFile`. Public API in `config.h` is unchanged. Existing config files are
format-compatible (same INI dialect, same escape sequences).

---

## Phase 8 — Replace `GtkComboBox` with `GtkDropDown` ✅

**Goal**: No `GtkComboBox` / `GtkComboBoxText` usage. Both were deprecated in
GTK 4.10; `GtkDropDown` + `GtkStringList` have been available since GTK 4.0
so no version guard is needed.

### Call sites

| File | Usage | Notes |
|------|-------|-------|
| `src/pref.c` | Game sound player combo, game-type combos | Several instances |
| `src/filter.c` | Game-type filter combo | Uses model + active index |
| `src/scripts.c` | Script event combo | `GtkComboBoxText` with entry |
| `src/rcon.c` | Server command combo | `GtkComboBoxText` with entry |
| `src/srv-prop.c` | Game/gamedir combos | `GtkComboBoxText` with entry |
| `src/statistics.c` | Statistics grouping combo | Uses `GtkListStore` model |
| `src/addserver.c` | Server type combo | `GtkComboBoxText` with entry |
| `src/addmaster.c` | Master type combo | `GtkComboBoxText` with entry |
| `src/psearch.c` | Player search combo | `GtkComboBoxText` with entry |
| `src/xqf-ui.c` | Game filter combo | Uses `GtkListStore` + pixbuf column |

---

## Phase 9 — Replace remaining deprecated GTK 4.12 APIs ✅

**Goal**: No `gtk_image_*_from_pixbuf` or `gtk_widget_get_allocation` usage.
Replacements available since GTK 4.0; no version guard needed.

**Status**: Complete.

### Tasks

1. ✅ **`gtk_image_new_from_pixbuf` / `gtk_image_set_from_pixbuf`** (deprecated 4.12)
   → `gtk_image_new_from_paintable` / `gtk_image_set_from_paintable`:
   `src/loadpixmap.c`, `src/game.c`, `src/rcon.c`, `src/srv-prop.c`,
   `src/statistics.c`, `src/skin.c`.
   Country flag loading in `src/country-filter.c` now also creates `pix->texture`
   so all call sites use the paintable API uniformly.

2. ✅ **`gtk_widget_get_allocation`** (deprecated 4.12) → `gtk_widget_get_width/height`:
   `src/rcon.c`, `src/xqf-ui.c`, `src/statistics.c`.

---

## Testing Strategy

XQF has no tests. Tests should be added **incrementally alongside the phases**
rather than in a dedicated pass, targeting code that can be exercised without
a display.

### Framework

**GLib GTest** (`g_test_*`). Zero new dependencies — GLib is already required.
Integrates with CMake via `enable_testing()` + `add_test()`. This is what GTK
itself uses. Wired up under `tests/`, run via `ctest` from the build directory.

### What to test

**Unit tests — pure logic, no GTK, no display required:**

| Module | What to cover |
|---|---|
| ✅ `src/config.c` → `GKeyFile` (Phase 7) | `tests/test_config.c`: round-trip for int/float/bool/string; `\n`/`\r`/`\\` escapes; `path=default` hint; `:` in section names; prefix stack; disk persistence via `config_sync()` |
| ✅ `src/host.c` | `tests/test_host.c`: address validation via `host_add()`; dedup-by-address; ref-counted free at zero; `all_hosts()` ref/unref balance |
| ✅ `src/filter.c` → `src/filter-eval.c` | `tests/test_filter.c`: `server_pass_filter()` (ping/retries/full/empty/cheats/password/game/gametype/map/version/name/country) and `quick_filter()`. The pure evaluation logic was split out of `filter.c`'s GTK dialog code into a new `filter-eval.c`/`.h` (`server_filter_vars_new/free/copy`, `server_pass_filter`, `quick_filter`, `filter_quick_*`, plus the `server_filters`/`current_server_filter` globals); `filter.c` now calls into it and keeps only the dialog UI |
| ✅ `src/flt-player.c` → `src/player-filter-eval.c` | `tests/test_player_filter.c`: `player_filter()` across string/substr/regexp pattern modes, including an invalid-regexp case. Same split as above: `player_filter`, `player_pattern_new/compile`, `free_player_pattern[_compiled_data]`, and the `players`/`curplrs` globals moved to `player-filter-eval.c`/`.h`; `flt-player.c` keeps the pattern-editing dialog UI and reads/writes those globals through the header |
| ✅ `src/server.c` → `src/addr-parse.c` (`parse_address`) | `tests/test_addr_parse.c`: hostname-only, IP-only, host:port, out-of-range/non-numeric port, leading colon, NULL args. `parse_address()` didn't touch server.c's server/userver hash tables, so it moved out whole rather than dragging in `game.c`'s generated games database just to link it |
| ✅ `src/rcon.c` → `src/rcon-msg.c` (`msg_terminate`) | `tests/test_rcon_msg.c`: all four termination-byte cases (already terminated, NUL but no `\n`, `\n` but no NUL, neither) plus the 0/1-byte edge cases. Same story: no sockets/huffman/GTK, so it moved out whole rather than linking readline (`BUILD_RCON`) or the GTK console widget (`BUILD_XQF`) |
| ✅ `src/huffman.c` (HexenWorld Huffman codec) | `tests/test_huffman.c`: encode/decode round-trip (empty, single byte, ASCII text, a repeated byte, all 256 byte values) plus a truncated-decode-stays-in-bounds check. Already fully self-contained (no glib, no app globals) — no extraction needed, just a test file. Caught a real buffer-sizing gotcha along the way: `HuffEncode()` writes the speculative compressed bitstream *before* deciding whether to fall back to a raw copy, so a caller must size the output buffer well above `inlen+1` (rcon.c does this by always encoding into its fixed 64KB `huffbuff`, never a buffer sized to the input) |
| `src/server.c` (the rest), `src/stat.c` | Server response packet parsing beyond address parsing — `server.c`'s remaining functions manage the server/userver hash tables and need `game.c`'s generated games database (`games[]`) linked in; `stat.c` additionally pulls in `dialogs.h`/`source.h`. Testable in principle with more extraction work, not attempted here |
| `src/rcon.c` (the rest, `BUILD_RCON`) | Packet construction/parsing for the rest of the protocol (challenge/response framing) — entangled with the live socket; would need either further extraction or driving the `rcon` CLI binary as a subprocess against a local UDP fixture |

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
