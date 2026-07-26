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

This repository contains the ongoing port of **ROX-Filer 2.12 to GTK3**.

The goal of this project is to preserve the speed, simplicity, flexibility and
traditional behaviour of the original ROX-Filer while replacing its GTK2-era
implementation with a modern GTK3 code base.

The main filer interface, file views, menus, preferences, file operations,
input handling, GTK theme integration, icon-theme support and dialogs have
already been adapted to GTK3.

This version is intended primarily for lightweight X11 and XLibre desktops,
including:

- Puppy Linux
- EssoraPup
- Essora
- JWM
- EssoraWM
- Other lightweight X11/XLibre environments

> ROX-Filer GTK3 currently targets X11/XLibre. It is not yet a native Wayland
> port. Basic use may work through XWayland, but the pinboard, panels and other
> desktop functions still depend on X11-specific APIs.

## Project goals

- Preserve the classic ROX-Filer workflow.
- Maintain fast startup and low resource usage.
- Remove the normal runtime dependency on GTK2.
- Integrate correctly with GTK3 themes and icon themes.
- Improve behaviour on modern Puppy Linux and Essora systems.
- Preserve all original copyright and contributor notices.
- Add practical desktop features without making the filer unnecessarily heavy.

## GTK3 status

The project builds against:

```text
GTK+ 3.22 or newer
```

The normal build uses GTK3 and does not require GTK2.

Major GTK3 porting work includes:

- GTK3 widgets and containers.
- GTK3 event and mouse handling.
- Cairo-based custom drawing.
- `GtkStyleContext` theme integration.
- GTK3-compatible menus.
- GTK3-compatible scroll adjustments and file views.
- GTK3-compatible drag and drop.
- GTK3 preferences and dialogs.
- GTK3 icon-theme loading.
- Native GTK3 About dialog.
- Replacement of removed GTK2 menu infrastructure.
- X11/XLibre compatibility through GDK X11 and Xlib.

## Main features

- Fast and lightweight graphical file manager.
- Classic ROX-Filer icon and detailed list views.
- Desktop pinboard support.
- Panel support.
- Drag and drop.
- Copy, move, rename and delete operations.
- File associations and MIME handling.
- AppDir support.
- Mount and unmount integration.
- Symbolic-link handling.
- Extended-attribute support.
- Per-directory display settings.
- GTK3 theme and icon-theme integration.
- Multilingual interface.
- Integrated terminal actions.
- Permanent partition toolbar.
- Native GTK3 About dialog.

## Complete directory view

The GTK3 port fixes a scrolling problem that prevented users from reaching all
files in directories containing hundreds or thousands of entries.

ROX-Filer was loading the complete directory, but `GtkViewport` was calculating
the scrollable area from the minimum widget height instead of the full natural
height of the collection.

The vertical view now uses the complete natural height, allowing access to every
visible item in the directory.

The default presentation is:

1. Directories first.
2. All remaining visible files afterwards.
3. Alphabetical order inside each group.

ROX-Filer displays archives, AppImages, scripts, binaries, images, documents and
all other visible file types.

Hidden files remain hidden unless the user enables **Show Hidden**.

## Partitions toolbar

ROX-Filer GTK3 includes a permanent **Partitions** button in the main toolbar.

The button:

- Is always displayed first, before the **Up** button.
- Cannot be hidden, removed or reordered through toolbar customization.
- Uses the active icon theme's standard `drive-harddisk` icon.
- Detects usable disk partitions.
- Shows the partition label, device name, size and mount state.
- Mounts an unmounted partition before opening it.
- Opens mounted partitions directly in the current ROX-Filer window.
- Displays partitions in a four-column grid.
- Continues on additional rows when more than four partitions are available.

Partition detection follows the filtering behaviour used by EssoraWM, avoiding
technical devices such as loop, RAM, swap and internal Puppy layer devices.

When available, ROX-Filer also respects:

```text
~/.config/essorawm/hidden-drives
```

<p align="center">
  <img src="screenshot/rox-particiones.png"
       alt="ROX-Filer GTK3 partitions toolbar and partition grid"
       width="760">
</p>

<p align="center">
  <em>Permanent partition access from the ROX-Filer toolbar.</em>
</p>

## Window behaviour

Normal ROX-Filer windows use:

```text
640 × 400 pixels
```

as their default and minimum usable size.

This prevents filer windows and dialogs from opening as very small or unusually
wide single-row windows.

Saved geometries smaller than the minimum are adjusted to a usable size.

Special desktop surfaces such as the pinboard, panels, menus and tooltips are
not forced to use the normal filer-window size.

## Preferences window

The Preferences window uses a minimum usable size of:

```text
640 × 400 pixels
```

Large option pages are placed inside scrollable GTK3 containers so their natural
size cannot expand the dialog beyond the available screen area.

The category panel has its own compact scrollable area.

## Terminal integration

ROX-Filer GTK3 adds two file-context-menu actions.

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

The command uses safely separated arguments, supports paths containing spaces
and keeps the terminal open until the user presses Enter.

The terminal command remains controlled by ROX-Filer's existing:

```text
menu_xterm
```

option.

Terminal entries use the standard icon-theme name:

```text
utilities-terminal
```

Depending on the selected file, terminal execution may use:

- The configured terminal emulator, such as `xterm`
- `/bin/sh`
- `python3`

AppImage files must have executable permission before they can be launched.

## File-operation dialog fixes

The GTK3 port fixes progress dialogs that previously remained open after a file
operation had already completed.

The child-process communication now processes all pending messages before
handling pipe closure.

This applies to:

- Copying files
- Replacing existing files
- Moving files
- Deleting files
- Deleting directories recursively

A progress dialog closes after a successful operation and remains open only
when an actual error must be shown.

## GTK3 theme integration

ROX-Filer uses the active GTK3 widget theme and icon theme.

GTK3 normally reads the user configuration from:

```text
~/.config/gtk-3.0/settings.ini
```

Example:

```ini
[Settings]
gtk-theme-name=YourGtkTheme
gtk-icon-theme-name=YourIconTheme
gtk-font-name=Sans 10
```

This port does not force a fixed filer background or text colour.

## System icon theme

ROX-Filer GTK3 uses the active system icon theme for interface graphics such as:

- Toolbar actions
- Directories
- Applications and AppDirs
- List view
- Selection actions
- Mounted devices
- Unmount and eject actions
- Symbolic links
- Extended attributes
- Iconified windows
- Terminal actions
- Partition toolbar

Standard icon names include:

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

Normal directories now request the standard `folder` icon from the active GTK3
icon theme instead of using the legacy Puppy `folder48.png` MIME icon.

Custom `.DirIcon` files and AppDir icons continue to take priority.

The only bundled interface fallback image is:

```text
ROX-Filer/images/rox-show-hidden.png
```

ROX-Filer first tries:

```text
view-hidden-files
```

and uses the bundled image only when the active icon theme does not provide a
suitable icon.

## Menu cleanup

Obsolete Help entries were removed from the active interface:

- Help
- Show Help Files
- Manual

They were removed from contextual menus, AppDir menus, desktop and panel menus,
and the main toolbar.

The native GTK3 **About ROX-Filer** dialog remains directly available.

Historical documentation remains in the source where required to preserve
project history and original licensing information.

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
integration, partition integration and file-operation dialogs. Their coverage
may continue to expand.

Precompiled `.mo` files are included so translations can work on systems where
`msgfmt` is not installed.

## Build requirements

The build requires a C development environment and development files for:

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

The build system checks its primary dependencies through:

```sh
pkg-config --cflags --libs gtk+-3.0 libxml-2.0 sm ice
```

Optional runtime tools include:

- `file`
- `xterm` or another configured terminal emulator
- `udisksctl`, normally provided by `udisks2`
- librsvg/GDK-Pixbuf SVG loader support

## Build and run

From the repository root:

```sh
cd ROX-Filer
./AppRun --compile
./AppRun -n
```

`AppRun --compile` removes the previous generated binary before rebuilding.

## Clean rebuild

For a completely clean rebuild:

```sh
cd ROX-Filer

rm -rf build
rm -f ROX-Filer ROX-Filer.dbg

./AppRun --compile
```

Close any older running instance before testing the newly compiled binary:

```sh
killall ROX-Filer 2>/dev/null
./AppRun -n
```

ROX-Filer uses a single running instance. If an older binary remains active,
new windows may still be created by that older process.

## Repository layout

```text
.
├── README.md
├── CHANGELOG
├── LICENSE
├── ROX-Filer.svg
├── screenshot/
│   └── rox-particiones.png
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

## Configuration

ROX-Filer stores user configuration under:

```text
~/.config/rox.sourceforge.net/ROX-Filer/
```

Common files include:

```text
Options
Settings.xml
```

GTK3 widget and icon themes are controlled through the user's GTK3 settings.

## Compatibility

The primary target is GTK3 on X11/XLibre.

The source contains X11-specific integration using:

- GDK X11
- GTK X11 embedding support
- Xlib
- X Session Management
- ICE

This is appropriate for Puppy Linux, Essora, JWM, EssoraWM and similar
lightweight X11/XLibre environments.

Native Wayland support is not currently included. A future port is technically
possible, but the pinboard, panels, exact window positioning and desktop
integration would require substantial redesign.

## Known limitations

- Additional testing remains useful across different GTK3 and icon themes.
- Arabic and Catalan translation coverage is not yet as extensive as every
  historical upstream catalogue.
- Some deprecated but still supported GTK3 APIs may remain.
- Behaviour can vary depending on the external terminal emulator.
- X11/XLibre-specific desktop and panel functionality is not expected to work
  natively on Wayland.
- SVG preview support depends on the system GDK-Pixbuf/librsvg loader.

## Reporting issues

When reporting a problem, include:

- Distribution and version
- Desktop or window manager
- X11 or XLibre version
- GTK3 version
- Steps needed to reproduce the problem
- Console output
- Relevant log files
- A screenshot when the problem is visual

For input or event diagnostics:

```sh
ROX_TRACE_INPUT=1 GDK_SYNCHRONIZE=1 ./AppRun -n
```

## Contributing

Contributions, test reports and translations are welcome.

Changes should:

- Preserve the lightweight character of ROX-Filer.
- Remain compatible with GTK3.
- Avoid unnecessary dependencies.
- Preserve original copyright notices.
- Follow the existing source style where practical.
- Be tested on X11 or XLibre.
- Include translation updates for new interface strings when possible.

## Changelog

See the consolidated file:

```text
CHANGELOG
```

It contains the development history of the GTK3 port and all fixes introduced
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

SPDX identifier:

```text
GPL-3.0-or-later
```

See:

```text
LICENSE
```

The original ROX-Filer source was distributed under GPL-2.0-or-later. That
license permits this modified version to be distributed under
GPL-3.0-or-later.

All original copyright, authorship and licensing notices are retained.

Files containing third-party code may retain their own compatible copyright and
license notices. Those file-specific notices remain applicable.

---

<p align="center">
  <strong>ROX-Filer 2.12 GTK3</strong><br>
  Classic ROX-Filer simplicity, adapted for modern GTK3 desktops.
</p>
