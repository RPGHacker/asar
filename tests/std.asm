;@B0 A0 90 80 70

incsrc "std/test.asm"

org $008000

if defined("stdincluded")
	db $B0
else
	error "Define 'stdincluded' not found! (Check std includes)."
endif

if defined("stddefined")
	db $A0
else
	error "Define 'stddefined' not found! (Check std defines)."
endif

if defined("stddefined2")
	db $90
else
	error "Define 'stddefined2' not found! (Check std defines)."
endif

if defined("stddefined3")
	db $80
else
	error "Define 'stddefined3' not found! (Check std defines)."
endif

if defined("stddefined4")
	db $70
else
	error "Define 'stddefined4' not found! (Check std defines)."
endif
