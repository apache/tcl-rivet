# \$Id\$
# Minimal config file for testing

# Parsed by makeconf.tcl

ServerType standalone

ServerRoot "$CWD"

PidFile "$CWD/httpd.pid"

ResourceConfig "$CWD/srm.conf"
AccessConfig "$CWD/access.conf"

Timeout 300

MaxRequestsPerChild 0

$LOADMODULES

LoadModule rivet_module $CWD/../src/mod_rivet[info sharedlibextension]

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

<IfModule mod_mime.c>
TypesConfig $CWD/mime.types

AddLanguage en .en
AddLanguage it .it
AddLanguage es .es
AddType application/x-httpd-rivet .rvt
AddType application/x-rivet-tcl .tcl
</IfModule>

RivetServerConf UploadFilesToVar on

<IfDefine SERVERCONFTEST>
RivetServerConf BeforeScript 'puts "Page Header"'
RivetServerConf AfterScript 'puts "Page Footer"'
</IfDefine>

<IfDefine DIRTEST>
<Directory />
RivetDirConf BeforeScript 'puts "Page Header"'
RivetDirConf AfterScript 'puts "Page Footer"'
</Directory>
</IfDefine>

