;` 02 02 02 20 20 20 20 20 2C 20 09 20 00 01 01 00 EA



org $008000
function lol(asdf,	qwer) = asdf*qwer

db lol(1,	 2)
db lol(1,	 2),	  lol(1,	 2)
db "     , 	 "

macro add_text(...)
    !i #= 0
    while !i < sizeof(...)
        if <...[!i]> == 'A'
            db $00
        elseif <...[!i]> == 'B'
            db $01
        elseif <...[!i]> == 'C'
            db $02
        elseif <...[!i]> == 'D'
            db $03
        else
            db $04
        endif

        !i #= !i+1
    endwhile
endmacro

%add_text('A', 'B', 'B', 'A')


macro variadic(...)
    if sizeof(...) > 0
        !i #= 0
        while !i < sizeof(...)
            .<...[!i]>:
            !i #= !i+1
        endwhile
    endif
endmacro

MainLabel:

%variadic()
%variadic(Sub1)
%variadic(Sub2, Sub3)

macro asdf(...)
!a = "elseif"
if 0
!a <...[1]>
endif
endmacro
%asdf(1, 1)



macro a(...)
!i = 0
if 0
elseif 42 == <...[!i]>
nop
else
error
endif
endmacro
%a(42)
