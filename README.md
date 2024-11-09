XQF
===

![XQF logo](pixmaps/128x128/xqf.png)

DESCRIPTION
-----------

XQF is a server browser and launcher for games using id Tech engines, Unreal engines and many others (see the [wiki/Supported-games](https://github.com/XQF/xqf/wiki/Supported-games) page for details). XQF is a front-end to QStat and uses the GTK+ 2 toolkit.

To learn more about what's new in XQF, please read the file [NEWS.md](NEWS.md) and the [ChangeLog](ChangeLog).


DOWNLOADS
---------

See the Wiki page: [wiki/Downloads](https://github.com/XQF/xqf/wiki/Downloads).


INSTALLATION
------------

[![Build](https://github.com/XQF/xqf/actions/workflows/build.yml/badge.svg)](https://github.com/XQF/xqf/actions/workflows/build.yml)

```sh
git clone https://github.com/XQF/xqf.git
cd xqf
mkdir build
cd build
cmake ..
make
make install
```

On Debian or Ubuntu, use ``cmake -DWITH_QSTAT=/usr/bin/quakestat -DCMAKE_INSTALL_PREFIX=/usr ..``.

For extended information, read this Wiki page: [wiki/How-to-build](https://github.com/XQF/xqf/wiki/How-to-build).


LINKS
-----

* Current home page: [xqf.github.io](http://xqf.github.io)
* XQF Wiki: [github.com/XQF/xqf/wiki](https://github.com/XQF/xqf/wiki)
* IRC Channel: [#xqf@Libera.Chat](https://web.libera.chat/?channel=#xqf)
* Source code repository: [github.com/XQF/xqf](https://github.com/XQF/xqf/) (up to date Git repository)
* Historical mail list: [sf.net/p/xqf/mailman/](https://sourceforge.net/p/xqf/mailman/) (havn't seen mail in years)
* Historical archives: [sf.net/p/xqf](https://sourceforge.net/projects/xqf/) (previously published files, old SVN repository, and old ticket list)
* QStat, the tool XQF is based on: [github.com/Unity-Technologies/qstat](https://github.com/Unity-Technologies/qstat)


HOW TO CONTRIBUTE
-----------------

The best way to contribute code is to fork this project. To contribute translation please visit our Transifex project. You will find more details on the wiki: [wiki/How-to-contribute](https://github.com/XQF/xqf/wiki/How-to-contribute).


COPYRIGHT
---------

XQF is Copyright © 1998-2000 Roman Pozlevich.
See the wiki page for contributors: [wiki/Contributors](https://github.com/XQF/xqf/wiki/Contributors).

Copying is allowed under the terms of the GNU General Public License.  
See the file [COPYING](COPYING) for more details.


HISTORY
-------

XQF was originally written by Roman Pozlevich in 1998. It has been maintained and improved by a devoted team over the years with following major developers:

1998-2000 Roman Pozlevich <hidden email="roma@botik.ru"/>  
2000-2002 Bill Adams <hidden email="bill@evilbill.org"/>  
2000-2003 Alex Burger <hidden email="alex_b@users.sf.net"/>  
2001-2010 Ludwig Nussel <hidden email="ludwig.nussel@suse.de"/>  
2001-2015 Jordi Mallach <hidden email="jordi@debian.org"/>  
2015-2015 Artem Vorotnikov <hidden email="artem@vorotnikov.me"/>  
2014-2024 Zack Middleton <hidden email="zturtleman@gmail.com"/>  
2013-2024 Thomas Debesse <hidden email="dev@illwieckz.net"/>  
