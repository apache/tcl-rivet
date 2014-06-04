puts "<html><head>"
puts "<style>"
puts "td { font-size: 12px; }\n td.bright { color: #ccc; }\n td.dark { color: #222; }\n</style>"
puts "</head><body>"
puts "<table>"

# we create a 8x8 table selecting a different background for each cell

for {set i 0} { $i < 9 } {incr i} {
    puts "<tr>"
    for {set j 0} {$j < 9} {incr j} {

        set r [expr int(255 * ($i + $j) * 32 / 512)] 
        set g [expr int(255 * (8 - $i + $j) * 32 / 512)]
        set b [expr int(255 * ($i + 8 - $j) * 32 / 512)]

# determining the background luminosity (NTSC) and choosing the foreground
# color accordingly in order maintain maximum contrast

        if { [expr ($r*0.2126)+($g*0.7152)+($b*0.0722)] > 128} {
            set cssclass "dark"
        } else {
            set cssclass "bright"
        }

        puts [format "<td bgcolor=\"%02x%02x%02x\" class=\"%s\">$r $g $b</td>" $r $g $b $cssclass]
    }
    puts "</tr>"
}
puts "</table>"
puts "</body></html>"
