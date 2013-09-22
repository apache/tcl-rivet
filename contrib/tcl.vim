" Syntax highlighting extention to include also the Rivet command set and various arguments accepted
" Language: Tcl
" Maintainer: Massimo Manghi <mxmanghi@apache.org>
" Filenames: *.tcl
"
" Place this file in ~/.vim/after/syntax to have it loaded after
" the system wide vim syntax highlighting gets loaded
"

syn keyword rivetArguments      get list exists number all set exists unset names 
syn keyword rivetArguments      array_get clear redirect add type numeric channel 
syn keyword rivetArguments      save data size type filename
syn keyword rivetCommand        abort_code abort_page apache_log_error apache_table clock_to_rfc850_gmt
syn keyword rivetCommand        cookie debug env escape_sgml_chars escape_string escape_shell_command
syn keyword rivetCommand        headers html http_accept import_keyvalue_pairs include inspect
syn keyword rivetCommand        lempty lmatch load_cookies load_env load_headers load_response makeurl
syn keyword rivetCommand        no_body parray parse raw_post read_file unescape_string upload
syn keyword rivetCommand        var_qs var_post var wrap wrapline
syn match   rivetNamespace      "::rivet::"

if version >= 508 || !exists("did_tcl_syntax_inits")
  if version < 508
    let did_tcl_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink rivetCommand       Statement
  HiLink rivetArguments     Special
  highlight rivetNamespace  ctermfg=White guifg=Yellow guibg=Red

  delcommand HiLink
endif
let b:current_syntax = "tcl"

"highlight rivetCommand      ctermfg=Green guifg=Green
"highlight rivetCndRef       ctermfg=Green guifg=Green

