<?
# $Id$
# rivet page used with Rivet's rivet1-test

hgetvars

headers setcookie -name mod -value rivet -expires 01-01-2003 

# hello-1.1
hputs "Hello, World\n"

# i18n-1.1
hputs "� � � � � � - El Burro Sabe M�s Que T�!\n"

if { [ var number ] > 0 } {
    # get/post variables 1.1
    if { [ var exists foobar ] } {
	hputs "VARS(foobar) = [var get foobar]\n"
    }
    # get/post variables 1.{2,3}
    if { [ var exists M�s ] } {
	hputs "VARS(M�s) = [var get M�s]\n"
    }
}

# env
hputs "ENVS(DOCUMENT_NAME) = $ENVS(DOCUMENT_NAME)\n"

# cookies
if { [ array exists COOKIES ] } {
    foreach { ck } [ array names COOKIES ]  {
        hputs "COOKIES($ck) = $COOKIES($ck)\n"
    }
}

?>