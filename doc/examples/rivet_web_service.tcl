#
# Ajax query servelet: a pseudo database is built into the dictionary 'composers' with the
# purpose of emulating the role of a real data source. 
# The script answers with  2 types of responses: a catalog of the record ids and a database 
# entry matching a given rec_id. The script obviously misses the error handling and the
# likes. Just an example to see rivet sending xml data to a browser. The full Tcl, JavaScript
# and HTML code are available from http://people.apache.org/~mxmanghi/rivet-ajax.tar.gz

# This example requires Tcl8.5 or Tcl8.4 with package 'dict' 
# (http://pascal.scheffers.net/software/tclDict-8.5.2.tar.gz)
# 

# A pseudo database. rec_id matches a record in the db

set composers [dict create  \
                1 {first_name Claudio middle_name "" last_name Monteverdi   \
                    lifespan 1567-1643 era Renaissance/Baroque}             \
                2 {first_name Johann middle_name Sebastian last_name Bach   \
                    lifespan 1685-1750 era Baroque }                        \
                3 {first_name Ludwig middle_name "" last_name "van Beethoven" \
                    lifespan 1770-1827 era Classical/Romantic}              \
                4 {first_name Wolfgang middle_name Amadeus last_name Mozart \
                    lifespan 1756-1791 era Classical }                      \
                5 {first_name Robert middle_name "" last_name Schumann      \
                    lifespan 1810-1856 era Romantic} ]

# we use the 'load' argument in order to determine the type of query
#
# load=catalog:         we have to return a list of the names in the database
# load=composer&amp;res_id=<id>: the script is supposed to return the record
#               having <id> as record id

if {[::rivet::var exists load]} {

# the xml declaration is common to every message (error messages included)

    set xml "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
    switch [::rivet::var get load] {
        catalog {
            append xml "<catalog>\n"
            foreach nm [dict keys $composers] {
                set first_name  [dict get $composers $nm first_name]
                set middle_name [dict get $composers $nm middle_name]
                set last_name   [dict get $composers $nm last_name]
                append xml "    <composer key=\"$nm\">$first_name "
                if {[string length [string trim $middle_name]] > 0} {
                    append xml "$middle_name "
                }
                append xml "$last_name</composer>\n"
            }
            append xml "</catalog>\n"
        }
        composer {
            append xml "<composer>\n"
            if {[::rivet::var exists rec_id]} {
                set rec_id [::rivet::var get rec_id]
                if {[dict exists $composers $rec_id]} {
                    foreach {k v} [dict get $composers $rec_id] {
                        append xml "<$k>$v</$k>\n"
                    }
                }
            }
            append xml "</composer>\n"
        }
    }

# we have to tell the client this is an XML message. Failing to do so
# would result in an XMLResponse property set to null

    ::rivet::headers type "text/xml"
    ::rivet::headers add Content-Length [string length $xml]
    puts $xml
}

