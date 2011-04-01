# convert_examples --
#
#
# reads the examples dir, checks if a file had been already converted
# in <target-dir> and compares their mtimes. If the file in
# <source-dir> is more recent the target dir is recreated.

# The script uses Rivet's escape_sgml_chars to convert examples code
# into text suitable to be inclued and displayed in XML/XHTML 
# documentation

# NOTICE: This script requires Rivet the rivet scripts and
# libraries in the auto_path.

# usage:
#
# tclsh ./convert_examples
#

package require Rivet

set source_dir examples
set target_dir examples-sgml

set source_examples [glob [file join $source_dir *.*]]
#puts "source examples $source_examples"

foreach example $source_examples {

    set exam_name [file tail $example]
    set example_sgml [file join $target_dir $exam_name]

    set example_sgml_exists [file exists $example_sgml]

    if {!$example_sgml_exists || \
        ([file mtime $example] > [file mtime $example_sgml])} { 

        puts "$example needs rebuild"
        
        set example_text [read_file $example]

        set example_sgml_fid [open $example_sgml w+]
        puts $example_sgml_fid [escape_sgml_chars $example_text]
        close $example_sgml_fid

    }

}

