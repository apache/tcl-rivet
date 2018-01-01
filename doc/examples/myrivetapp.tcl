## myrivetapp.tcl -- 
#
# Application class definition and instance creation
#

package require Itcl

::itcl::class MyRivetApp {

   private variable application_name

   public method init {}
   public method request_processing {urlencoded_args}
   public method abort_handler {abort_code}
   public method error_handler {error_info}

}

::itcl::body MyRivetApp::init {app_name}{

   # any initialization steps must go here
   # ....

   set application_name $app_name

}

::itcl::body MyRivetApp::request_processing {urlencoded_args} {

   # the whole application code will run from this method
   ...

}

MyRivetApp ::theApplication

::theApplication init [dict get [::rivet::inspect server] hostname]

# -- myrivetapp.tcl
