# Building ROX-Filer packages

Run from the project root:

```sh
./build-package.sh
```

The script compiles both **ROX-Filer** and the companion **ROX File Search**
application, then creates everything under `output/`:

- `rox-filer_<version>_<architecture>.deb`
- the complete Debian package directory;
- a portable filesystem directory containing `usr/`;
- a portable `.tar.gz` archive for other package formats.

The generated runtime trees include:

- `/usr/local/apps/ROX-Filer`
- `/usr/bin/ROX-Filer`
- `/usr/bin/rox-find`
- the ROX File Search desktop entry, icon and translation catalogues
- the standard Puppy MIME icons installed through the Debian maintenance scripts

The runtime trees do not contain `ROX-Filer/src` or `ROX-Filer/build`. The
development source keeps `ROX-Filer/src`, because it is required for future
compilation. The temporary `ROX-Filer/build` directory is removed after
packaging.

The installed `ROX-Filer/ROX` directory comes from the package base supplied by
josejp2424 in `package-base/usr/local/apps/ROX-Filer/ROX`.

To package binaries that have already been compiled:

```sh
./build-package.sh --skip-compile
```

To compile without creating packages:

```sh
./ROX-Filer/AppRun --compile-only
```

To remove generated package output and temporary binaries:

```sh
./build-package.sh --clean
```
