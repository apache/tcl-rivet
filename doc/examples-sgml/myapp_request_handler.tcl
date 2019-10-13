# -- myapp_request_handler.tcl
#
# This script will be read by mod_rivet during the process/thread 
# initialization stage and its content stored in a Tcl_Obj object. 
# This object is evaluated internally by mod_rivet
#

::try {

    ::theApplication request_processing [::rivet::var_qs all]

} trap {RIVET ABORTPAGE} {err opts} {

     set abort_code [::rivet::abort_code]

    switch $abort_code {
       code1 {
           # handling abort_page with code1
           ....
       }
       code2 {
           # handling abort_page with code2
          ....      
       }
       # ...
       default {
           # default abort handler
       }
   }

} trap {RIVET THREAD_EXIT} {err opts} {
    
    # myApplication sudden exit handler
    ...

} on error {err opts} {

    # myApplication error handler
    ...

} finally {

    # request processing final elaboration
    
}

# -- myapp_request_handler.tcl
