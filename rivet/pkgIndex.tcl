# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded AsciiGlyphs 0.1 [list source [file join $dir packages/asciiglyphs/asciiglyphs.tcl]]
package ifneeded Calendar 1.2 [list source [file join $dir packages/calendar/calendar.tcl]]
package ifneeded DIO 1.1 [list source [file join $dir packages/dio/dio.tcl]]
package ifneeded DIODisplay 1.0 [list source [file join $dir packages/dio/diodisplay.tcl]]
package ifneeded Rivet 3.2 [list source [file join $dir init.tcl]]
package ifneeded RivetEntities 1.0 [list source [file join $dir packages/entities/entities.tcl]]
package ifneeded Session 1.0 [list source [file join $dir packages/session/session-class.tcl]]
package ifneeded dio_Mysql 0.3 [list source [file join $dir packages/dio/dio_Mysql.tcl]]
package ifneeded dio_Oracle 0.1 [list source [file join $dir packages/dio/dio_Oracle.tcl]]
package ifneeded dio_Postgresql 0.1 [list source [file join $dir packages/dio/dio_Postgresql.tcl]]
package ifneeded dio_Sqlite 0.1 [list source [file join $dir packages/dio/dio_Sqlite.tcl]]
package ifneeded dio_Tdbc 0.1 [list source [file join $dir packages/dio/dio_Tdbc.tcl]]
package ifneeded form 1.0 [list source [file join $dir packages/form/form.tcl]]
package ifneeded form 2.1 [list source [file join $dir packages/form/form2.tcl]]
package ifneeded formbroker 1.0 [list source [file join $dir packages/formbroker/formbroker.tcl]]
package ifneeded rivetlib 3.2.0 [list load [file join $dir librivetlib.so]]
package ifneeded rivetparser 0.2 [list load [file join $dir librivetparser.so]]
package ifneeded tclrivet 0.1 [list source [file join $dir packages/tclrivet/tclrivet.tcl]]
package ifneeded tclrivetparser 0.1 [list source [file join $dir packages/tclrivet/tclrivetparser.tcl]]
