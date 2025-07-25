# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded DIO 1.1 [list source [file join $dir dio11.tcl]]
package ifneeded DIO 1.2 [list source [file join $dir dio.tcl]]
package ifneeded DIODisplay 1.0 [list source [file join $dir diodisplay.tcl]]
package ifneeded dio::formatters 1.0 [list source [file join $dir formatters.tcl]]
package ifneeded dio_Mysql 0.4 [list source [file join $dir dio_Mysql04.tcl]]
package ifneeded dio_Mysql 1.2 [list source [file join $dir dio_Mysql.tcl]]
package ifneeded dio_Oracle 0.1 [list source [file join $dir dio_Oracle01.tcl]]
package ifneeded dio_Oracle 1.2 [list source [file join $dir dio_Oracle.tcl]]
package ifneeded dio_Postgresql 0.1 [list source [file join $dir dio_Postgresql01.tcl]]
package ifneeded dio_Postgresql 1.2 [list source [file join $dir dio_Postgresql.tcl]]
package ifneeded dio_Sqlite 0.1 [list source [file join $dir dio_Sqlite01.tcl]]
package ifneeded dio_Sqlite 1.2 [list source [file join $dir dio_Sqlite.tcl]]
package ifneeded dio_Tdbc 1.2.1 [list source [file join $dir dio_Tdbc.tcl]]
