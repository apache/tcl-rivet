# ![Rivet Logo](doc/images/home.png) <center>Apache Rivet</center> 

[Apache Rivet](https://tcl.apache.org/rivet/) is a powerful, flexible, consistent, fast, and robust solution to creating web applications with the help of the [Tcl](http://www.tcl.tk/) language and [Apache HTTP Server](https://httpd.apache.org/).

See the [doc/html/installation.html](doc/html/installation.html) directory for installation and usage instructions.

See [INSTALL](INSTALL) for brief installation instructions - although the above docs in HTML are more thorough and extensive.

See [LICENSE](LICENSE) for licensing terms.


## Current Travis/AppVeyor CI build status for Rivet:

| OS | Master Branch | Release Branch | Branch 'winbuild' |
---|---|---|--
| Linux 64, Apache 2.4.37, Tcl/Tk 8.6 |  | | |
| Windows 64, Apache 2.4.37, Tcl/Tk 8.6.7 | [![Build status](https://ci.appveyor.com/api/projects/status/3si279ye7gxl7wgg/branch/master?svg=true)](https://ci.appveyor.com/project/petasis/tcl-rivet/branch/master) <br/>fork: [![Build status](https://ci.appveyor.com/api/projects/status/69nj1qs4ia8pj87v/branch/master?svg=true)](https://ci.appveyor.com/project/petasis/tcl-rivet-scuqj/branch/master) | | [![Build status](https://ci.appveyor.com/api/projects/status/3si279ye7gxl7wgg/branch/winbuild?svg=true)](https://ci.appveyor.com/project/petasis/tcl-rivet/branch/winbuild) <br/>fork: [![Build status](https://ci.appveyor.com/api/projects/status/69nj1qs4ia8pj87v/branch/winbuild?svg=true)](https://ci.appveyor.com/project/petasis/tcl-rivet-scuqj/branch/winbuild) |
| macOS 64 (Darwin), Apache 2.4.37, Tcl/Tk 8.5 |  | | |

## RIVET NAMESPACE

 - The Rivet command set was moved into the ::rivet namespace and each command should now be fully qualified. By default the command set (exceptions are ::rivet::try and ::rivet::catch) is placed in the namespace export list, so you can import it into the global namespace for compatibility with Rivet version < 2.0. This is a deprecated option though and it could be removed in future releases
