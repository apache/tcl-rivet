# -- validate_mac 
#

    proc validate_mac {_mac_address_d} {
        upvar $_mac_address_d mac_address_d

        dict with mac_address_d {

            set var [string trim $var]
            if {[regexp {^[[:xdigit:]]{2}([:-][[:xdigit:]]{2}){5}$} $var]} {

                set var [string tolower $var]

                # we normalize the mac address to the Unix form.
                # The dash '-' characters in the windows representation 
                # are replaced by columns ':'

                set var [regsub -all -- {-} $var :]

                # the 'constrain' field is bidirectional:
                # it tells the validator to curb/change the value
                # within bonds/forms/representation. By setting it the
                # validator we tell the FormBroker to copy the value
                # back in the response array

                set constrain 1
                return FB_OK

            } else {

                return FB_WRONG_MAC

            }

        }
    }
 
