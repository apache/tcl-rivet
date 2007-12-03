# \$Id\$
# Minimal config file for testing

# Parsed by makeconf.tcl

ServerRoot "$CWD"

PidFile "$CWD/httpd.pid"

ResourceConfig "$CWD/srm.conf"
AccessConfig "$CWD/access.conf"

Timeout 300

MaxRequestsPerChild 0

$LOADMODULES

Port 8081

ServerName localhost

DocumentRoot "$CWD"

<Directory "$CWD">
Options All MultiViews
AllowOverride All
Order allow,deny
Allow from all
</Directory>

<IfModule mod_dir.c>
DirectoryIndex index.html
</IfModule>

AccessFileName .htaccess

HostnameLookups Off

ErrorLog $CWD/error_log

LogLevel debug

LogFormat "%h %l %u %t \\"%r\\" %>s %b \\"%{Referer}i\\" \\"%{User-Agent}i\\"" combined
CustomLog "$CWD/access_log" combined
