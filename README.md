<p align="center">
  <img src="ROX-Filer.svg" alt="ROX-Filer GTK3 logo" width="180">
</p>

<h1 align="center">ROX-Filer 2.12 GTK3</h1>

<p align="center">
  A fast and lightweight GTK3 file manager and desktop for Puppy Linux,
  Essora and other lightweight X11/XLibre environments.
</p>

<p align="center">
  Classic ROX-Filer simplicity, modern GTK3 integration and an optional desktop mode.
</p>

<p align="center">
  <img src="screenshot/rox-desktop-demo-slow.gif"
       alt="ROX-Filer GTK3 desktop demonstration">
</p>

Overview

ROX-Filer is a fast and lightweight graphical file manager originally createdby Thomas Leonard for the ROX Desktop.

This repository contains an ongoing GTK3 development line based on theROX-Filer 2.11 source used by Puppy Linux Woof-CE.

The 2.12 GTK3 name identifies this maintained fork. It does not refer to aseparate upstream ROX-Filer 2.12 release.

The project preserves the traditional ROX-Filer workflow while modernising theGTK2-era implementation and adding practical desktop features for current PuppyLinux, EssoraPup and Essora systems.

The primary targets are:

Puppy Linux

EssoraPup

Essora

JWM

EssoraWM

Other lightweight X11 and XLibre environments

ROX-Filer GTK3 currently targets X11/XLibre. It is not yet a native Waylanddesktop or file manager.

Project goals

Preserve the classic ROX-Filer workflow.

Keep startup fast and resource use low.

Remove the normal runtime dependency on GTK2.

Integrate correctly with GTK3 widget and icon themes.

Use XDG and Freedesktop standards where practical.

Improve behaviour on Puppy Linux, EssoraPup and Essora.

Keep the source understandable and maintainable.

Preserve original copyright and contributor notices.

Add useful desktop features without turning ROX-Filer into a heavy desktopenvironment.

Main capabilities

GTK3 file-manager interface.

Classic ROX-Filer icon and detailed list views.

Optional ROX Desktop mode.

Files and launchers from ~/Desktop.

Wallpaper and desktop-application managers.

Correct device icons for internal, removable, optical and flash storage.

Configurable desktop drive layout.

Standard Freedesktop Trash.

XDG application associations.

Standard GTK icon-theme integration.

Faster rsync-assisted copy and move operations.

Improved copy, move and permanent-delete progress dialogs.

Multiple desktop-item selection and movement.

Built-in file templates.

Centred normal windows and dialogs.

Classic ROX pinboard and panel compatibility.

Debian and portable package generation.

Standard /usr/bin/ROX-Filer launcher.

Quick start

Build ROX-Filer:

cd ROX-Filer
./AppRun --compile

Run a separate file-manager instance:

./AppRun -n

Start ROX Desktop:

ROX-Filer --desktop

Open the wallpaper manager:

ROX-Filer --desktop-wallpaper

Open the desktop application manager:

ROX-Filer --desktop-apps

Current GTK3 status

The project builds against:

GTK+ 3.22 or newer

The normal build uses GTK3 and no longer requires GTK2.

The GTK3 port includes:

GTK3 widgets and containers.

GTK3 event handling.

Cairo-based custom drawing where still required.

GtkStyleContext integration.

GTK3-compatible menus and toolbars.

GTK3-compatible scrolling and file views.

GTK3-compatible drag and drop.

GTK3 preferences and dialogs.

GTK3 icon-theme loading.

Native GTK3 About dialog.

Replacement of removed GTK2 menu infrastructure.

X11/XLibre integration through GDK X11 and Xlib.

Some historical compatibility code and deprecated-but-still-supported GTK2 APIsmay remain. They can be cleaned up gradually without introducing a GTK2dependency.

File manager

Main file-manager features

Fast and lightweight graphical file manager.

Classic ROX-Filer icon and detailed list views.

Folders-first ordering.

Complete non-hidden directory contents.

Drag and drop.

AppDir support.

Symbolic-link handling.

Extended-attribute support.

Per-directory display settings.

Configurable toolbar.

Per-window Back and Forward history.

Permanent Partitions button.

Mount, unmount, eject and open support.

Fast rsync-assisted copy and move operations.

Standard Freedesktop Trash through GIO.

Separate permanent deletion.

XDG default-application handling.

Standard GTK3 widget and icon themes.

Multilingual interface.

Integrated terminal actions.

Built-in file templates.

Native GTK3 About dialog.

Window defaults

New filer windows use an initial size of:

640 × 400 pixels

The size is applied after the initial GTK mapping so the content layout cannotstretch a new window into a single wide row.

A saved per-directory geometry still takes priority. Older saved sizes areclamped so a normal filer window cannot open below 640 × 400 pixels.

The window remains fully resizable.

Centred and square dialogs

Normal ROX-Filer dialogs are centred using the usable monitor area rather thanthe complete screen rectangle.

This includes:

Rename

Delete

Permanent delete

Properties

Create directory

Create symbolic link

Preferences

Bulk rename

Icon editing

Confirmation dialogs

Wallpaper and desktop-management windows

The usable-area calculation takes the desktop panel into account so dialogbuttons are not hidden behind it.

Menus and normal dialogs use square corners. This avoids black corner artefactsthat may appear with rounded popup windows on XLibre, systems without acompositor or themes that use transparent rounded surfaces.

GTK3 still controls colours, text, selection, spacing, typography and icons.

Preferences window

The Preferences window uses a default size of:

640 × 400 pixels

Large pages are placed inside scrollable GTK3 containers. The category listalso uses its own compact scrollable panel.

Old ROX-specific colour controls were removed. File-view colours come from theactive GTK3 theme.

Back and Forward navigation

Each filer window keeps its own lightweight directory history.

Alt+Left   Back
Alt+Right  Forward

Opening a new directory after navigating backwards clears the Forward branch.History is limited to 100 paths per window.

Complete directory contents

The normal filer view displays every non-hidden item in the current directory.

Folders are placed first. Other files follow in stable name order.

Historical type filters, saved glob filters and directories-only/files-onlystates are not applied to the normal view. This prevents archives, AppImages,scripts and other regular files from appearing to be missing.

At the end of a scan, ROX-Filer compares the visible collection with thedirectory's internal item table. If an item is missing, duplicated or stale, theview is rebuilt and the GTK3 layout is recalculated.

Hidden files remain controlled by the Hidden toolbar action.

Permanent Partitions button

The main toolbar includes a permanent Partitions button.

It is outside the configurable toolbar-item list and cannot be removed from thetoolbar preferences.

The partition view combines information from:

lsblk

Puppy runtime entries under /tmp/pup_event_frontend/drive_*

Real partitions under /sys/class/block/*/partition

Technical devices are filtered, including:

Loop devices

ZRAM and RAM devices

Device-mapper helper devices

Puppy runtime layers

SquashFS, overlay and AUFS layers

Swap volumes

EFI system partitions

ROX-Filer first requests the complete modern lsblk column set and retries witha smaller compatible set when older Puppy Linux versions do not provide everycolumn.

Mounted devices open directly.

Unmounted devices are mounted before opening:

Puppy/root sessions use /bin/mount under /mnt/<device>.

Normal-user sessions may use udisksctl mount -b.

Right-clicking a partition provides:

Open

Mount

Unmount

Eject

Removable media can be unmounted and safely powered off with udisksctl, witheject as a fallback.

<p align="center">
  <img src="screenshot/rox-particiones.png" alt="ROX-Filer partition browser">
</p>

Correct device icons

ROX-Filer identifies the real device type before requesting an icon from theactive icon theme.

Examples include:

drive-harddisk
drive-harddisk-solidstate
drive-removable-media
media-flash
media-cdrw
drive-network
media-floppy

Recognised devices include:

Internal hard drives

SSD and NVMe drives

USB flash drives

SD and MMC cards

Optical devices such as sr0

Floppy devices

Network drives

ROX-Filer does not scan unrelated installed icon themes and does not forceGNOME icon files. GTK3 resolves the semantic icon name through the active icontheme configured by the user.

Fast rsync copy and move engine

ROX-Filer GTK3 includes a hybrid file-operation engine for faster local copies.

The historical engine started an external cp process for each regular file.That is reliable but slow for directories containing hundreds or thousands ofsmall files.

When rsync is available, the project can use:

One rsync process for a complete directory tree.

One rsync batch for compatible multiple selections.

rename() or mv for moves on the same filesystem.

rsync --remove-source-files for cross-filesystem moves and directory merges.

--partial to preserve incomplete transfer data after interruption.

ROX-Filer never adds --delete to normal copy or move operations, so unrelateddestination files are not removed.

When rsync is unavailable, ROX-Filer falls back to the classic copy and moveengine.

Conflict policy

When destination items already exist, one conflict-policy dialog is shown.

Available choices:

Ask for each conflict.

Replace existing files.

Skip existing files.

Replace only when the source is newer.

Ask for each conflict is the safe default.

The comparison dialog can apply one Replace or Skip choice to all remainingconflicts in the current operation.

The selected policy is reset before the next copy or move.

Improved operation dialogs

Copy, move and permanent-delete operations use clearer GTK3 progress windows.

The interface can show:

The current operation.

Source and destination information.

Visual progress.

Error details when needed.

Cancel and decision controls.

The dialogs keep a compact ROX-style workflow while presenting progress in aform familiar to users of current graphical file managers.

The child-process communication handles pending input correctly when GLibreports data and pipe closure at the same time. Successful operations closetheir progress window automatically.

Standard Trash

The normal Delete key moves selected items to the standard FreedesktopTrash.

ROX-Filer uses GIO so each filesystem can select the correct Trash location andwrite the metadata required for restoring files.

The standard user Trash is normally:

~/.local/share/Trash/files
~/.local/share/Trash/info

Available actions include:

Open Trash

Move selected items to Trash

Restore selected items

Empty Trash

Permanently delete with Shift+Delete

The Trash is available:

In the filer toolbar.

In contextual menus.

As an optional desktop icon.

Filesystems without Trash support show an error. ROX-Filer does not silentlyconvert a failed Trash operation into permanent deletion.

Permanent deletion remains a separate operation and asks once for the completeselection.

XDG file associations

ROX-Filer uses the standard XDG/GIO application-association system.

The primary user configuration is:

~/.config/mimeapps.list

Applications selected as default in another XDG-compatible file manager, suchas Thunar or PCManFM, can therefore also be recognised by ROX-Filer.

The old ROX-specific association directories are no longer used to decide thedefault application:

~/Choices/MIME-types
~/.config/rox.sourceforge.net/MIME-types

The Set Default Application action uses GIO to store the standard defaultapplication for the selected MIME type.

Standard MIME icons

ROX-Filer requests standard MIME icon names from the active icon theme.

Puppy-specific MIME icons can be installed into the standard hicolor theme:

application-pet
application-x-sfs
application-x-squashfs-image

The Debian package can install them under:

/usr/share/icons/hicolor/16x16/mimetypes
/usr/share/icons/hicolor/24x24/mimetypes
/usr/share/icons/hicolor/48x48/mimetypes
/usr/share/icons/hicolor/scalable/mimetypes

The post-installation script creates missing mimetypes directories andrefreshes the icon cache when gtk-update-icon-cache is available.

This makes the icons available to ROX-Filer and other applications that followthe Freedesktop icon-theme standard.

GTK3 theme integration

GTK3 reads the user's theme configuration from:

~/.config/gtk-3.0/settings.ini

For example:

[Settings]
gtk-theme-name=YourGtkTheme
gtk-icon-theme-name=YourIconTheme
gtk-font-name=Sans 10

ROX-Filer does not force fixed file-view background or text colours.

The active GTK3 theme controls:

File-view background

Normal text

Selected text

Selection background

Disabled controls

Menus

Dialogs

Toolbar buttons

Fonts and spacing

System icon theme

ROX-Filer uses standard semantic icon names for:

Toolbar actions

Folders

Applications

AppDirs

List view

Selection actions

Devices

Unmount and eject actions

Symbolic links

Extended attributes

Terminal actions

Wallpaper management

Desktop application management

Trash

Examples include:

folder
application-x-executable
view-list
edit-select-all
drive-harddisk
drive-removable-media
media-eject
document-properties
emblem-symbolic-link
utilities-terminal
preferences-desktop-wallpaper
applications-other
user-trash
user-trash-full

The bundled ROX-Filer/images/rox-show-hidden.png image is used only when theactive theme does not provide a supported Hidden-action icon.

Terminal integration

When a directory is selected, the contextual menu provides:

Open Terminal Here

For supported executable files, it provides:

Run in Terminal

Supported files include:

Native executable binaries

Executable shell scripts

.sh files

.py files

.pyw files

Executable AppImages

Files with a valid interpreter line

Arguments are safely separated and paths containing spaces are supported.

The configured terminal is controlled by ROX-Filer's existing:

menu_xterm

option.

The standard icon is:

utilities-terminal

New menu and built-in templates

The toolbar and context menu include a New submenu.

It can create:

A directory

A blank file

A shell script

A text file

A web page

A Python file

User-defined templates

Bundled templates are stored in:

ROX-Filer/Templates/

User templates can be stored in:

~/.config/rox.sourceforge.net/Templates/

When a user template and a bundled template have the same name, the usertemplate takes priority.

ROX Desktop

Starting the desktop

Start ROX Desktop with:

ROX-Filer --desktop

This mode manages:

Wallpaper

Files and launchers from ~/Desktop

Drive and partition icons

Desktop icon positions

Standard Trash

Desktop contextual menus

Desktop refresh operations

The /usr/bin/ROX-Filer launcher provides a standard executable path for thismode and for the normal filer.

Desktop files and applications

The new desktop always uses:

~/Desktop

It does not create or switch to translated directory names such as~/Escritorio.

Files, folders and .desktop launchers placed in ~/Desktop appear on thedesktop.

Supported interactions include:

Double-click to launch or open.

Drag to move an icon.

Persistent manual positions.

Multiple selection with Ctrl, Shift or Ctrl+Alt.

Group movement of selected icons.

Delete to move selected items to Trash.

Enter to open selected items.

Contextual actions for one or multiple items.

Positions are stored in:

~/.config/rox.sourceforge.net/ROX-Filer/desktop-positions.conf

Desktop preferences are stored in:

~/.config/rox.sourceforge.net/ROX-Filer/desktop.conf

Desktop application manager

Open the application manager with:

ROX-Filer --desktop-apps

The manager allows applications to be added to or removed from ~/Desktop.

A menu entry can use:

Exec=ROX-Filer --desktop-apps
Icon=applications-other

Wallpaper manager

Open the wallpaper manager with:

ROX-Filer --desktop-wallpaper

The manager supports:

Browsing /usr/share/backgrounds.

Choosing another image.

Wallpaper fill styles.

Applying several wallpapers without closing the selector.

Refreshing the desktop after the wallpaper changes.

Restart-aware refresh for wbar and desktop work-area changes.

A menu entry can use:

Exec=ROX-Filer --desktop-wallpaper
Icon=preferences-desktop-wallpaper

The selector remains open after Apply, allowing the user to continue testingdifferent wallpapers.

Desktop drive icons

Drive icons are displayed independently from the classic PuppyPin.

The desktop can show or hide:

Internal drives

Removable drives

Network drives

Labels

Label frames

Quick unmount buttons

Layout controls include:

Horizontal or vertical orientation

Left, centre or right position

Top, centre or bottom position

Icon size

Horizontal spacing

Vertical spacing

Horizontal offset

Vertical offset

Reverse order

A typical EssoraWM-compatible layout is:

[Main]
desktop_drive_icons=true
desktop_drive_show_internal=true
desktop_drive_show_removable=true
desktop_drive_show_network=false
desktop_drive_icon_size=32
ShowLabels=true
ShowFrame=false
Vertical=false
ReversePack=true
SpacingX=87
SpacingY=87
XOffset=20
YOffset=-40
XPos=0.0
YPos=1.0

XOffset and YOffset move the complete drive group by small increments.

The Realign Drive Icons action reapplies the configured layout and reserveddesktop area.

ROX-Filer refreshes drive placement:

During desktop startup.

After wallpaper changes.

After wbar restarts.

When the usable monitor work area changes.

When devices are added or removed.

Desktop application icons and drive icons use collision-aware placement so theydo not overlap.

Desktop Trash

ROX Desktop can display a standard Trash icon.

The contextual menu provides:

Open Trash

Restore Items

Empty Trash

The icon changes between the active theme's empty and full Trash states whensupported:

user-trash
user-trash-full

Desktop text

Desktop labels use transparent backgrounds.

Normal labels use high-contrast text and a subtle shadow so they remain readableon light and dark wallpapers without drawing a solid rectangle behind everyname.

Selected desktop items use the active GTK3 selection colours.

Classic PuppyPin compatibility

ROX Desktop does not use:

/root/Choices/ROX-Filer/PuppyPin

The old pinboard remains available through the classic ROX command-line options,including:

ROX-Filer -p PINBOARD

Classic panel support also remains available.

Users who run only:

ROX-Filer --desktop

do not need to modify PuppyPin.

Command-line interface

Important desktop commands:

ROX-Filer --desktop
ROX-Filer --desktop-wallpaper
ROX-Filer --desktop-apps

Important file-manager options:

-c, --client-id=ID
-d, --dir=DIR
-D, --close=DIR
-h, --help
-m, --mime-type=FILE
-n, --new
-R, --RPC
-s, --show=FILE
-u, --user
-U, --url=URL
-v, --version
-x, --examine=FILE

Classic compatibility options:

-b, --border=PANEL
-B, --bottom=PANEL
-l, --left=PANEL
-p, --pinboard=PIN
-r, --right=PANEL
-S, --rox-session
-t, --top=PANEL

Build and packaging

Build requirements

A C development environment and development files are required for:

GTK+ 3.22 or newer

GLib and GObject

GDK-Pixbuf

Cairo

libxml2

X11

X Session Management (sm)

Inter-Client Exchange (ice)

pkg-config

GNU Autoconf tools when regenerating configure

The exact package names depend on the distribution.

The main dependency check is equivalent to:

pkg-config --cflags --libs gtk+-3.0 libxml-2.0 sm ice

Optional runtime tools

rsync enables fast directory and batch-copy operations.

A configured terminal emulator is used by terminal actions.

python3 is used for Python terminal actions and the bundled Python template.

udisksctl can mount, unmount and power off devices in normal-user sessions.

gtk-update-icon-cache refreshes the hicolor cache after packageinstallation.

Build and create packages

From the repository root:

cd ROX-Filer
./AppRun --compile

The build process can:

Compile ROX-Filer.

Update translations.

Create the complete package tree.

Remove build and src from the final runtime package.

Create a Debian package.

Preserve a complete Debian package directory.

Create a portable usr/ tree.

Create a portable tar.gz archive.

Remove the temporary build directory.

Generated names use the current project version and architecture. A typicaloutput layout is:

output/
├── rox-filer_<version>_<architecture>.deb
├── rox-filer_<version>_<architecture>/
├── rox-filer-<version>-portable-<architecture>/
└── rox-filer-<version>-portable-<architecture>.tar.gz

The Debian package directory contains the complete package control and runtimetree.

The portable directory contains the runtime usr/ tree and can be used as abase for:

PET

TXZ

Slackware packages

Arch-style packages

Other distribution-specific formats

Build without packaging

./ROX-Filer/AppRun --compile-only

Package an existing binary

./build-package.sh --skip-compile

Clean generated files

./build-package.sh --clean

Clean rebuild

cd ROX-Filer

rm -rf build
rm -f ROX-Filer ROX-Filer.dbg

./AppRun --compile
./AppRun -n

Close an older process before testing a new binary:

killall ROX-Filer 2>/dev/null
./AppRun -n

Debian maintenance scripts

The source includes Debian maintenance scripts for installing and removing thePuppy MIME icons.

The post-installation script:

Creates missing hicolor/*/mimetypes directories.

Installs PET, SFS and SquashFS MIME icons.

Applies mode 0644.

Refreshes the icon cache when possible.

The post-removal script removes the installed package-owned MIME icons andrefreshes the cache.

Repository layout

Important files and directories include:

.
├── README.md
├── CHANGELOG
├── LICENSE
├── ROX-Filer.svg
├── build-package.sh
├── DEBIAN/
│   ├── postinst
│   └── postrm
├── package-base/
├── screenshot/
│   ├── rox-desktop-demo-slow.gif
│   └── rox-particiones.png
├── ROX-Filer/
│   ├── AppRun
│   ├── AppInfo.xml
│   ├── Options.xml
│   ├── Templates.ui
│   ├── Templates/
│   │   ├── Script
│   │   ├── Text.txt
│   │   ├── WebPage.html
│   │   └── python3.py
│   ├── ROX/
│   │   └── MIME/
│   │       ├── application-pet.svg
│   │       ├── application-x-sfs.svg
│   │       └── application-x-squashfs-image.svg
│   ├── images/
│   ├── Messages/
│   ├── Help/
│   └── src/
└── .gitignore

The exact layout may evolve as development continues.

Configuration

ROX-Filer configuration is stored under:

~/.config/rox.sourceforge.net/ROX-Filer/

Important files include:

Options
Settings.xml
desktop.conf
desktop-positions.conf

Other relevant standards-based locations include:

~/.config/mimeapps.list
~/.config/gtk-3.0/settings.ini
~/.local/share/Trash/
~/Desktop

Interface languages

Included interface languages include:

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

Simplified Chinese

Traditional Chinese

The original upstream catalogues are preserved and updated catalogues areincluded for the GTK3 additions.

Precompiled .mo files are included so translations can work on systems wheremsgfmt is unavailable.

ROX-Filer binds the translation domain to:

ROX-Filer/Messages/<locale>/LC_MESSAGES/ROX-Filer.mo

When installed under /usr/local/apps/ROX-Filer, the catalogues remain under:

/usr/local/apps/ROX-Filer/Messages

Compatibility and future backends

The primary target is GTK3 on X11/XLibre.

The source uses:

GDK X11

GTK X11 embedding support

Xlib

X Session Management

ICE

The --desktop command is a stable desktop entry point.

At present it launches the X11/XLibre desktop implementation. In the future,the same command may be able to select additional desktop backends withoutchanging how users start ROX Desktop.

Native Wayland support is not currently implemented.

A complete Wayland backend would require replacements for:

The bottom desktop window layer

Work-area detection

Desktop icon positioning

X11 window-property handling

Panel and compositor integration

Other X11-specific functions

Known limitations

More testing is useful across different GTK3 themes and icon themes.

Behaviour can vary between Puppy Linux variants and window managers.

Some deprecated-but-supported GTK3 APIs remain.

Some historical classic pinboard and panel code remains for compatibility.

X11/XLibre desktop and panel functionality does not work natively on Wayland.

Optional features depend on external tools such as rsync, udisksctl and aterminal emulator.

Translation coverage can vary between languages.

Reporting issues

Please include:

Distribution and version

Desktop or window manager

X11 or XLibre version

GTK3 version

ROX-Filer revision

Steps to reproduce

Console output

Relevant log files

A screenshot for visual problems

For input or event problems:

ROX_TRACE_INPUT=1 GDK_SYNCHRONIZE=1 ./AppRun -n

Report issues to:

puppylinuxjosejp2424@gmail.com

Project repository:

https://github.com/josejp2424/ROX-Filer-gtk3

Contributing

Contributions, tests and translations are welcome.

Changes should:

Preserve the lightweight character of ROX-Filer.

Remain compatible with GTK3.

Avoid unnecessary dependencies.

Preserve original copyright notices.

Follow the existing source style where practical.

Be tested on X11 or XLibre.

Include translation updates for new interface strings when possible.

Changelog and releases

Development changes are recorded in:

CHANGELOG

Release-specific announcements, screenshots and package names should be kept inGitHub Releases, forum posts or changelog entries rather than permanentlydescribing one revision in this README.

When a new revision is published, normally only these items need review:

The visible version in AppInfo.xml and the About dialog.

Package metadata.

CHANGELOG.

Build output naming.

Features that were added, removed or changed.

Known limitations.

