# Simple routine that takes files and spits out everything inside the
# <body></body> tags.

proc main { } {
    global argv
    puts {
	<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
	<html>
	<head>
	<title></title>
	<link rel="stylesheet" href="style.css">
	</head>
	<body>
    }
	
    foreach fl $argv {
	set ofl [ open $fl r ]
	set dump [ read $ofl ]
	set lines [ split $dump "\n" ]
	set inbody 0
	foreach ln $lines {
	    if { [ string first "<body>" $ln ] != -1 } {
		set inbody 1
	    } elseif { [ string first "</body>" $ln ] != -1 } { 
		set inbody 0
	    } else {
		if { $inbody == 1 } { puts $ln }
	    }
	}
    }
    puts {
	</body>
	</html>
    }    
}

main