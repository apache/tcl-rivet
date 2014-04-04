
array set ::failtest { env 0 makeurl 0 } 
if {[catch {::rivet::env HTTP_HOST}]} { set ::failtest(env) 1 } 
if {[catch {::rivet::makeurl}]} { set ::failtest(makeurl) 1 } 
