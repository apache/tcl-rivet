
$Id$

INTRODUCTION

This is session management code.  It provides an interface to allow you to
generate and track a browser's visit as a "session", giving you a unique
session ID and an interface for storing and retrieving data for that session
on the server.

This is an alpha/beta release -- documentation is not in final form, but
everything you need should be in this file.

Using sessions and their included ability to store and retrieve 
session-related data on the server, programmers can generate more
secure and higher-performance websites.  For example, hidden fields
do not have to be included in forms (and the risk of them being manipulated
by the user mitigated) since data that would be stored in hidden fields can 
now be stored in the session cache on the server.  Forms are then
faster since no hidden data is transmitted -- hidden fields must be
sent twice, once in the form to the broswer and once in the response from it.

Robust login systems, etc, can be built on top of this code.

REQUIREMENTS

Rivet.  Currently has only been tested with Postgresql.  All DB interfacing
is done through DIO, though, so it should be relatively easy to add support 
for other databases.

PREPARING TO USE IT

Create the tables in your SQL server.  With Postgres, do a "psql www" or
whatever DB you connect as, then a backslash-i on session-create.sql

(If you need to delete the tables, use session-drop.sql)

The session code by default requires a DIO handle called DIO (the name of 
which can be overridden).  We get it by doing a

    RivetServerConf ChildInitScript "package require DIO"
    RivetServerConf ChildInitScript "::DIO::handle Postgresql DIO -user www"

EXAMPLE USAGE

In your httpd.conf, add:

    RivetServerConf ChildInitScript "package require Session; Session SESSION"

This tells Rivet you want to create a session object named SESSION in every
child process Apache creates.

You can configure the session at this point using numerous key-value pairs 
(which are defined later in this doc).  Here's a quick example:

    RivetServerConf ChildInitScript "package require Session; Session SESSION -cookieLifetime 120 -debugMode 1"

Turn debugging on (-debugMode 1) to figure out what's going on -- it's really 
useful, if verbose.

In your .rvt file, when you're generating the <HEAD> section:

    SESSION activate

Activate handles everything for you with respect to creating new sessions, and
for locating, validating, and updating existing sessions.  Activate will 
either locate an existing session, or create a new one.  Sessions will 
automatically be refreshed (their lifetimes extended) as additional requests 
are received during the session, all under the control of the key-value pairs 
controlling the session object.

USING SESSIONS FROM YOUR CODE

The main methods your code will use are:

    SESSION id - After doing a "SESSION activate", this will return a 32-byte 
	ASCII-encoded random hexadecimal string.  Every time this browser 
	comes to us with a request within the timeout period, this same string 
	will be returned (assuming they have cookies enabled).

    SESSION is_new_session - returns 1 if it's a new session or 0 if it has
       previously existed (i.e. it's a zero if this request represents a 
       "return" or subsequent visit to a current session.)

    SESSION new_session_reason - this will return why this request is the first
	request of a new session, either "no_cookie" saying the browser didn't 
	give us a session cookie, "no_session" indicating we got a cookie but 
	couldn't find it in our session table, or "timeout" where they had a 
	cookie and we found the matching session but the session has timed out.

    SESSION store packageName key data - given the name of a package, a key,
	and some data.  Stores the data in the rivet session cache table.

    SESSION fetch packageName key - given a package name and a key, return 
	the data stored by the store method, or an empty string if none was 
	set.  (Status is set to the DIO error that occurred, it can be fetched 
	using the status method.)


SESSION CONFIGURATION OPTIONS

The following key-value pairs can be specified when a session object (like
SESSION above) is created:

sessionLifetime - how many seconds the session will live for.  7200 == 2 hours

sessionRefreshInterval - if a request is processed for a browser that currently
has a session and this long has elapsed since the session update time was
last updated, update it.  900 == 15 minutes.  so if at least 15 minutes has
elapsed and we've gotten a new request for a page, update the session update
time, extending the session lifetime (sessions that are in use keep getting
extended).

cookieName - name of the cookie stored on the user's web browser
    default rivetSession

dioObject - the name of the DIO object we'll use to access the database
(default DIO)

gcProbability - the probability that garbage collection will occur in percent.
(default 1%, 1) (not coded yet)

gcMaxLifetime - the number of seconds after which data will be seen as 
"garbage" and cleaned up -- defaults to 1 day (86400) (not coded yet)

refererCheck - the substring you want to check each HTTP referer for.  If 
the referer was sent by the browser and the substring is not found, the 
session will be deleted. (not coded yet)

entropyFile - the name of a file that random binary data can be read from.
("/dev/urandom")  Data will be used from this file to help generate a
super-hard-to-guess session ID.

entropyLength - The number of bytes which will be read from the entropy file.
If 0, the entropy file will not be read (default 0)

scrambleCode -  Set the scramble code to something unique for the site or 
your app or whatever, to slightly increase the unguessability of session ids
(default "some random string")

cookieLifetime - The lifetime of the cookie in minutes.  0 means until the 
browser is closed (I think). (default 0)

cookiePath - The webserver subpath that the session cookie applies to 
(defaults to /)

cookieDomain - The domain to set in the session cookie (not coded yet)

cookieSecure - specifies whether the cookie should only be sent over secure 
connections, 0 = any, 1 = secure connections only (default 0)

sessionTable - the name of the table that session info will be stored in
(default "rivet_session")

sessionCacheTable - the name of the table that contains cached session data
(default "rivet_session_cache")

debugMode - Set debug mode to 1 to trace through and see the session object do 
its thing (default 0)

debugFile - the file handle that debugging messages will be written to
(default stdout)


SESSION METHODS

The following methods can be invoked to find out stuff about the current
session, store and fetch server data identified with this session, etc:

SESSION status - return the status of the last operation

SESSION id - get the session ID of the current browser.  Returns an
empty string if there's no session (will not happen is SESSION activate
has been issued.)

SESSION new_session_reason - Returns the reason why there wasn't a previous
session, either "no_cookie" saying the browser didn't give us a session
cookie, "no_session" indicating we got a cookie but couldn't find it in
the session table, or "timeout" when we had a cookie and a session but
the session had timed out.

SESSION store packageName key data

Given a package name, a key string, and a data string, store the data in the 
rivet session cache.

SESSION fetch packageName key

Given a package name and a key, return the data stored by the store method,
or an empty string if none was set.  Status is set to the DIO error that
occurred, it can be fetched using the status method.


SESSION delete - given a user ID and looking at their IP address we inherited
from the environment (thanks, Apache), remove them from the session
table.  (the session table is how the server remembers stuff about sessions).

If the session ID was not specified the current session is deleted.


SESSION activate

Find and validate the session ID if they have one.  If they don't have one or
it isn't valid (timed out, etc), create a session and drop a cookie on them.


GETTING ADDITIONAL RANDOMNESS FROM THE ENTROPY FILE

    RivetServerConf ChildInitScript "Session SESSION -entropyFile /dev/urandom -entropyLength 10 -debugMode 1"

This options say we want to get randomness from an entropy file (random data 
pseudo-device) of /dev/urandom, to get ten bytes of random data from that 
entropy device, and to turn on debug mode, which will cause the SESSION object 
to output all manner of debugging information as it does stuff.  This has
been tested on FreeBSD and appears to work.

