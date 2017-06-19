puts &quot;&lt;html&gt;&lt;head&gt;&quot;
puts &quot;&lt;style&gt;\n  td { font-size: 12px; text-align: center; padding-left: 3px; padding-right: 3px}&quot;
puts &quot;  td.bright { color: #eee; }\n  td.dark { color: #222; }\n&lt;/style&gt;&quot;
puts &quot;&lt;/head&gt;&lt;body&gt;&quot;
puts &quot;&lt;table&gt;&quot;

# we create a 9x9 table selecting a different background for each cell

for {set i 0} { $i &lt; 9 } {incr i} {
    puts &quot;&lt;tr&gt;&quot;
    for {set j 0} {$j &lt; 9} {incr j} {

        set r [expr int(255 * ($i + $j) / 16)] 
        set g [expr int(255 * (8 - $i + $j) / 16)]
        set b [expr int(255 * ($i + 8 - $j) / 16)]

# determining the background luminosity (YIQ space of NTSC) and choosing
# the foreground color accordingly in order maintain maximum contrast

        if { [expr ($r*0.29894)+($g*0.58704)+($b*0.11402)] &gt; 128.0} {
            set cssclass &quot;dark&quot;
        } else {
            set cssclass &quot;bright&quot;
        }

        puts [format &quot;&lt;td bgcolor=\&quot;%02x%02x%02x\&quot; class=\&quot;%s\&quot;&gt;$r $g $b&lt;/td&gt;&quot; $r $g $b $cssclass]
    }
    puts &quot;&lt;/tr&gt;&quot;
}
puts &quot;&lt;/table&gt;&quot;
puts &quot;&lt;/body&gt;&lt;/html&gt;&quot;
