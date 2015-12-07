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

namespace eval ::AsciiGlyphs:: {

    variable ASCIIGLYPHS

    array set ASCIIGLYPHS {}

    proc glyph {g} {
        variable ASCIIGLYPHS

        return $ASCIIGLYPHS([string toupper $g])
    }
    namespace export glyph

    proc glyph_catalog {} {
        variable ASCIIGLYPHS

        return [array names ASCIIGLYPHS]

    }
    namespace export glyph_catalog

    proc build_hex {hs} {
        variable ASCIIGLYPHS

        set glyphs_avail [array names ASCIIGLYPHS]

        set hs [string toupper $hs]
        for {set i 0} {$i < [string length $hs]} {incr i} {

            set c [string index $hs $i]

            #if {![string is xdigit $c]} 
            if {[lsearch $glyphs_avail $c] < 0} {
                return -code error -errocode invalid_char "Invalid non hexadecimal or non space character"
            }

            set lines [split $ASCIIGLYPHS($c) "\n"]
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

set ::AsciiGlyphs::ASCIIGLYPHS(A) {
           
    /\     
   /  \    
  / /\ \   
 / ____ \  
/_/    \_\ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(B) {
 ____  
|  _ \ 
| |_) |
|  _ < 
| |_) |
|____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(C) {
  _____ 
 / ____|
| |     
| |     
| |____ 
 \_____|
}

set ::AsciiGlyphs::ASCIIGLYPHS(D) {
 _____  
|  __ \ 
| |  | |
| |  | |
| |__| |
|_____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(E) {
 ______ 
|  ____|
| |__   
|  __|  
| |____ 
|______|
}

set ::AsciiGlyphs::ASCIIGLYPHS(F) {
 ______ 
|  ____|
| |__   
|  __|  
| |     
|_|     
}

set ::AsciiGlyphs::ASCIIGLYPHS(G) {
  _____  
 / ____| 
| |  __  
| | |_ \ 
| |___| |
 \_____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(H) {
 _   _  
| | | | 
| |_| | 
|  _  | 
| | | | 
|_| |_| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(I) {
 ___ 
|   |
 | | 
 | | 
 | | 
|___|
}

set ::AsciiGlyphs::ASCIIGLYPHS(J) {
   ___ 
  |   |
   | | 
 _ | | 
| || | 
\____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(K) {
 _   _  
| | / | 
| |/ /  
|  <    
| |\ \  
|_| \_\ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(L) {
 _      
| |     
| |     
| |     
| |___  
|_____| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(M) {
 _    _  
| \  / | 
|  \/  | 
| |  | | 
| |  | | 
|_|  |_| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(N) {
 _    _  
| \  | | 
|  \ | | 
| \ \| | 
| |\   | 
|_| \__| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(O) {
 _____  
|  _  | 
| | | | 
| | | | 
| |_| | 
|_____| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(P) {
 ____   
|  _ \  
| |_| | 
|  __/  
| |     
|_|     
}

set ::AsciiGlyphs::ASCIIGLYPHS(Q) {
 _____  
|  _  | 
| | | | 
| |_| | 
|___\\| 
     \\ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(R) {
 ____   
|  _ \  
| |_| | 
|    /  
| |\ \  
|_| \_\ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(S) {
 ____   
/  __|  
| |__   
\__  \  
 __|  | 
|____/  
}

set ::AsciiGlyphs::ASCIIGLYPHS(T) {
 _____  
|_   _| 
  | |   
  | |   
  | |   
  |_|   
}

set ::AsciiGlyphs::ASCIIGLYPHS(U) {
 _   _  
| | | | 
| | | | 
| | | | 
| |_| | 
 \___/  
}

set ::AsciiGlyphs::ASCIIGLYPHS(V) {
 __     __ 
 \ \   / / 
  \ \ / /  
   \ v /   
    \ /    
     v     
}

set ::AsciiGlyphs::ASCIIGLYPHS(W) {
 __      __ 
 \ \    / / 
  \ \/\/ /  
   \    /   
    \__/    
            
}

set ::AsciiGlyphs::ASCIIGLYPHS(X) {
 __  __  
 \ \/ /  
  \  /   
  /  \   
 / /\ \  
/_/  \_\ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(Y) {
 __    __ 
 \ \  / / 
  \ \/ /  
   \  /   
   / /    
  /_/     
}

set ::AsciiGlyphs::ASCIIGLYPHS(Z) {
  ______   
 |____  |  
     / /   
    / /    
   / /___  
  /______| 
}


set ::AsciiGlyphs::ASCIIGLYPHS(0) {
  ___  
 / _ \ 
| | | |
| | | |
| |_| |
 \___/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(1) {
 __ 
/_ |
 | |
 | |
 | |
 |_|
}

set ::AsciiGlyphs::ASCIIGLYPHS(2) {
 ___  
|__ \ 
   ) |
  / / 
 / /_ 
|____|
}

set ::AsciiGlyphs::ASCIIGLYPHS(3) {
 ____  
|___ \ 
  __) |
 |__ < 
 ___) |
|____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(4) {
 _  _   
| || |  
| || |_ 
|__   _|
   | |  
   |_|  
}

set ::AsciiGlyphs::ASCIIGLYPHS(5) {
 _____ 
| ____|
| |__  
|___ \ 
 ___) |
|____/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(6) {
   __  
  / /  
 / /_  
| '_ \ 
| (_) |
 \___/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(7) {
 ______ 
|____  |
    / / 
   / /  
  / /   
 /_/    
}
set ::AsciiGlyphs::ASCIIGLYPHS(8) {
  ___  
 / _ \ 
| (_) |
 > _ < 
| (_) |
 \___/ 
}

set ::AsciiGlyphs::ASCIIGLYPHS(9) {
  ___  
 / _ \ 
| (_) |
 \__, |
   / / 
  /_/  
}
 
set ::AsciiGlyphs::ASCIIGLYPHS(-) {
       
       
 ____  
|____| 
       
       
}

set ::AsciiGlyphs::ASCIIGLYPHS(_) {
         
         
         
         
 ______  
|______| 
}

set ::AsciiGlyphs::ASCIIGLYPHS(:) {
     
  _  
 |_| 
  _  
 |_| 
     
}

set ::AsciiGlyphs::ASCIIGLYPHS(;) {
     
  _  
 |_| 
  _  
 | | 
 |/   
}

set ::AsciiGlyphs::ASCIIGLYPHS(.) {
     
     
     
  _  
 |_| 
     
}

set ::AsciiGlyphs::ASCIIGLYPHS(/) {
      __ 
     / / 
    / /  
   / /   
  / /    
 /_/     
}

set ::AsciiGlyphs::ASCIIGLYPHS(\) {
 __      
 \ \     
  \ \    
   \ \   
    \ \  
     \_\ 
}

package provide AsciiGlyphs 0.1


