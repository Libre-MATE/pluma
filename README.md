# Pluma - The MATE text editor

![pluma-icon](pixmaps/pluma.ico)

## General Information

Pluma (pluma) is a small and lightweight UTF-8 text editor for the MATE environment. It started as a fork of Gedit (at version 2.30) in 2011, back then the text editor for the GNOME 2 environment.

Pluma is part of MATE and uses the latest GTK+ and MATE libraries.
Complete MATE integration is featured, with support for Drag and Drop from Caja (the MATE file manager), the use of the MATE help system,
the MATE Virtual File System and the MATE print framework.

Pluma uses a Multiple Document Interface, which lets you edit more than
one document at the same time.

Pluma supports most standard editing features, plus several not found in your
average text editor (plugins being the most notable of these).

Although new features are always under development, currently Pluma has:

- **Complete support for UTF-8 text**
- **Syntax highlighting**
- **Support for editing remote files**
- **Search and Replace**
- **Printing and Print Previewing Support**
- **File Revert**
- **A complete preferences interface**
- **Configurable Plugin system**


Some of the plugins, packaged and installed with Pluma include, among others:

- **Spell checker** - *Checks the spelling of the current document*
- **File Browser** - *Easy file access from the side pane*
- **Sort** - *Sorts a document or selected text*
- **Insert Date/Time** - *Inserts current date and time at the cursor position*
- **Tag list** - *Provides a method to easily insert code tags.*

Other external plugins are also available.

Pluma is released under the GNU General Public License (GPL) version 2, see the file 'COPYING' for more information.


## Build/Installation

Pluma requires GTK+ (>= 3.22) and GtkSourceView (>= 4.0.2). For a complete list of dependencies see the [build.yml](https://github.com/mate-desktop/pluma/blob/master/.build.yml).

**Warning**: This procedure doesn't install in a separate prefix, so it may
overwrite your system binaries.

Simple install procedure:

```
$ git submodule update --init --recursive   # Init git submodules
$ NOCONFIGURE=1 ./autogen.sh                # Copy configuration requirements
$ ./configure                               # Build configuration
$ make                                      # Build
[ Become root if necessary ]
$ make install                              # Installation
```
For installation to a separate prefix change the above `./configure` command to

```
$ ./configure --prefix=/an/other/path
```
To get more information type the command below:
```
$ ./configure --help
```
