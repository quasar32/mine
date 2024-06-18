if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
inoremap <C-U> u
nnoremap "*p :let @"=substitute(system("wl-paste --no-newline --primary"),  '', '', 'g')p
nnoremap "+p :let @"=substitute(system("wl-paste --no-newline"),  '', '', 'g')p
xnoremap "+y y:call system("wl-copy", @")
nmap Q gq
xmap Q gq
omap Q gq
xmap gx <Plug>NetrwBrowseXVis
nmap gx <Plug>NetrwBrowseX
xnoremap <silent> <Plug>NetrwBrowseXVis :call netrw#BrowseXVis()
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#BrowseX(netrw#GX(),netrw#CheckIfRemote(netrw#GX()))
tnoremap <silent> <Plug>(fzf-normal) 
tnoremap <silent> <Plug>(fzf-insert) i
nnoremap <silent> <Plug>(fzf-normal) <Nop>
nnoremap <silent> <Plug>(fzf-insert) i
inoremap  u
let &cpo=s:cpo_save
unlet s:cpo_save
set backspace=indent,eol,start
set backupdir=~/.cache/vim/backup//
set directory=~/.cache/vim/swap//
set display=truncate
set fileencodings=ucs-bom,utf-8,default,latin1
set helplang=en
set history=200
set incsearch
set langnoremap
set nolangremap
set mouse=nvi
set nrformats=bin,hex
set ruler
set scrolloff=5
set showcmd
set suffixes=.bak,~,.o,.info,.swp,.aux,.bbl,.blg,.brf,.cb,.dvi,.idx,.ilg,.ind,.inx,.jpg,.log,.out,.png,.toc
set ttimeout
set ttimeoutlen=100
set undodir=~/.cache/vim/undo//
set wildmenu
" vim: set ft=vim :
