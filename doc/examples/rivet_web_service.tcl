# The database array contains xml fragments representing the
# results of queries to a database. Many databases are now able
# to produce the results of a query in XML. 
#
#array unset composer
#
set	composer(1)	"&lt;composer&gt;\n"
append  composer(1)     "    &lt;first_name&gt;Claudio&lt;/first_name&gt;\n"
append  composer(1)	"    &lt;last_name&gt;Monteverdi&lt;/last_name&gt;\n"
append	composer(1)	"    &lt;lifespan&gt;1567-1643&lt;/lifespan&gt;\n"
append	composer(1)	"    &lt;era&gt;Renaissance/Baroque&lt;/era&gt;\n"
append	composer(1)	"    &lt;key&gt;1&lt;/key&gt;\n"
append	composer(1)	"&lt;/composer&gt;\n"

set	composer(2)	"&lt;composer&gt;\n"
append  composer(2)     "    &lt;first_name&gt;Johann Sebastian&lt;/first_name&gt;\n"
append  composer(2)	"    &lt;last_name&gt;Bach&lt;/last_name&gt;\n"
append	composer(2)	"    &lt;lifespan&gt;1685-1750&lt;/lifespan&gt;\n"
append	composer(2)	"    &lt;era&gt;Baroque&lt;/era&gt;\n"
append	composer(2)	"    &lt;key&gt;2&lt;/key&gt;\n"
append	composer(2)	"&lt;/composer&gt;\n"

set	composer(3)	"&lt;composer&gt;\n"
append  composer(3)     "    &lt;first_name&gt;Ludwig&lt;/first_name&gt;\n"
append  composer(3)	"    &lt;last_name&gt;van Beethoven&lt;/last_name&gt;\n"
append	composer(3)	"    &lt;lifespan&gt;1770-1827&lt;/lifespan&gt;\n"
append	composer(3)	"    &lt;era&gt;Romantic&lt;/era&gt;\n"
append	composer(3)	"    &lt;key&gt;3&lt;/key&gt;\n"
append	composer(3)	"&lt;/composer&gt;\n"

set	composer(4)	"&lt;composer&gt;\n"
append  composer(4)     "    &lt;first_name&gt;Wolfgang Amadaeus&lt;/first_name&gt;\n"
append  composer(4)	"    &lt;last_name&gt;Mozart&lt;/last_name&gt;\n"
append	composer(4)	"    &lt;lifespan&gt;1756-1791&lt;/lifespan&gt;\n"
append	composer(4)	"    &lt;era&gt;Classical&lt;/era&gt;\n"
append	composer(4)	"    &lt;key&gt;4&lt;/key&gt;\n"
append	composer(4)	"&lt;/composer&gt;\n"

set	composer(5)	"&lt;composer&gt;\n"
append  composer(5)     "    &lt;first_name&gt;Robert&lt;/first_name&gt;\n"
append  composer(5)	"    &lt;last_name&gt;Schumann&lt;/last_name&gt;\n"
append	composer(5)	"    &lt;lifespan&gt;1810-1856&lt;/lifespan&gt;\n"
append	composer(5)	"    &lt;era&gt;Romantic&lt;/era&gt;\n"
append	composer(5)	"    &lt;key&gt;5&lt;/key&gt;\n"
append	composer(5)	"&lt;/composer&gt;\n"

# we use the 'load' argument in order to determine the type of query
#
# load=catalog:		    we have to return a list of the names in the database
# load=composer&amp;res_id=&lt;id&gt;: the script is supposed to return the record
#			    having &lt;id&gt; as record id

if {[var exists load]} {

# the xml declaration is common to every message (error messages included)

    set xml "&lt;?xml version=\"1.0\" encoding=\"ISO-8859-1\"?&gt;\n"
    switch [var get load] {
	catalog {
	    append xml "&lt;catalog&gt;\n"
	    foreach nm [array names composer] {
	    	if {[regexp {&lt;last_name&gt;(.+)&lt;/last_name&gt;}   $composer($nm) m last_name] &amp;&amp; \
		    [regexp {&lt;first_name&gt;(.+)&lt;/first_name&gt;} $composer($nm) m first_name]} {
	            append xml "    &lt;composer key='$nm'&gt;$first_name $last_name&lt;/composer&gt;\n"
		}
	    }
	    append xml "&lt;/catalog&gt;"
	}
	composer {
	    if {[var exists rec_id]} {
		set rec_id [var get rec_id]
		if {[info exists composer($rec_id)]} {
		    append xml $composer($rec_id)
		}
	    }
	}
    }

# we have to tell the client this is an XML message. Failing to do so
# would result in an XMLResponse property set to null

    headers type "text/xml"
    headers add Content-Length [string length $xml]
    puts $xml
}

