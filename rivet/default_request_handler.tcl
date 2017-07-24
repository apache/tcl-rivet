# -- request_handler.tcl
#
# Copyright 2002-2017 The Apache Rivet Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#	http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# code of the default handler of HTTP requests

    ::try {
        ::Rivet::initialize_request
    } on error {err} {
        ::rivet::apache_log_error crit \
            "Rivet request initialization failed: $::errorInfo"
    }

    ::try {

        set script [::rivet::inspect BeforeScript]
        if {$script ne ""} {
            set ::Rivet::script $script
            eval $script
        }

        set script [::rivet::url_script]
        if {$script ne ""} {
            set ::Rivet::script $script
            namespace eval ::request $script
        }

        set script [::rivet::inspect AfterScript]
        if {$script ne ""} {
            set ::Rivet::script $script
            eval $script
        }

    } trap {RIVET ABORTPAGE} {err opts} {
        ::Rivet::finish_request $script $err $opts AbortScript
    } trap {RIVET THREAD_EXIT} {err opts} {
        ::Rivet::finish_request $script $err $opts AbortScript
    } on error {err opts} {
        ::Rivet::finish_request $script $err $opts
    } finally {
        ::Rivet::finish_request $script "" "" AfterEveryScript
    }
   
# default_request_handler.tcl ---
