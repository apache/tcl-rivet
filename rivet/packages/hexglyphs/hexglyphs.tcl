# hexgliphs.tcl --

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

#
# The ASCII glyphs appearance was taken from Fossil 
# http://fossil-scm.org/ and reproduced by permission 
# of Richard Hipp
# 

namespace eval ::HexGlyphs:: {

    variable HEXGLYPH

    array set HEXGLYPH {}

    proc glyph {g} {
        variable HEXGLYPH

        return $HEXGLYPH([string toupper $g])
    }
    namespace export glyph

    proc glyph_catalog {} {
        variable HEXGLYPH

        return [array names HEXGLYPH]

    }
    namespace export glyph_catalog

    proc build_hex {hs} {
        variable HEXGLYPH

        set glyphs_avail [array names HEXGLYPH]

        set hs [string toupper $hs]
        for {set i 0} {$i < [string length $hs]} {incr i} {

            set c [string index $hs $i]

            #if {![string is xdigit $c]} 
            if {[lsearch $glyphs_avail $c] < 0} {
                return -code error -errocode invalid_char "Invalid non hexadecimal or non space character"
            }

            set lines [split $HEXGLYPH($c) "\n"]
            set lines [lrange $lines 1 end-1]

            set l 0
            foreach gliphline $lines {
                append hexline($l) $gliphline
                incr l
            } 
   
        }

        return [list $hexline(0) \
                     $hexline(1) \
                     $hexline(2) \
                     $hexline(3) \
                     $hexline(4) \
                     $hexline(5)]

    }

    proc toGlyphs {hexstring} {

        set hexstring_l [split $hexstring " \t"]
        foreach s $hexstring_l {

            set s [string trim $s]

            set string_l [[namespace current]::build_hex $s]
            for {set i 0} {$i < 6} {incr i} {
                lappend bigstring($i) [lindex $string_l $i] 
            }

        }

        return [join [list  [join $bigstring(0) "  "] \
                            [join $bigstring(1) "  "] \
                            [join $bigstring(2) "  "] \
                            [join $bigstring(3) "  "] \
                            [join $bigstring(4) "  "] \
                            [join $bigstring(5) "  "]] "\n"]
    }
    namespace export toGlyphs

    namespace ensemble create
}

set ::HexGlyphs::HEXGLYPH(A) {
           
    /\     
   /  \    
  / /\ \   
 / ____ \  
/_/    \_\ 
}

set ::HexGlyphs::HEXGLYPH(B) {
 ____  
|  _ \ 
| |_) |
|  _ < 
| |_) |
|____/ 
}

set ::HexGlyphs::HEXGLYPH(C) {
  _____ 
 / ____|
| |     
| |     
| |____ 
 \_____|
}

set ::HexGlyphs::HEXGLYPH(D) {
 _____  
|  __ \ 
| |  | |
| |  | |
| |__| |
|_____/ 
}

set ::HexGlyphs::HEXGLYPH(E) {
 ______ 
|  ____|
| |__   
|  __|  
| |____ 
|______|
}

set ::HexGlyphs::HEXGLYPH(F) {
 ______ 
|  ____|
| |__   
|  __|  
| |     
|_|     
}

set ::HexGlyphs::HEXGLYPH(G) {
  _____  
 / ____| 
| |  __  
| | |_ \ 
| |___| |
 \_____/ 
}

set ::HexGlyphs::HEXGLYPH(0) {
  ___  
 / _ \ 
| | | |
| | | |
| |_| |
 \___/ 
}

set ::HexGlyphs::HEXGLYPH(1) {
 __ 
/_ |
 | |
 | |
 | |
 |_|
}

set ::HexGlyphs::HEXGLYPH(2) {
 ___  
|__ \ 
   ) |
  / / 
 / /_ 
|____|
}

set ::HexGlyphs::HEXGLYPH(3) {
 ____  
|___ \ 
  __) |
 |__ < 
 ___) |
|____/ 
}

set ::HexGlyphs::HEXGLYPH(4) {
 _  _   
| || |  
| || |_ 
|__   _|
   | |  
   |_|  
}

set ::HexGlyphs::HEXGLYPH(5) {
 _____ 
| ____|
| |__  
|___ \ 
 ___) |
|____/ 
}

set ::HexGlyphs::HEXGLYPH(6) {
   __  
  / /  
 / /_  
| '_ \ 
| (_) |
 \___/ 
}

set ::HexGlyphs::HEXGLYPH(7) {
 ______ 
|____  |
    / / 
   / /  
  / /   
 /_/    
}
set ::HexGlyphs::HEXGLYPH(8) {
  ___  
 / _ \ 
| (_) |
 > _ < 
| (_) |
 \___/ 
}

set ::HexGlyphs::HEXGLYPH(9) {
  ___  
 / _ \ 
| (_) |
 \__, |
   / / 
  /_/  
}
 
set ::HexGlyphs::HEXGLYPH(-) {
        
        
 _____  
|_____| 
        
        
}

set ::HexGlyphs::HEXGLYPH(:) {
        
   __   
  |__|  
   __   
  |__|  
        
        
}
package provide HexGlyphs 0.1


