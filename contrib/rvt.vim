" RVT (mod_dtcl) syntax file.
" Language:	Tcl + HTML
" Maintainer:	Wojciech Kocjan <zoro@nowiny.net>
" Filenames:	*.rvt

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if !exists("main_syntax")
  let main_syntax = 'rvt'
endif

if version < 600
  so <sfile>:p:h/html.vim
else
  runtime! syntax/html.vim
  unlet b:current_syntax
endif


syntax include @tclTop syntax/tcl.vim

syntax region rvtTcl keepend matchgroup=Delimiter start="<?" end="?>" contains=@tclTop

let b:current_syntax = "rvt"

if main_syntax == 'rvt'
  unlet main_syntax
endif
