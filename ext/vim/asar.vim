" asar syntax highlighting for vim/neovim.
" i trust that if you use vim you know how to install syntax files.

syn case ignore
syn keyword asarCmd lorom hirom exlorom exhirom sa1rom fullsa1rom sfxrom norom endmacro struct endstruct extends incbin incsrc fillbyte fillword filllong filldword fill padbyte pad padword padlong paddword cleartable skip namespace print org base on off reset freespaceuse pc bytes freespace freecode freedata ram noram align cleaned static autoclean prot pushpc pullpc pushbase pullbase function if else elseif endif while endwhile for endfor assert arch 65816 spc700 superfx bankcross full half bank noassume auto asar includefrom includeonce include error double pushtable pulltable undef check title nested warn address dpbase optimize dp none always default mirrors global spcblock endspcblock nspc custom execute offset pushns pullns segment start pin norats freespacebyte
syn match asarMacroDefStart "macro " contained
syn match asarMacroDef "macro [0-9a-z_]\+\ze(" contains=asarMacroDefStart
syn keyword asarInsn db dw dl dd r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15
syn match asarInsnSuffix "\.[bwl]" contained
" TODO: some of this stuff probably shouldn't allow a suffix lol
syn keyword asarInsn nextgroup=asarInsnSuffix adc and asl bcc blt bcs bge beq bit bmi bne bpl bra brk brl bvc bvs clc cld cli clv cmp cop cpx cpy dec dea dex dey eor inc ina inx iny jmp jml jsr jsl lda ldx ldy lsr mvn mvp nop ora pea pei per pha phb phd phk php phx phy pla plb pld plp plx ply rep rol ror rti rtl rts sbc sec sed sei sep sta stp stx sty stz tax tay tcd tcs tdc trb tsc tsb tsx txa txs txy tya tyx wai wdm xba xce add alt1 alt2 alt3 asr bic cache cmode color div2 fmult from getb getbh getbl getbs getc hib ibt iwt ldb ldw link ljmp lm lms lmult lob loop merge mult not or plot ramb romb rpix sbk sex sm sms stb stop stw sub swap to umult with xor addw ya and1 bbc0 bbc1 bbc2 bbc3 bbc4 bbc5 bbc6 bbc7 bbs0 bbs1 bbs2 bbs3 bbs4 bbs5 bbs6 bbs7 call cbne clr0 clr1 clr2 clr3 clr4 clr5 clr6 clr7 clrc clrp clrv cmpw daa das dbnz decw di div ei eor1 incw mov sp mov1 movw mul not1 notc or1 pcall pop push ret reti set0 set1 set2 set3 set4 set5 set6 set7 setc setp sleep subw tcall tclr tset xcn lea move moves moveb movew
" this is separate just so the spc `push` instructions aren't highlighted as commands
syn match asarCmd "warnings\s\+\(push\|pull\|enable\|disable\)\?"
syn match asarString "\".\{-}\"" contains=asarMacroArg,asarDefine
syn match asarString "'.'"
syn match asarComment ";.*$"
syn region asarComment start=";\[\[" end="\]\]"
syn match asarOperator "(\|)\|\[\|\]\|+\|-\|\*\|\/\|%\|<\|>\|&\||\|\^\|\~\|#=\|:=\|?=\|!=\|="
syn match asarFunction "[0-9a-z_]\+\ze("
syn match asarDecNumber "\<[0-9]\+\(\.[0-9]\+\)\?\>"
syn match asarBinNumber "%[0-1]\+\>"
syn match asarHexNumber "\$[0-9a-f]\+\>"
syn match asarMacroCall "%[0-9a-z_]\+\ze("
syn match asarDefine "!\^*[0-9a-z_]\+"
syn region asarDefine start="!\^*{" end="}" contains=asarDefine,asarMacroArg
syn match asarMacroArg "<\^*[a-z_][0-9a-z_]\+>"
syn region asarMacroArg start="<\^*\.\.\.\[" end="\]>" contains=asarDefine,asarMacroArg
hi link asarMacroDefStart asarCmd
hi link asarMacroDef asarMacroCall
hi def link asarInsnSuffix asarInsn

hi def link asarCmd Keyword
hi def link asarComment Comment
hi def link asarString String
hi def link asarDecNumber Number
hi def link asarBinNumber Number
hi def link asarHexNumber Number
hi def link asarOperator Operator
hi def link asarInsn Identifier
hi def link asarDefine Define
hi def link asarMacroArg Define
" idk what to really use for this - this one is ok with my colorscheme
hi def link asarFunction Special
" slightly hacky as usual, but i want macro calls to be different and usually
" there's nothing other than type that's available and unused
hi def link asarMacroCall Type
