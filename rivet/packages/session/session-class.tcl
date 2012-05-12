#
# Session - Itcl object for web session management for Rivet
#
# $Id$
#

# Copyright 2004 The Apache Software Foundation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

package provide Session 1.0
package require Itcl

::itcl::class Session {
    # true if the page being processed didn't have a previous session
    public variable isNewSession 1

    # contains the reason why this session is a new session, or "" if it isn't
    public variable newSessionReason ""

    # the routine that will handle saving data, could use DIO, could use
    # flatfiles, etc.
    public variable saveHandler ""

    # the name of the DIO object that we'll use to access the database
    public variable dioObject "DIO"

    # the name of the cookie used to set the session ID
    public variable cookieName "rivetSession"

    # the probability that garbage collection will occur in percent.
    public variable gcProbability 1

    # the number of seconds after which data will be seen as "garbage"
    # and cleaned up -- defaults to 1 day
    public variable gcMaxLifetime 86400

    # the substring you want to check each HTTP referer for.  If the
    # referer was sent by the browser and the substring is not found,
    # the session will be deleted.
    public variable refererCheck ""

    public variable entropyFile "/dev/urandom"

    # the number of bytes which will be read from the entropy file
    public variable entropyLength 0

    # set the scramble code to something unique for the site or the
    # app or whatever, to slightly increase the unguessability of
    # session ids
    public variable scrambleCode "some random string"

    # the lifetime of the cookie in minutes.  0 means until the browser
    # is closed.
    public variable cookieLifetime 0

    # the lifetime of the session in seconds.  this will be updated if
    # additional pages are fetched while the session is still alive.
    public variable sessionLifetime 7200

    # if a request is being processed, a session is active, and this many
    # seconds have elapsed since the session was created or the session
    # update time was last updated, the session update time will be updated
    # (the session being in use extends the session lifetime)
    public variable sessionRefreshInterval 900

    # the webserver subpath that the session cookie applies to -- defaults
    # to /
    public variable cookiePath "/"

    # the domain to set in the session cookie
    public variable cookieDomain ""

    # the status of the last operation, "" if ok
    public variable status

    # specifies whether cookies should only be sent over secure connections
    public variable cookieSecure 0

    # specifies whether cookies should only be sent over http connections
    public variable cookieHttpOnly 0

    # the name of the table that session info will be stored in
    public variable sessionTable "rivet_session"

    # the name of the table that contains cached session data
    public variable sessionCacheTable "rivet_session_cache"

    # the file that debug messages will be written to
    public variable debugFile stdout

    # set debug mode to 1 to trace through and see the session object
    # do its thing
    public variable debugMode 1

    constructor {args} {
	eval configure $args
    	$dioObject registerSpecialField rivet_session session_update_time NOW
	$dioObject registerSpecialField rivet_session session_start_time NOW
    }

    method status {args} {
	if {$args == ""} {
	    return $status
	}
	set status $args
    }

    # get_entropy_bytes - read entropyLength bytes from a random data
    # device, such as /dev/random or /dev/urandom, available on some
    # systems as a way to generate random data
    #
    # if entropyLength is 0 (the default) or entropyFile isn't defined
    # or doesn't open successfully, returns an empty string
    #
    method get_entropy_bytes {} {
	if {$entropyLength == 0 || $entropyFile == ""} {
	    return ""
	}
	if {[catch {open $entropyFile} fp] == 1} {
	    return ""
	}

	set entropyBytes [read $fp $entropyLength]
	close $fp
	if {[binary scan $entropyBytes h* data]} {
	    debug "get_entropy_bytes: returning '$data'"
	    return $data
	}
	error "software bug - binary scan behaved unexpectedly"
    }

    #
    # gen_session_id - generate a session ID by md5'ing as many things
    # as we can get our hands on.
    #
    method gen_session_id {args} {
	package require md5

	# if the Apache unique ID module is installed, the environment
	# variable UNIQUE_ID will have been set.  If not, we'll get an
	# empty string, which won't hurt anything.
	set uniqueID [env UNIQUE_ID]

	set sessionIdKey "$uniqueID[clock clicks][pid]$args[clock seconds]$scrambleCode[get_entropy_bytes]"
	debug "gen_session_id - feeding this to md5: '$sessionIdKey'"
	return [::md5::md5 -hex -- $sessionIdKey]
    }

    #
    # do_garbage_collection - delete dead sessions from the session table.
    #    corresponding session table cache entries will automatically be
    #    deleted as well (assuming they've been defined with ON DELETE CASCADE)
    #
    method do_garbage_collection {} {
	debug "do_garbage_collection: performing garbage collection"
#	set result [$dioObject exec "delete from $sessionTable where timestamp 'now' - session_update_time > interval '$gcMaxLifetime seconds';"]
	set del_cmd "delete from $sessionTable where "
	append del_cmd [$dioObject makeDBFieldValue $sessionTable session_update_time now SECS]
	append del_cmd " - [$dioObject makeDBFieldValue $sessionTable session_update_time {} SECS]"
	append del_cmd " > $gcMaxLifetime"
	debug "do_garbage_collection: > $del_cmd  <"
	set result [$dioObject exec $del_cmd]
	$result destroy
    }

    #
    # consider_garbage_collection - perform a garbage collection gcProbability
    #   percent of the time.  For example, if gcProbability is 1, about 1 in
    #   every 100 times this routine is called, garbage collection will be
    #   performed.
    #
    method consider_garbage_collection {} {
	if {rand() <= $gcProbability / 100.0} {
	    do_garbage_collection
	}
    }

    #
    # set_session_cookie - set a session cookie to the specified value --
    #  other cookie attributes are controlled by variables defined in the
    #  object
    #
    method set_session_cookie {value} {
	cookie set $cookieName $value \
	    -path $cookiePath \
	    -minutes $cookieLifetime \
	    -secure $cookieSecure \
	    -HttpOnly $cookieHttpOnly
    }

    #
    # id - get the session ID of the current browser
    #
    # returns a session ID if their session cookie matches a current session.
    # returns an empty string if they do not have a session.
    #
    # status will be set to an empty string if all is ok, "timeout" if
    # the session had timed out, "no_cookie" if no cookie was previously
    # defined (session id could still be valid though -- first visit)
    #
    # ...caches the results in the info array to avoid calls to the database
    # in subsequent requests for the user ID from the same page, a common
    # occurrence.
    #
    method id {} {
	::request::global sessionInfo

	status ""

	# if we already know the session ID, we're done.
	# (i.e. we've already validated them earlier in the 
	# handling of the current page.)

	if {[info exists sessionInfo(sessionID)]} { 
	    debug "id called, returning cached ID '$sessionInfo(sessionID)'"
	    return $sessionInfo(sessionID) 
	}

	#
	# see if they have a session cookie.  if they don't,
	# set status and return.
	#
	set sessionCookie [cookie get $cookieName]
	if {$sessionCookie == ""} {
	    # they did not have a cookie set, they are not logged in
	    status "no_cookie"
	    debug "id: no session cookie '$cookieName'"
	    return ""
	}

	# there is a session Cookie, grab the remote address of the connection,
	# see if our state table says he has logged into us from this
	# address within our login timeout window and we've given him
	# this session

	debug "id: found session cookie '$cookieName' value '$sessionCookie'"

	set a(session_id) $sessionCookie
	set a(ip_address) [env REMOTE_ADDR]

	# see if there's a record matching the session ID cookie and
	# IP address
	set kf [list session_id ip_address]
	set key [$dioObject makekey a $kf]
	if {![$dioObject fetch $key a -table $sessionTable -keyfield $kf]} {
	    debug "id: no entry in the session table for session '$sessionCookie' and address [env REMOTE_ADDR]: [$dioObject errorinfo]"
	    status "no_session"
	    return ""
	}

	## Carve the seconds out of the session_update_time field in the
	# $sessionTable table.  Trim off the timezone at the end.
	set secs [clock scan [string range $a(session_update_time) 0 18]]

	# if the session has timed out, delete the session and return -1

	if {[expr $secs + $sessionLifetime] < [clock seconds]} {
	    $dioObject delete $key -table $sessionTable -keyfield $kf
	    debug "id: session '$sessionCookie' timed out"
	    status "timeout"
	    return ""
	}

	# Their session is still alive.  If the session refresh 
	# interval time has expired, update the session update time in the 
	# database (we don't update every time they request a page for 
	# performance reasons)  The idea is it's been at least 15 minutes or 
	# something like that since they've logged in, and they're still 
	# doing stuff, so reset their session update time to now

	if {[expr $secs + $sessionRefreshInterval] < [clock seconds]} {
	    debug "session '$sessionCookie' alive, refreshing session update time"
	    set a(session_update_time) now
	    if {![$dioObject store a -table $sessionTable -keyfield $kf]} {
		debug "id: Failed to store $sessionTable: [$dioObject errorinfo]"
		puts "Failed to store $sessionTable: [$dioObject errorinfo]"
	    }
	}

	#
	# THEY VALIDATED.  Cache the session ID in the sessionInfo array
	# that will only exist for the handling of this request, set that
	# this is not a new session (at least one previous request has been
	# handled with this session ID) and return the session ID
	#
	debug "id: active session, '$a(session_id)'"
	set sessionInfo(sessionID) $a(session_id)
	set isNewSession 0
	return $a(session_id)
    }

    #
    # store - given a package name, a key string, and a data string,
    #  store the data in the rivet session cache
    #
    method store {packageName key data} {
	set a(session_id) [id]
	set a(package_) $packageName
	set a(key_) $key

	regsub -all {\\} $data {\\\\} data
	set a(data) $data

	debug "store session data, package_ '$packageName', key_ '$key', data '$data'"
	set kf [list session_id package_ key_]

	if {![$dioObject store a -table $sessionCacheTable -keyfield $kf]} {
	    puts "Failed to store $sessionCacheTable '$kf'"
	    parray a
	    error [$dioObject errorinfo]
	}
    }

    #
    # fetch - given a package name and a key, return the data stored
    #   for this session
    #
    method fetch {packageName key} {
	set kf [list session_id package_ key_]

	set a(session_id) [id]
	set a(package_) $packageName
	set a(key_) $key

	set key [$dioObject makekey a $kf]
	if {![$dioObject fetch $key a -table $sessionCacheTable -keyfield $kf]} {
	    status [$dioObject errorinfo]
	    debug "error: [$dioObject errorinfo]"
	    debug "fetch session data failed, package_ '$packageName', key_ '$key', error '[$dioObject errorinfo]'"
	    return ""
	}

	debug "fetch session data succeeded, package_ '$packageName', key_ '$key', result '$a(data)'"

	return $a(data)
    }

    #
    # delete - given a user ID and looking at their IP address we inherited
    # from the environment (thanks, webserver), remove them from the session
    # table.  (the session table is how the server remembers stuff about
    # sessions)
    #
    method delete_session {{session_id ""}} {
	variable conf

	set ip_address [env REMOTE_ADDR]

	if {$session_id == ""} {
	    set session_id [id]
	}

	debug "delete session $session_id"

	set kf [list session_id ip_address]
	$dioObject delete [list $session_id $ip_address] -table $sessionTable -keyfield $kf

	## NEED TO delete saved session data here too, from the
	# $sessionCacheTable structure.
    }

    #
    # create_session - Generate a session ID and store the session in the
    #  session table.
    #
    # returns the session_id
    #
    method create_session {} {
	global conf

	## Create their session by storing their session information in 
	# the session table.
	set a(ip_address) [env REMOTE_ADDR]
	set a(session_start_time) now
	set a(session_update_time) now

	set a(session_id) [gen_session_id $a(ip_address)]

	set kf [list ip_address session_id]
	if {![$dioObject store a -table $sessionTable -keyfield $kf]} {
	    debug "Failed to store $sessionTable: [$dioObject errorinfo]"
	    puts "Failed to store $sessionTable: [$dioObject errorinfo]"
	}

	debug "create_session: ip $a(ip_address), id '$a(session_id)'"

	return $a(session_id)
    }

    #
    # activate - find the session ID if they have one.  if they don't, create
    # one and drop a cookie on them.
    #
    method activate {} {
	::request::global sessionInfo

	debug "activate: checking out the situation"

	# a small percentage of the time, try to delete stale session data
	consider_garbage_collection

	set id [id]
	if {$id != ""} {
	    debug "activate: returning session id '$id'"
	    return $id
	}

	# it's a new session, save the reason for why it's a new session,
	# set that it's a new session, drop a session cookie on the browser
	# that issued this request, set the session ID cache variable, and 
	# return the cookie ID
	set newSessionReason [status]
	debug "activate: new session, reason '$newSessionReason'"
	set id [create_session]
	set isNewSession 1
	set_session_cookie $id
	set sessionInfo(sessionID) $id
	debug "activate: created session '$id' and set cookie (theoretically)"
	return $id
    }

    #
    # is_new_sesion - return a 1 if it's a new session, else a zero if there
    # were one or more prior pages creating and/or using this session ID
    #
    method is_new_session {} {
	return $isNewSession
    }

    #
    # new_session_reason - return the reason why a session is new, either
    # it didn't have a cookie "no_cookie", there was a cookie but no
    # matching session "no_session", or there was a cookie and a session
    # but the session has timed out "timeout".  if the session isn't new,
    # returns ""
    #
    method new_session_reason {} {
	return $newSessionReason
    }

    #
    # debug - output a debugging message
    #
    method debug {message} {
	if {$debugMode} {
	    puts $debugFile "$this (debug) $message<br>"
	    flush $debugFile
	}
    }
}

