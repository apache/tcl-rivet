#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh "$0" "$@"

# By David N. Welton <davidw@dedasys.com>
# $Id$

# This is to generate new versions based on CVS information.

set newversionvar 0

proc newversion { } {
    global newversionvar
    puts stderr "New version"
    set newversionvar 1
}

proc diffentries { {dir .} } {
    global newversionvar

    set CVSEntries [file join . CVS Entries]
    set OldEntries [file join . .OLDEntries]

    puts stderr "Diffentries for $dir"
    set currentdir [pwd]
    cd $dir
    if { ! [file exists $CVSEntries] } {
	puts stderr "You must be in a directory with a path to ./CVS/Entries."
    }

    if { ! [file exists $OldEntries] } {
	puts stderr "No OLDEntries file.  It will be created."
	set fl [open $OldEntries w]
	close $fl
    }

    set entries [open $CVSEntries]
    set blob ""
    while { [gets $entries ln] != -1 } {
	lappend blob $ln
    }
    close $entries

    set oldentries [open $OldEntries]
    set blob2 ""
    while { [gets $oldentries ln] != -1 } {
	lappend blob2 $ln
    }
    close $oldentries

    if { $blob != $blob2 } {
	newversion
    }
    foreach ln $blob {
	# the regexp below scans for directories in CVS Entries files
	if { [regexp {^D/(.*)////$} "$ln" match dir] } {
	    diffentries $dir
	}
    }

    file copy -force $CVSEntries $OldEntries
    cd $currentdir
}

proc main {} {
    global newversionvar

    diffentries

    if { $newversionvar == 0 } {
	puts stderr "No changes, exiting."
    } else {
	if { [file exists VERSION] } {
	    set versionfile [open VERSION "r"]
	    gets $versionfile versionstring
	    close $versionfile
	} else {
	    set versionstring "0.0.0"
	}

	if { ! [regexp {([0-9]+)\.([0-9]+)\.([0-9]+)} $versionstring match major minor point] } {
	    puts stderr "Problem with versionstring '$versionstring', exiting"
	    exit 1
	}

	set versionfile [ open VERSION "w" ]
	if { [catch {
	while { 1 } {
	    puts -nonewline "Current version: $major.$minor.$point.  "
	    puts -nonewline {Increment [M]ajor, m[I]nor, [P]oint release, or [A]bort? >>> }
	    flush stdout
	    gets stdin answer
	    switch [string tolower $answer] {
		m {
		    incr major
		    set minor 0
		    set point 0
		    break
		}
		i {
		    incr minor
		    set point 0
		    break
		}
		p {
		    incr point
		    break
		}
		a {
		    puts "Aborted"
		    break
		}
	    }
	}
	puts $versionfile "$major.$minor.$point"
	} err] } {
	    puts stderr "Problem writing VERSION file: $err"
	    puts $versionfile "$major.$minor.$point"
	}
	close $versionfile
	puts "Done, version is $major.$minor.$point"
    }
}
main