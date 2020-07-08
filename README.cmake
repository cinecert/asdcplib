**General**
As an alternative to configure (autotools), cmake build system files are provided.
In order to use them, you will need to install Cmake binaries on your system, or build Cmake from source.
Note: Cmake 2.8.12 or higher is required.

**Options**
The following options can be defined on the cmake command line with the -D switch:

KUMU_NAMESPACE - Sets the C++ namespace for the Kumu library (default "Kumu")
ASDCP_NAMESPACE - Sets the C++ namespace for the ASDCP library (default "ASDCP")
AS_02_NAMESPACE - Sets the C++ namespace for the AS-02 library (default "AS_02")

**Configuration**
Linux / MacOS:
$ mkdir build
$ cd build/
$ cmake ..
See man cmake for additional options.

**Build/Install**
make && sudo make install
will install in /usr/local/ as usual, in addition cmake target information will be installed in /usr/local/targets/

Wolfgang Ruppel 2016-04-20


