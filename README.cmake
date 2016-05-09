**General**
As an alternative to configure (autotools), cmake build system files are provided.
In order to use them, you will need to install Cmake binaries on your system, or build Cmake from source.
Note: Cmake 2.8.12 or higher is required.

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


