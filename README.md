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

Overview

ROX-Filer is a fast and lightweight graphical file manager originally createdby Thomas Leonard for the ROX Desktop.

This repository contains the ongoing port of ROX-Filer 2.12 to GTK3.

The goal of this project is to preserve the speed, simplicity, flexibility andtraditional behaviour of the original ROX-Filer while replacing its GTK2-eraimplementation with a modern GTK3 code base.

The main filer interface, file views, menus, preferences, file operations,input handling, GTK theme integration, icon-theme support and dialogs havealready been adapted to GTK3. Development may continue with additional testing,cleanup, translation work and compatibility improvements.

This version is intended primarily for lightweight X11 and XLibre desktops,including:

Puppy Linux

EssoraPup

Essora

JWM

EssoraWM

Other lightweight X11/XLibre environments

ROX-Filer GTK3 currently targets X11/XLibre. It is not a native Wayland port.

Project goals

Preserve the classic ROX-Filer workflow.

Maintain fast startup and low resource usage.

Remove the dependency on GTK2.

Integrate correctly with GTK3 themes and icon themes.

Improve behaviour on modern Puppy Linux and Essora systems.

Keep the source understandable and maintainable.

Preserve all original copyright and contributor notices.

Add practical desktop features without making the filer unnecessarily heavy.

Current GTK3 status

The project builds against:

GTK+ 3.22 or newer

The source uses GTK3 for the main interface and no longer requires GTK2 for thenormal build.

Major GTK3 porting work includes:

GTK3 widgets and containers.

GTK3 event handling.

Cairo-based custom drawing.

GtkStyleContext theme integration.

GTK3-compatible menus.

GTK3-compatible scroll adjustments and file views.

GTK3-compatible drag-and-drop handling.

GTK3 preferences and dialogs.

GTK3 icon-theme loading.

Native GTK3 About dialog.

Replacement of removed GTK2 menu infrastructure.

Compatibility with X11/XLibre through GDK X11 and Xlib.

Main features

Fast and lightweight graphical file manager.

Classic ROX-Filer icon and list views.

Desktop pinboard support.

Panel support.

Drag and drop.

File copy, move, rename and delete operations.

File type handling and application associations.

AppDir support.

Mount and unmount integration.

Symbolic-link handling.

Extended-attribute support.

Per-directory display settings.

Configurable toolbar.

GTK3 theme and icon-theme integration.

Multilingual interface.

Integrated terminal actions.

Native GTK3 About dialog.

Window defaults

New filer windows use a real initial size of:

600 × 400 pixels

This size is applied after the initial GTK3 window mapping so that the contentlayout cannot stretch a new window into a single wide row.

A saved per-directory geometry continues to take priority when one exists.

The window remains fully resizable and the default size is not enforced as aminimum size.

Preferences window

The Preferences window uses a default size of:

600 × 400 pixels

Large settings pages are placed inside scrollable GTK3 containers. This preventstheir natural size from expanding the dialog beyond the available screen area.

The category list also has its own compact scrollable panel.

Terminal integration

ROX-Filer GTK3 adds two practical file-context-menu actions.

Open Terminal Here

When a directory is selected, the contextual menu provides:

Open Terminal Here

The configured terminal emulator opens with the selected directory as itsworking directory.

Run in Terminal

The contextual menu provides:

Run in Terminal

for supported files, including:

Executable native binaries

Executable shell scripts

Files ending in .sh

Python scripts ending in .py

Python GUI scripts ending in .pyw

Executable AppImage files

Other executable files with a valid interpreter line

The command is launched with safely separated arguments, supports pathscontaining spaces and keeps the terminal open until the user presses Enter.

The terminal command remains controlled by ROX-Filer's existing:

menu_xterm

option.

The terminal menu entries use the standard icon-theme name:

utilities-terminal

Optional runtime tools

Depending on the selected file, the terminal integration may use:

A configured terminal emulator, such as xterm

/bin/sh

python3

AppImage files must have executable permission before they can be launched.

File-operation dialog fixes

The GTK3 port includes corrections for file-operation dialogs that previouslyremained visible after an operation had already completed.

The child-process communication now handles pending input correctly when GLibreports input and pipe closure at the same time.

This applies to operations such as:

Copying files

Replacing existing files

Moving files

Deleting files

Deleting directories recursively

The progress dialog closes after a successful operation and remains open onlywhen an actual error must be shown to the user.

GTK3 theme integration

ROX-Filer uses the active GTK3 settings, widget theme and icon theme.

GTK3 normally reads the user's configuration from:

~/.config/gtk-3.0/settings.ini

For example:

[Settings]
gtk-theme-name=YourGtkTheme
gtk-icon-theme-name=YourIconTheme
gtk-font-name=Sans 10

This port does not force a fixed filer background or text colour. File views usestandard GTK3 styling so that the selected system theme controls theirappearance.

System icon theme

ROX-Filer GTK3 uses the active system icon theme for interface graphics such as:

Toolbar actions

Folders

Applications and AppDirs

List view

Selection actions

Mounted devices

Unmount and eject actions

Symbolic links

Extended attributes

Iconified windows

Terminal actions

Examples of standard icon names used by the application include:

folder
application-x-executable
view-list
edit-select-all
drive-harddisk
media-eject
document-properties
emblem-symbolic-link
utilities-terminal

ROX-Filer also checks for symbolic variants when appropriate.

The only bundled interface fallback image is:

ROX-Filer/images/rox-show-hidden.png

ROX-Filer first tries the theme icon:

view-hidden-files

and uses the bundled image only when the active icon theme does not provide asuitable icon.

Menu cleanup

Obsolete Help entries have been removed from the active interface, including:

Help

Show Help Files

Manual

They were removed from the main contextual menus, AppDir menus, desktop andpanel menus, and the main toolbar.

The native GTK3 About ROX-Filer dialog remains directly available.

Historical documentation may remain in the source tree because it containsimportant project history and original licensing information.

About dialog

The About dialog is integrated directly into the GTK3 source.

It identifies the project as:

ROX-Filer 2.12 GTK3

Credits are preserved as follows:

Original author: Thomas Leonard

Original contributors: ROX Desktop contributors

GTK3 port and new features: josejp2424

Maintainer of this GTK3 version: josejp2424

All original copyright and attribution notices remain in the source files.

Interface languages

Included interface languages:

English

Arabic

Catalan

German

Spanish

French

Italian

Portuguese

Japanese

Hungarian

Russian

Chinese, simplified

Chinese, traditional

The original upstream translation catalogues are preserved.

Arabic and Catalan currently cover the principal interface, menus, terminalintegration and file-operation dialogs. Their translation coverage may continueto expand as development progresses.

Precompiled .mo files are included so the available translations can work onsystems where msgfmt is not installed.

Build requirements

The build requires a C development environment and the development files for:

GTK+ 3.22 or newer

GLib and GObject

GDK-Pixbuf

Cairo

libxml2

X11

X Session Management library (sm)

Inter-Client Exchange library (ice)

pkg-config

GNU Autoconf tools when regenerating configure

The exact package names depend on the distribution.

The build system checks the main dependencies through pkg-config, including:

pkg-config --cflags --libs gtk+-3.0 libxml-2.0 sm ice

Build and run

From the repository root:

cd ROX-Filer
./AppRun --compile

Run a new ROX-Filer instance with:

./AppRun -n

The complete sequence is:

cd ROX-Filer
./AppRun --compile
./AppRun -n

AppRun --compile removes the previous generated binary before rebuilding, whichhelps prevent stale object files or an older binary from being reused.

Clean rebuild

For a completely clean rebuild:

cd ROX-Filer

rm -rf build
rm -f ROX-Filer ROX-Filer.dbg

./AppRun --compile
./AppRun -n

Close an older running instance before testing a newly compiled version:

killall ROX-Filer 2>/dev/null
./AppRun -n

Repository layout

Important files and directories include:

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

The exact contents may evolve as the GTK3 port continues.

Configuration

ROX-Filer stores user configuration under:

~/.config/rox.sourceforge.net/ROX-Filer/

Common configuration files may include:

Options
Settings.xml

The GTK3 widget and icon themes are controlled separately through the user'sGTK3 configuration.

Compatibility

The primary target is GTK3 on X11/XLibre.

The source contains X11-specific integration using components such as:

GDK X11

GTK X11 embedding support

Xlib

X Session Management

ICE

This is appropriate for Puppy Linux, Essora, JWM, EssoraWM and similarlightweight X11/XLibre environments.

Native Wayland support is not currently a project goal.

Known limitations

Additional testing is still useful across different GTK3 themes and iconthemes.

Arabic and Catalan translation coverage is not yet as extensive as everyhistorical upstream catalogue.

Some deprecated GTK3 APIs may remain, although they are still available inGTK3 and do not introduce a GTK2 dependency.

Behaviour can vary depending on the external terminal emulator and desktopenvironment.

X11/XLibre-specific desktop and panel functionality is not expected to worknatively on Wayland.

Reporting issues

When reporting a problem, include:

Distribution and version

Desktop or window manager

X11 or XLibre version

GTK3 version

Steps needed to reproduce the problem

Console output

Relevant log files

A screenshot when the issue is visual

For input or event problems, running ROX-Filer from a terminal can provide usefuldiagnostics:

ROX_TRACE_INPUT=1 GDK_SYNCHRONIZE=1 ./AppRun -n

Contributing

Contributions, testing reports and translations are welcome.

Changes should:

Preserve the lightweight character of ROX-Filer.

Remain compatible with GTK3.

Avoid adding unnecessary dependencies.

Preserve original copyright notices.

Follow the existing source style where practical.

Be tested on X11 or XLibre.

Include translation updates for newly added interface strings when possible.

Changelog

See the consolidated file:

CHANGELOG

It contains the development history of the GTK3 port and the fixes introducedduring the conversion.

Credits

Original project

ROX-Filer was originally created by Thomas Leonard for the ROX Desktop.

The project also includes work from the original ROX Desktop contributors.Their copyright and attribution notices remain preserved throughout the source.
