Functy ReadMe
=============

Functy is a 3D graph drawing package. The emphasis for the application is to allow Cartesian and spherical functions to be plotted and altered quickly and easily.

Install
=======

Note that to build Functy you must first install libsymbolic. This should be available from the same place you obtained Functy. Once you've installed libsymbolic, continue with the instructions below.

You'll need the autoconf tools in order to build Functy. They should be available in your distributions repository (e.g. in Ubuntu 13.04 they're available as the "autoconf" package). Once you have these, you should then execute the following command inside the Functy folder before continuing.

autoreconf --install

This will install the correct autotools files inside the folder. You can then continue to build and install Functy using the following 3 commands:

./configure
make
sudo make install

Once built, you can execute Functy with the command

functy

License
=======

Read COPYING for information on the license. Functy is released under the MIT License.

Contact and Links
=================

More information can be found at: http://functy.sourceforge.net/
The source code can be obtained via svn from: https://functy.svn.sourceforge.net/svnroot/functy

I can be contacted via one of the following.

* My website: http://www.flypig.co.uk
* Email: at david@flypig.co.uk



---

 

**Update**: I managed to make it work in Linux Mint 19.3 and in Debian 13 with AppImage (see tags)
