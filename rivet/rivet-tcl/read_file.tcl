###
## read_file <file>
##    Read the entire contents of a file and return it as a string.
##
##    file - Name of the file to read.
###

proc read_file {file} {
    set fp [open $file]
    set x [read $fp]
    close $fp
    return $x
}
