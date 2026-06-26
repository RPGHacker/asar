function doch(c) = ?(c=='a',$8000, ?(c=='x',$4000, ?(c=='l',$2000, ?(c=='r',$1000, 1/0))))
function buttons_impl(s,i) = ?(i<stringlength(s), doch(char(s,i))|buttons_impl(s,i+1),0)
function buttons(s) = buttons_impl(s, 0)
org $8000
;`a9 00 d0
lda #buttons("axr")

if buttons("a") != $8000
	error "oops"
endif
