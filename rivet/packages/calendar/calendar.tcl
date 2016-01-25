#
#   Copyright 2000-2005 The Apache Software Foundation
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#
#
# $Id: calendar.tcl 916 2010-07-03 00:37:44Z massimo.manghi $
#

package provide Calendar 1.2
package require Itcl

# Calendar: base class to create a calendar table. 
#
# Calendar prints an ascii calendar following the output form of a Unix 
# 'cal' command. Even though it can be used as a concrete class it was
# designed to have methods and mechanisms abstract enough to be easly
# customized and specialized through derivation of other classes (see XmlCalendar)
#
# The output of Calendar (method 'emit') 
#
#
#       Jun 2010            |   header     | banner  
#  Su Mo Tu We Th Fr Sa     |              | weekdays
#        1  2  3  4  5      |   table
#  6  7  8  9 10 11 12      |   
# 13 14 15 16 17 18 19      |
# 20 21 22 23 24 25 26
# 27 28 29 30
#
# 


::itcl::class   Calendar {
    public  common  month_names
    public  common  day_names

    private variable    month_year_processed    {}

# language to be used: key to be used in 'month_names' 
# and in case in other databases

    public  variable    language    en 

    private method  numberOfDays    { month year }
    private method  cal             { month year }

    protected method weekdays       { }
    protected method banner         { mth yr }
    protected method header         { mth yr }
    protected method first_week     { mth yr wkday } 
    protected method formatDayCell  { day } 
    protected method openRow        { wkn }
    protected method closeRow       { }
    protected method table          { mth yr }
    protected method startOutput    { } 
    protected method closeOutput    { }

    public method cal_processed {} { return $month_year_processed }

    public method emit      { args }

    constructor {args} {
    set month_names(en) { Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec }
    set month_names(it) { Gen Feb Mar Apr Mag Giu Lug Ago Set Ott Nov Dic }
    set day_names(en)   { Su Mo Tu We Th Fr Sa }
    set day_names(it)   { Do Lu Ma Me Gi Ve Sa }
    }
}


# numberOfDays <month> <year>: private method that returns the number of days in 
# the current month. 
#

::itcl::body Calendar::numberOfDays {month year} {

    if {$month == 12} { set month 1; incr year }
    return [clock format [clock scan "[incr month]/1/$year  1 day ago"] -format %d]

}

::itcl::body Calendar::banner {month_idx yr} {

    set month_name [lindex $month_names($language) $month_idx]
    return "      $month_name $yr\n"

}

::itcl::body Calendar::weekdays {} {
    return "$day_names($language)\n"
}

# header <month_idx> <year>
# returns the header of the calendar table. The header is made of a banner (e.g. "Jul 2010")
# and a list of the weekdays (Su Mo ... Sa)
#
#   Arguments:      <month_idx> month index (0: jan, 11: dec). 
#           <year> year number.
#
#   Returned value: text of the cal table header.
#

::itcl::body Calendar::header {mth_idx yr} { 
    return "[$this banner $mth_idx $yr][$this weekdays]"
}

# first_week: cal tables are organized in columns corresponding to weekdays (from Sunday to Saturday). 
# first_week returns as many blank cells as the number of weekdays starting from Sun up to the first day of the
# month.
#
::itcl::body Calendar::first_week {month_idx year weekday} {
    return  [string repeat "   " $weekday]
}

::itcl::body Calendar::formatDayCell { day } { return [format %3d $day] }
::itcl::body Calendar::openRow { wkn } { return "" }
::itcl::body Calendar::closeRow { } { return "\n" }

# table <month> <year>: 

::itcl::body Calendar::table {month_idx year} {

    set wk 0
    set tbl [$this openRow $wk]  

    set month [lindex $month_names(en) $month_idx]
    set weekday [clock format [clock scan "1 $month $year"] -format %w]

    append  tbl [$this first_week $month_idx $year $weekday]

    scan [clock format [clock scan "1 $month $year"] -format %m] %d decm
    set maxd [numberOfDays $decm $year]

    for {set d 1} {$d <= $maxd} {incr d} {
    if {$weekday == 0} { 
        incr wk
        append tbl [$this openRow $wk] 
    }
        append tbl [formatDayCell $d]
        if {[incr weekday] > 6} {append tbl [$this closeRow]; set weekday 0}
    }
    return $tbl
}


# abstract base methods for starting and closing the output buffer.

::itcl::body Calendar::startOutput {} { return "" }
::itcl::body Calendar::closeOutput {} { return "" }

# cal <month> <year>: cal does the real heavy lifting of building the
# calendar table. cal is designed to be the most abstract possible: 
#   - the output buffer is initialized by startOutput (this class does nothing)
#   - the output buffer is filled with the header: in the classical Unix cal
#   command output this corresponds to the 2 lines showing the year, the month and
#   the weekdays
#   - the output buffer is appended filled with the actual table of days of the month
#   - the output is closed. This class does basically nothing

::itcl::body Calendar::cal {month_idx year} {

    set month_year_processed [list $month_idx $year]

    set     res     [$this startOutput]
    append  res     [$this header $month_idx $year]
    append  res     [$this table  $month_idx $year]
    append  res     [$this closeOutput]
    
    return $res

}

# emit args: 
#
# emit returns the text of the calendar. If one argument is passed
# to this method its value is taken as a year number and the whole
# calendar for that year is printed, thus cycling this same method
# for each month of the year and concatenating the output in a single 
# buffer. If 2 arguments are passed emit interprets them as month
# and year. <month> can be specified both in number (1-12) or 
# abbreviated name (Jan,Feb,....,Dec). A minimal support for other
# languages exists. If no arguments are passed to 'emit' the current
# month calendar is displayed.
#

::itcl::body Calendar::emit { args } {

    set argsnumber  [llength $args]

# if we have just one argument therefore it be an year and we proceed to
# generate a whole year calendar, otherwise we have to examine possible
# options and values

    if {$argsnumber > 1} {

    if {$argsnumber%2 == 0} {

        set primo_chr [string range [lindex $args 0] 0 0]
        if {$primo_chr == "-"} {

# we proceed to eval import_arguments $args
    
        set numeric_parameters  {}
        eval $this configure $args

        } else {

# arguments number is even. If the first switch is not an option (-opt)
# we assume we are passing 2 parameters to the methods, while the
# remaining list are actually an -opt val pairs list

# we assume the rest of the args are in the form -opt1 val1 -opt2 val2 ...
# we proceed to eval import_arguments [lrange $args 2 end]

        set numeric_parameters  [lrange $args 0 1]
        eval $this configure    [lrange $args 2 end]

        }
    } else {

# we assume the rest of the args are in the form -opt1 val1 -opt2 val2 ... 
# and then we eval import_arguments [lrange $args 1 end]

        set numeric_parameters  [lrange $args 0 0]
        eval $this configure    [lrange $args 1 end]
    }

    } else {
    set numeric_parameters $args
    }

    set argsnumber  [llength $numeric_parameters]

    switch $argsnumber {
    1 {

#   if only one argument is passed to this procedure then we treat it as either as a 
#   year (therefore must be a number) or a month name of the current year

        if {[regexp {^[0-9]+$} $numeric_parameters]} {
        set res {}
        set year $numeric_parameters
        for {set m 0} {$m < 12} {incr m} {
            append res [cal $m $year]\n\n
        }
        
        return [string trimright $res]
        }

        set month_idx [lsearch $month_names($language) $numeric_parameters]
        if {$month_idx >= 0} {
        set year [clock format [clock sec] -format %Y]
        return [cal $month_idx $year]
        } else {
        return ""
        }
    }
    2 {

# two args: the first is the month, the second the year.

        set month [lindex $numeric_parameters 0]
        set year  [lindex $numeric_parameters 1]        

        if  {[regexp {^\d{1,2}$} $month mat] && ($month > 0) && ($month <= 12)} {
        return [cal [incr month -1] $year]
        } elseif { [lsearch $month_names($language) $month] >= 0} {
        return [cal [lsearch $month_names($language) $month] $year]
        }
    }
    0 -
    default {

        # no arguments, we take today as reference
            
            scan [clock format [clock seconds] -format %m] "%d" month
        set year    [format "%d" [clock format [clock sec] -format %Y]]
        return      [cal [incr month -1] $year]

    }
    }

}

# XmlCalendar: XmlCalendar inherits the table structure of Calendar and 
# adds XML markup to a calendar table. The design is driven by the layout
# of a calendar table. This is probably a rather naive approach.  
# A better implementation would require separate data and layout classes,
# but it's only a calendar table anyway 

::itcl::class XmlCalendar {
    inherit Calendar

    private method  validateWeekday { wkd }

# dictionary of table generation parameters (tag , attributes). key for the dictionary can be
# 
#  - container: 
#  - header
#  - weekdays
#  - days_row
#  - days_cell
#
# for every key a 'tag' and 'attr' key is defined. attr is a even-length list storing 
# attribute-value pairs

    public variable parameters

# we are emitting (x)html code that has to be encapsulated
# in this root element. If the value is a list the first element is
# the tag name and the rest is treated as a list of <attr>,<value pairs
# so this list has to have an odd length 

# These public variables are listed in order to enable the corresponding configuration options:
#
# $calObj configure -current_day 4 -container table -banner ....
#
# They work as transit variables as the values are actually stored in the dictionary 'parameters'
#

 
    public  variable    container   {}  { $this expandValues container      $container }
    public  variable    header      {}  { $this expandValues header     $header }
    public  variable    body        {}  { $this expandValues body       $body }
    public  variable    foot        {}  { $this expandValues foot       $foot }
    public  variable    banner      {}  { $this expandValues banner     $banner }
    public  variable    banner_month    {}  { $this expandValues banner_month   $banner_month }
    public  variable    banner_year {}  { $this expandValues banner_year    $banner_year }
    public  variable    weekdays    {}  { $this expandValues wkdays_bar     $weekdays }
    public  variable    weekday_cell    {}  { $this expandValues wkday_cell     $weekday_cell }
    public  variable    days_row        {}  { $this expandValues days_row       $days_row }
    public  variable    days_cell   {}  { $this expandValues days_cell      $days_cell }
    public  variable    cell_function   ""
    public  variable    current_day 0
    public  variable    current_weekday -1  { $this validateWeekday $current_weekday }

    private method  expandValues { element values_list }

    protected method startOutput { } 
    protected method closeOutput { } 

    protected method mkOpenTag   { tag {attrib {}} }
    protected method mkCloseTag  { tag }
    
    protected method header  { mth yr }
    protected method table   { mth yr }
    protected method weekdays    { }
    protected method banner  { mth yr }
    protected method first_week  { mth yr wkday } 
    protected method openRow     { wkn }
    protected method closeRow    { }
    protected method formatDayCell { day } 
    protected method getParameters { param what }

    constructor {args} {Calendar::constructor $args} {

    set parameters [dict create container   {tag "calendar"     attr "" } \
                    header  {tag "calheader"    attr "" } \
                    body    {tag "calbody"      attr "" } \
                    foot    {tag "calfoot"      attr "" } \
                    banner  {tag "monthyear"    attr "" } \
                    banner_month {tag "month"       attr "" } \
                    banner_year {tag "year"     attr "" } \
                    wkdays_bar  {tag "weekdays"     attr "" } \
                    wkday_cell  {tag "wkday"        attr "" } \
                    days_row    {tag "week"     attr "" } \
                    days_cell   {tag "day"      attr "" }]
    }
}

::itcl::body XmlCalendar::getParameters {param what} {
    if {[dict exists $parameters $param $what]} {
    return [dict get $parameters $param $what]
    } else {
    return ""
    }
}

::itcl::body XmlCalendar::expandValues { element value_list } {

    dict set parameters $element tag    [lindex $value_list 0]
    dict set parameters $element attr   [lrange $value_list 1 end]

}

::itcl::body XmlCalendar::validateWeekday { wkd } {
    if {$wkd == "today"} {
    set current_weekday [clock format [clock scan today] -format %w]
    }
}

::itcl::body XmlCalendar::startOutput {} { 
    return [$this mkOpenTag  [getParameters container tag] [getParameters container attr]]
}

::itcl::body XmlCalendar::closeOutput {} { 
    return [$this mkCloseTag [getParameters container tag]]
}

::itcl::body XmlCalendar::mkOpenTag {tag {attrib {}}} {

    set open_tag "<$tag"
    foreach  {a v} $attrib {
        append open_tag " $a=\"$v\""
    }
    append open_tag ">"

    return $open_tag
}

::itcl::body XmlCalendar::mkCloseTag {tag} { return "</$tag>" }

# The Xml header is made of a banner (i.e Month Year) and
# a bar showing the weekdays with their markup.
# 


::itcl::body XmlCalendar::header {mth_idx yr} {
    set header_tag [getParameters header tag]
    set header_att [getParameters header attr]

    return "[mkOpenTag $header_tag $header_att][Calendar::header $mth_idx $yr][mkCloseTag $header_tag]\n"
}

::itcl::body XmlCalendar::weekdays { } {
    set rowtag  [getParameters wkdays_bar tag]
    set xml [mkOpenTag $rowtag]    

    set tagname [getParameters wkday_cell tag]
    set wdn 0
    foreach dn $day_names($language) {
        if {$wdn == $current_weekday} {
            append xml "[mkOpenTag $tagname {class current_wkday}]$dn[mkCloseTag $tagname]"
        } else {
            append xml "[mkOpenTag $tagname]$dn[mkCloseTag $tagname]"
        }
        incr wdn
    }
    append xml [mkCloseTag $rowtag]
    return $xml
}

::itcl::body XmlCalendar::banner {month_idx yr} {
    set month_name [lindex $month_names($language) $month_idx]

    set header_tag  [getParameters banner tag]

    set month_open_tag [mkOpenTag [getParameters banner_month tag] [getParameters banner_month attr]]
    set year_open_tag  [mkOpenTag [getParameters banner_year tag]  [getParameters banner_year attr]]

    set banner_html [mkOpenTag $header_tag]
    append banner_html  "${month_open_tag}${month_name}[mkCloseTag [getParameters banner_month tag]]"
    append banner_html  "${year_open_tag}$yr[mkCloseTag [getParameters banner_year tag]]"
    append banner_html  [mkCloseTag $header_tag]
    return $banner_html
}

::itcl::body XmlCalendar::formatDayCell { day } {
    set tagname [getParameters days_cell tag]
    set tagattr [getParameters days_cell attr]

    array set attributes $tagattr
    if {$day == $current_day} {
    set attributes(class) current
    }
    
    if {$cell_function != "" && $day != ""} {

    set month_year [$this cal_processed]    

    set month [lindex $month_names(en) [lindex $month_year 0]] 
    set year  [lindex $month_year 1] 
    set wkday [clock format [clock scan "$month $day $year"] -format %w]

    array set attributes [eval $cell_function $day $month_year $wkday]
    }

    set tagattr [array get attributes]
    return "[mkOpenTag $tagname $tagattr]$day[mkCloseTag $tagname]"   
}

::itcl::body XmlCalendar::first_week { mth yr wkday } {
    set emptyCell [formatDayCell ""]
    return  [string repeat $emptyCell $wkday]
} 

::itcl::body XmlCalendar::table {month_idx year} {
    set body_tag [getParameters body tag]
    set body_att [getParameters body attr]
    
    return "[mkOpenTag $body_tag $body_att][Calendar::table $month_idx $year][mkCloseTag $body_tag]\n"
}

::itcl::body XmlCalendar::openRow { wkn } {
    set tagname     [getParameters days_row tag]
    set attributes  [concat class week_${wkn} [getParameters days_row attr]]
    return [mkOpenTag $tagname $attributes]
}

::itcl::body XmlCalendar::closeRow {} {
    set tagname [getParameters days_row tag]
    return "[mkCloseTag $tagname]\n"
}


# HtmlCalendar: concrete class for generating Html formatted cal output.
#
#

::itcl::class HtmlCalendar {
    inherit XmlCalendar
    
    constructor {args} {XmlCalendar::constructor $args} {
        $this configure     -container      table   \
                            -header     thead   \
                            -body       tbody   \
                            -banner     tr      \
                            -banner_month   {th colspan 3 style "text-align: right;"} \
                            -banner_year    {th colspan 4 style "text-align: left;"}  \
                            -weekdays       tr      \
                            -weekday_cell   th      \
                            -days_row       tr      \
                            -days_cell      td 
    }
}

