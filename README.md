<p align="center">
  <img src="ROX-Filer.svg" alt="ROX-Filer GTK3 logo" width="180">
</p>

<h1 align="center">ROX-Filer 2.12 GTK3</h1>

<p align="center">
  A modern GTK3 port of the classic, fast and lightweight ROX-Filer file manager.
</p>

<p align="center">
  Maintained by <strong>josejp2424</strong>
</p>

---

## Overview

ROX-Filer is a fast and lightweight graphical file manager originally created
by **Thomas Leonard** for the ROX Desktop.

This repository contains the ongoing port of **ROX-Filer 2.12 to GTK3**

The goal of this project is to preserve the speed, simplicity, flexibility and
traditional behaviour of the original ROX-Filer while replacing its GTK2-era
implementation with a modern GTK3 code base.

The main filer interface, file views, menus, preferences, file operations,
input handling, GTK theme integration, icon-theme support and dialogs have
already been adapted to GTK3. Development may continue with additional testing,
cleanup, translation work and compatibility improvements.

This version is intended primarily for lightweight X11 and XLibre desktops,
including:

- Puppy Linux
- EssoraPup
- Essora
- JWM
- EssoraWM
- Other lightweight X11/XLibre environments

> ROX-Filer GTK3 currently targets X11/XLibre. It is not a native Wayland port.

## Project goals

- Preserve the classic ROX-Filer workflow.
- Maintain fast startup and low resource usage.
- Remove the dependency on GTK2.
- Integrate correctly with GTK3 themes and icon themes.
- Improve behaviour on modern Puppy Linux and Essora systems.
- Keep the source understandable and maintainable.
- Preserve all original copyright and contributor notices.
- Add practical desktop features without making the filer unnecessarily heavy.

## Current GTK3 status

The project builds against:

```text
GTK+ 3.22 or newer
```

The source uses GTK3 for the main interface and no longer requires GTK2 for the
normal build.

Major GTK3 porting work includes:

- GTK3 widgets and containers.
- GTK3 event handling.
- Cairo-based custom drawing.
- `GtkStyleContext` theme integration.
- GTK3-compatible menus.
- GTK3-compatible scroll adjustments and file views.
- GTK3-compatible drag-and-drop handling.
- GTK3 preferences and dialogs.
- GTK3 icon-theme loading.
- Native GTK3 About dialog.
- Replacement of removed GTK2 menu infrastructure.
- Compatibility with X11/XLibre through GDK X11 and Xlib.

Some historical compatibility code and deprecated-but-still-supported GTK3 APIs
may remain and can be cleaned up gradually without preventing the application
from being a GTK3 program.

## Main features

- Fast and lightweight graphical file manager.
- Classic ROX-Filer icon and list views.
- Desktop pinboard support.
- Panel support.
- Drag and drop.
- Fast rsync-assisted copy and move operations.
- Standard Freedesktop Trash integration through GIO.
- Separate permanent deletion with Shift+Delete.
- File type handling and application associations.
- AppDir support.
- Mount and unmount integration.
- Symbolic-link handling.
- Extended-attribute support.
- Per-directory display settings.
- Configurable toolbar.
- GTK3 theme and icon-theme integration.
- Multilingual interface.
- Integrated terminal actions.
- Permanent partition browser in the main toolbar.
- Mount-and-open support for internal and removable partitions.
- Directories-first ordering without hiding normal file types.
- Native GTK3 About dialog.

## Window defaults

New filer windows use a real initial size of:

```text
640 × 400 pixels
```

This size is applied after the initial GTK3 window mapping so that the content
layout cannot stretch a new window into a single wide row.

A saved per-directory geometry continues to take priority when one exists, but
older saved sizes are clamped so no normal filer instance can open below
640 × 400 pixels. The window remains fully resizable above that minimum.

## Preferences window

The Preferences window uses a default size of:

```text
640 × 400 pixels
```

Large settings pages are placed inside scrollable GTK3 containers. This prevents
their natural size from expanding the dialog beyond the available screen area.

The category list also has its own compact scrollable panel.

## Permanent partition button

The main filer toolbar includes a permanent **Partitions** button. It is added
outside the configurable toolbar-item list, so it cannot be removed or disabled
from the toolbar preferences.

The button uses the active icon theme's standard:

```text
drive-harddisk
```

icon and refreshes the partition list each time it is opened. It is inserted
immediately after the **Up** button and has an additional fallback insertion
path so an old toolbar configuration cannot hide it.

The partition detection is based on the drive-handling approach used by
EssoraWM. It reads `lsblk` data and filters technical devices that should not be
presented as normal user volumes, including:

- Loop devices
- ZRAM and RAM devices
- Device-mapper helper devices
- Puppy Linux runtime layers
- SquashFS, overlay and AUFS layers
- Swap volumes
- EFI system partitions

For compatibility with older Puppy Linux versions, ROX-Filer first requests the
complete modern `lsblk` column set and automatically retries with a smaller
compatible set when some columns are unavailable.

Mounted partitions open directly in the current filer window. When a partition
is not mounted:

- Puppy Linux and other root sessions mount it under `/mnt/<device>` using
  `/bin/mount`.
- Regular-user sessions can use `udisksctl mount -b` when it is available.
- After a successful mount, ROX-Filer opens the resulting mount point.

The partition selector is a GTK3 popover arranged as a four-column grid. The
first four units appear on the first row, the next four on the second row, and
so on. Each unit shows its volume label, device name, size and mounted state.


<p align="center">
  <img src="screenshot/rox-particiones.png" alt="partition">
</p>
## File visibility and ordering

The historical toolbar control that switched between **directories only** and
**files only** was removed. It could be activated accidentally and make normal
files appear to be missing, including archives such as `.tar.gz`, AppImages,
shell scripts, Python files and other regular file types.

ROX-Filer now keeps every normally visible directory and file in the view.
Inherited filters, saved per-directory filters and glob filters are ignored in
the normal filer view so archives, AppImages, scripts and other files cannot
disappear. The standard Hidden button remains the only visibility control for
dotfiles and other hidden entries.

Normal directories are always placed first. Every remaining visible file is
shown after the directory group. This rule applies to:

- Icon view
- Detailed list view
- Stable ascending name ordering with folders first

AppDirs continue to behave as applications rather than ordinary directories.

## Fast rsync copy and move engine

ROX-Filer GTK3 includes a hybrid file-operation engine designed to improve the
speed of large local copies.

The historical engine starts an external `cp` process for each regular file.
That behaviour is reliable but becomes slow when a directory contains hundreds
or thousands of small files.

When `rsync` is available, this version uses:

- One rsync process for a complete directory tree.
- One rsync batch for multiple selected items when their destination types are
  compatible.
- `rename()`/`mv` for moves inside the same filesystem.
- `rsync --remove-source-files` for moves across filesystems and directory
  merges.
- `--partial` to preserve partial transfer data after an interrupted copy.

ROX-Filer never adds `--delete` to normal copy or move operations, so unrelated
files already present in the destination are not removed.

If `rsync` is not installed, ROX-Filer automatically uses its classic copy and
move engine.

### Conflict policy

When destination items already exist, ROX-Filer displays one conflict-policy
dialog. The choices are presented as four large ROX-style buttons instead of a
drop-down menu:

- Ask for each conflict.
- Replace existing files.
- Skip existing files.
- Replace only if the source is newer.

The buttons are arranged in a compact two-by-two grid and use standard icons
from the active GTK3 icon theme. **Ask for each conflict** is the safe default
and can be activated with Enter.

When **Ask for each conflict** is selected, the traditional comparison dialog
also provides **Apply this decision to all remaining conflicts**. This allows a
single Replace or Skip decision to be reused for the rest of the operation.

The selected policy applies only to the current copy or move and is reset before
the next operation.

## Terminal integration

ROX-Filer GTK3 adds two practical file-context-menu actions.

### Open Terminal Here

When a directory is selected, the contextual menu provides:

```text
Open Terminal Here
```

The configured terminal emulator opens with the selected directory as its
working directory.

### Run in Terminal

The contextual menu provides:

```text
Run in Terminal
```

for supported files, including:

- Executable native binaries
- Executable shell scripts
- Files ending in `.sh`
- Python scripts ending in `.py`
- Python GUI scripts ending in `.pyw`
- Executable AppImage files
- Other executable files with a valid interpreter line

The command is launched with safely separated arguments, supports paths
containing spaces and keeps the terminal open until the user presses Enter.

The terminal command remains controlled by ROX-Filer's existing:

```text
menu_xterm
```

option.

The terminal menu entries use the standard icon-theme name:

```text
utilities-terminal
```

### Optional runtime tools

Depending on the selected file, the terminal integration may use:

- A configured terminal emulator, such as `xterm`
- `/bin/sh`
- `python3`

AppImage files must have executable permission before they can be launched.

## Standard Trash and permanent deletion

The normal **Delete** key now moves the selected items to the standard
Freedesktop Trash used by PCManFM, EssoraFM and other compatible file managers.

The implementation uses GIO, so each filesystem can use its correct local Trash
location and write the standard metadata required for restoring files.

The interface preserves the traditional compact ROX style:

- One small confirmation dialog for the whole selection.
- **Delete** moves the selection to Trash.
- **Shift+Delete** opens a separate permanent-delete confirmation.
- Permanent deletion asks only once for the complete selection.
- Small confirmation buttons are used instead of large modern panels.
- Filesystems without Trash support show an error and are never silently
  converted to permanent deletion.

Moving a directory to Trash does not recursively process every file inside it.
The complete top-level directory is moved by the GIO backend, which makes the
operation much faster for directories containing many files.

Permanent deletion continues to use the native ROX deletion engine, but in
batch mode it avoids per-file questions and refreshes only the selected
top-level paths when the operation finishes.

## File-operation dialog fixes

The GTK3 port includes corrections for file-operation dialogs that previously
remained visible after an operation had already completed.

The child-process communication now handles pending input correctly when GLib
reports input and pipe closure at the same time.

This applies to operations such as:

- Copying files
- Replacing existing files
- Moving files
- Deleting files
- Deleting directories recursively

The progress dialog closes after a successful operation and remains open only
when an actual error must be shown to the user.

## GTK3 theme integration

ROX-Filer uses the active GTK3 settings, widget theme and icon theme.

GTK3 normally reads the user's configuration from:

```text
~/.config/gtk-3.0/settings.ini
```

For example:

```ini
[Settings]
gtk-theme-name=YourGtkTheme
gtk-icon-theme-name=YourIconTheme
gtk-font-name=Sans 10
```

This port does not force a fixed filer background or text colour. File views use
standard GTK3 styling so that the selected system theme controls their
appearance.

## System icon theme

ROX-Filer GTK3 uses the active system icon theme for interface graphics such as:

- Toolbar actions
- Folders
- Applications and AppDirs
- List view
- Selection actions
- Mounted devices
- Unmount and eject actions
- Symbolic links
- Extended attributes
- Iconified windows
- Terminal actions

Examples of standard icon names used by the application include:

```text
folder
application-x-executable
view-list
edit-select-all
drive-harddisk
media-eject
document-properties
emblem-symbolic-link
utilities-terminal
```

ROX-Filer also checks for symbolic variants when appropriate.

The only bundled interface fallback image is:

```text
ROX-Filer/images/rox-show-hidden.png
```

For the **Hidden** toolbar action, ROX-Filer searches the active icon theme in
this order:

```text
cab_view
view-hidden-files
view-hidden-files-symbolic
view-reveal
view-reveal-symbolic
```

`cab_view` is preferred because it is widely included by Puppy Linux icon
themes and normally displays an eye. The bundled `rox-show-hidden.png` image is
used only when the active theme provides none of these icon names.

## Menu cleanup

Obsolete Help entries have been removed from the active interface, including:

- Help
- Show Help Files
- Manual

They were removed from the main contextual menus, AppDir menus, desktop and
panel menus, and the main toolbar.

The native GTK3 **About ROX-Filer** dialog remains directly available.

Historical documentation may remain in the source tree because it contains
important project history and original licensing information.

## About dialog

The About dialog is integrated directly into the GTK3 source.

It identifies the project as:

```text
ROX-Filer 2.12 GTK3
```

Credits are preserved as follows:

- Original author: Thomas Leonard
- Original contributors: ROX Desktop contributors
- GTK3 port and new features: josejp2424
- Maintainer of this GTK3 version: josejp2424

All original copyright and attribution notices remain in the source files.

## Interface languages

Included interface languages:

- English
- Arabic
- Catalan
- German
- Spanish
- French
- Italian
- Portuguese
- Japanese
- Hungarian
- Russian
- Chinese, simplified
- Chinese, traditional

The original upstream translation catalogues are preserved.

Arabic and Catalan currently cover the principal interface, menus, terminal
integration and file-operation dialogs. Their translation coverage may continue
to expand as development progresses.

Precompiled `.mo` files are included so the available translations can work on
systems where `msgfmt` is not installed.

ROX-Filer binds its translation domain directly to the AppDir path:

```text
ROX-Filer/Messages/<locale>/LC_MESSAGES/ROX-Filer.mo
```

When the AppDir is installed as `/usr/local/apps/ROX-Filer`, the catalogues
therefore remain under:

```text
/usr/local/apps/ROX-Filer/Messages
```

They do not need to be duplicated under `/usr/share/locale` unless the source is
changed to use a system-wide locale directory instead of the ROX AppDir.

## Build requirements

The build requires a C development environment and the development files for:

- GTK+ 3.22 or newer
- GLib and GObject
- GDK-Pixbuf
- Cairo
- libxml2
- X11
- X Session Management library (`sm`)
- Inter-Client Exchange library (`ice`)
- `pkg-config`
- GNU Autoconf tools when regenerating `configure`

The exact package names depend on the distribution.

### Optional runtime tools

- `rsync` enables the fast directory and batch-copy engine and cross-filesystem moves.
- A configured terminal emulator is used by the terminal integration.
- `python3` is used when running Python scripts in a terminal.
- `udisksctl` can be used for partition mounting in normal-user sessions.

The build system checks the main dependencies through `pkg-config`, including:

```sh
pkg-config --cflags --libs gtk+-3.0 libxml-2.0 sm ice
```

## Build and run

From the repository root:

```sh
cd ROX-Filer
./AppRun --compile
```

Run a new ROX-Filer instance with:

```sh
./AppRun -n
```

The complete sequence is:

```sh
cd ROX-Filer
./AppRun --compile
./AppRun -n
```

`AppRun --compile` removes the previous generated binary before rebuilding, which
helps prevent stale object files or an older binary from being reused.

## Clean rebuild

For a completely clean rebuild:

```sh
cd ROX-Filer

rm -rf build
rm -f ROX-Filer ROX-Filer.dbg

./AppRun --compile
./AppRun -n
```

Close an older running instance before testing a newly compiled version:

```sh
killall ROX-Filer 2>/dev/null
./AppRun -n
```

## Repository layout

Important files and directories include:

```text
.
├── README.md
├── CHANGELOG
├── LICENSE
├── ROX-Filer.svg
├── ROX-Filer/
│   ├── AppRun
│   ├── AppInfo.xml
│   ├── Options.xml
│   ├── Templates.ui
│   ├── images/
│   │   └── rox-show-hidden.png
│   ├── Messages/
│   ├── Help/
│   └── src/
└── .gitignore
```

The exact contents may evolve as the GTK3 port continues.

## Configuration

ROX-Filer stores user configuration under:

```text
~/.config/rox.sourceforge.net/ROX-Filer/
```

Common configuration files may include:

```text
Options
Settings.xml
```

The GTK3 widget and icon themes are controlled separately through the user's
GTK3 configuration.

## Compatibility

The primary target is GTK3 on X11/XLibre.

The source contains X11-specific integration using components such as:

- GDK X11
- GTK X11 embedding support
- Xlib
- X Session Management
- ICE

This is appropriate for Puppy Linux, Essora, JWM, EssoraWM and similar
lightweight X11/XLibre environments.

Native Wayland support is not currently a project goal.


## Complete directory contents

The normal filer view always displays every non-hidden item present in the
current directory. Folders are placed first and all other files follow in name
order. Historical type and glob filters are not applied to the normal view.

At the end of each directory scan, ROX-Filer compares the visible collection
against the directory's complete internal item table. If an item is missing,
duplicated or stale, the view is rebuilt from the authoritative directory
contents and its GTK3 layout is recalculated. This also ensures the vertical
scrollbar reaches the final row.

Hidden files remain controlled by the existing **Hidden** toolbar action.

## Partition detection

The permanent **Partitions** button uses the standard `drive-harddisk` icon and
combines three local sources:

- `lsblk` output
- Puppy runtime entries under `/tmp/pup_event_frontend/drive_*`
- Real partitions exposed by `/sys/class/block/*/partition`

This allows the button to list the same drives visible on a Puppy desktop even
when an older `lsblk` build omits newer columns. Duplicate devices and technical
loop, RAM, Puppy layer and swap devices are filtered.

## Known limitations

- Additional testing is still useful across different GTK3 themes and icon
  themes.
- Arabic and Catalan translation coverage is not yet as extensive as every
  historical upstream catalogue.
- Some deprecated GTK3 APIs may remain, although they are still available in
  GTK3 and do not introduce a GTK2 dependency.
- Behaviour can vary depending on the external terminal emulator and desktop
  environment.
- X11/XLibre-specific desktop and panel functionality is not expected to work
  natively on Wayland.

## Reporting issues

When reporting a problem, include:

- Distribution and version
- Desktop or window manager
- X11 or XLibre version
- GTK3 version
- Steps needed to reproduce the problem
- Console output
- Relevant log files
- A screenshot when the issue is visual

For input or event problems, running ROX-Filer from a terminal can provide useful
diagnostics:

```sh
ROX_TRACE_INPUT=1 GDK_SYNCHRONIZE=1 ./AppRun -n
```

## Contributing

Contributions, testing reports and translations are welcome.

Changes should:

- Preserve the lightweight character of ROX-Filer.
- Remain compatible with GTK3.
- Avoid adding unnecessary dependencies.
- Preserve original copyright notices.
- Follow the existing source style where practical.
- Be tested on X11 or XLibre.
- Include translation updates for newly added interface strings when possible.

## Changelog

See the consolidated file:

```text
CHANGELOG
```

It contains the development history of the GTK3 port and the fixes introduced
during the conversion.

## Credits

### Original project

ROX-Filer was originally created by **Thomas Leonard** for the ROX Desktop.

The project also includes work from the original ROX Desktop contributors.
Their copyright and attribution notices remain preserved throughout the source.

### GTK3 version

- GTK3 port: **josejp2424**
- New GTK3 integration and features: **josejp2424**
- Maintainer of this version: **josejp2424**

## License

This modified GTK3 version of ROX-Filer is distributed under the:

```text
GNU General Public License, version 3 or, at your option, any later version
```

SPDX license identifier:

```text
GPL-3.0-or-later
```

See the root license file:

```text
LICENSE
```

The original ROX-Filer source was distributed under GPL-2.0-or-later. That
licensing permits this modified version to be distributed under
GPL-3.0-or-later.

All original copyright, authorship and licensing notices are retained.

Files containing third-party code may retain their own compatible copyright and
license notices. Those file-specific notices remain applicable to those files.

---

<p align="center">
  <strong>ROX-Filer 2.12 GTK3</strong><br>
  Classic ROX-Filer simplicity, adapted for modern GTK3 desktops.
</p>
