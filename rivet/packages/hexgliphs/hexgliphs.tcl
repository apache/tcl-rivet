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

namespace eval ::HexGliphs:: {

    variable HEXGLIPH

    array set HEXGLIPH {}


    proc build_hex {hs} {
        variable HEXGLIPH

        for {set i 0} {$i < [string length $hs]} {incr i} {

            set lines [split $HEXGLIPH([string toupper [string index $hs $i]]) "\n"]
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

    proc toGliphs {hexstring} {

        set hexstring_l [split $hexstring " "]
        foreach s $hexstring_l {

            set s [string trim $s]

            set string_l [[namespace current]::build_hex $s]
            for {set i 0} {$i < 6} {incr i} {
                lappend bigstring($i) [lindex $string_l $i] 
            }

        }

        return [join [list  [join $bigstring(0) "   "] \
                            [join $bigstring(1) "   "] \
                            [join $bigstring(2) "   "] \
                            [join $bigstring(3) "   "] \
                            [join $bigstring(4) "   "] \
                            [join $bigstring(5) "   "]] "\n"]
    }
    namespace export toGliphs


    namespace ensemble create

}

set ::HexGliphs::HEXGLIPH(A) {
           
    /\     
   /  \    
  / /\ \   
 / ____ \  
/_/    \_\ 
}

set ::HexGliphs::HEXGLIPH(B) {
 ____  
|  _ \ 
| |_) |
|  _ < 
| |_) |
|____/ 
}

set ::HexGliphs::HEXGLIPH(C) {
  _____ 
 / ____|
| |     
| |     
| |____ 
 \_____|
}

set ::HexGliphs::HEXGLIPH(D) {
 _____  
|  __ \ 
| |  | |
| |  | |
| |__| |
|_____/ 
}

set ::HexGliphs::HEXGLIPH(E) {
 ______ 
|  ____|
| |__   
|  __|  
| |____ 
|______|
}

set ::HexGliphs::HEXGLIPH(F) {
 ______ 
|  ____|
| |__   
|  __|  
| |     
|_|     
}

set ::HexGliphs::HEXGLIPH(0) {
  ___  
 / _ \ 
| | | |
| | | |
| |_| |
 \___/ 
}

set ::HexGliphs::HEXGLIPH(1) {
 __ 
/_ |
 | |
 | |
 | |
 |_|
}

set ::HexGliphs::HEXGLIPH(2) {
 ___  
|__ \ 
   ) |
  / / 
 / /_ 
|____|
}

set ::HexGliphs::HEXGLIPH(3) {
 ____  
|___ \ 
  __) |
 |__ < 
 ___) |
|____/ 
}

set ::HexGliphs::HEXGLIPH(4) {
 _  _   
| || |  
| || |_ 
|__   _|
   | |  
   |_|  
}

set ::HexGliphs::HEXGLIPH(5) {
 _____ 
| ____|
| |__  
|___ \ 
 ___) |
|____/ 
}

set ::HexGliphs::HEXGLIPH(6) {
   __  
  / /  
 / /_  
| '_ \ 
| (_) |
 \___/ 
}

set ::HexGliphs::HEXGLIPH(7) {
 ______ 
|____  |
    / / 
   / /  
  / /   
 /_/    
}
set ::HexGliphs::HEXGLIPH(8) {
  ___  
 / _ \ 
| (_) |
 > _ < 
| (_) |
 \___/ 
}

set ::HexGliphs::HEXGLIPH(9) {
  ___  
 / _ \ 
| (_) |
 \__, |
   / / 
  /_/  
}
 

package provide HexGliphs


