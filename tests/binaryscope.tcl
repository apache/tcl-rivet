::rivet::headers type "application/octet-stream"

set before [list \
    [fconfigure stdout -translation] \
    [fconfigure stdout -encoding] \
    [fconfigure stdout -eofchar]]

::rivet::with_binary_output {
    puts -nonewline stdout [binary format H* 00ff]
}

::rivet::write_binary [binary format H* fe01]

set afterSuccess [list \
    [fconfigure stdout -translation] \
    [fconfigure stdout -encoding] \
    [fconfigure stdout -eofchar]]

catch {::rivet::with_binary_output {error expected}} message

set returnCode [catch {
    ::rivet::with_binary_output {return returned}
} returnMessage]

set afterError [list \
    [fconfigure stdout -translation] \
    [fconfigure stdout -encoding] \
    [fconfigure stdout -eofchar]]

puts -nonewline stdout [expr {
    $before eq $afterSuccess &&
    $before eq $afterError &&
    $message eq "expected" &&
    $returnCode == 2 &&
    $returnMessage eq "returned" ? "OK" : "BAD"
}]
