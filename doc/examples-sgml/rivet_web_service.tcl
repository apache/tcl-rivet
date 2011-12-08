#
# Ajax query servelet: a pseudo database is built into the dictionary &#39;composers&#39; with the
# purpose of emulating the role of a real data source. 
# The script answers with  2 types of responses: a catalog of the record ids and a database 
# entry matching a given rec_id. The script obviously misses the error handling and the
# likes. Just an example to see rivet sending xml data to a browser. The full Tcl, JavaScript
# and HTML code are available from http://people.apache.org/~mxmanghi/rivet-ajax.tar.gz

# This example requires Tcl8.5 or Tcl8.4 with package &#39;dict&#39; 
# (http://pascal.scheffers.net/software/tclDict-8.5.2.tar.gz)
# 

# A pseudo database. rec_id matches a record in the db

set composers [dict create  \
                1 {first_name Claudio middle_name &quot;&quot; last_name Monteverdi   \
                    lifespan 1567-1643 era Renaissance/Baroque}             \
                2 {first_name Johann middle_name Sebastian last_name Bach   \
                    lifespan 1685-1750 era Baroque }                        \
                3 {first_name Ludwig middle_name &quot;&quot; last_name &quot;van Beethoven&quot; \
                    lifespan 1770-1827 era Classical/Romantic}              \
                4 {first_name Wolfgang middle_name Amadeus last_name Mozart \
                    lifespan 1756-1791 era Classical }                      \
                5 {first_name Robert middle_name &quot;&quot; last_name Schumann      \
                    lifespan 1810-1856 era Romantic} ]

# we use the &#39;load&#39; argument in order to determine the type of query
#
# load=catalog:         we have to return a list of the names in the database
# load=composer&amp;amp;res_id=&lt;id&gt;: the script is supposed to return the record
#               having &lt;id&gt; as record id

if {[::rivet::var exists load]} {

# the xml declaration is common to every message (error messages included)

    set xml &quot;&lt;?xml version=\&quot;1.0\&quot; encoding=\&quot;ISO-8859-1\&quot;?&gt;\n&quot;
    switch [::rivet::var get load] {
        catalog {
            append xml &quot;&lt;catalog&gt;\n&quot;
            foreach nm [dict keys $composers] {
                set first_name  [dict get $composers $nm first_name]
                set middle_name [dict get $composers $nm middle_name]
                set last_name   [dict get $composers $nm last_name]
                append xml &quot;    &lt;composer key=\&quot;$nm\&quot;&gt;$first_name &quot;
                if {[string length [string trim $middle_name]] &gt; 0} {
                    append xml &quot;$middle_name &quot;
                }
                append xml &quot;$last_name&lt;/composer&gt;\n&quot;
            }
            append xml &quot;&lt;/catalog&gt;\n&quot;
        }
        composer {
            append xml &quot;&lt;composer&gt;\n&quot;
            if {[::rivet::var exists rec_id]} {
                set rec_id [::rivet::var get rec_id]
                if {[dict exists $composers $rec_id]} {
                    foreach {k v} [dict get $composers $rec_id] {
                        append xml &quot;&lt;$k&gt;$v&lt;/$k&gt;\n&quot;
                    }
                }
            }
            append xml &quot;&lt;/composer&gt;\n&quot;
        }
    }

# we have to tell the client this is an XML message. Failing to do so
# would result in an XMLResponse property set to null

    ::rivet::headers type &quot;text/xml&quot;
    ::rivet::headers add Content-Length [string length $xml]
    puts $xml
}


