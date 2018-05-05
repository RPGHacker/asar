;`B0 A0 90 80 70 60 50 40 30 20 10 F0 E0

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

if defined("stddefined5")
	; There is no space here on purpose so that we can test if quoted defines work properly
	db!stddefined5
else
	error "Define 'stddefined5' not found! (Check std defines)."
endif

if defined("cmddefined")
	db $30
else
	error "Define 'cmddefined' not found! (Check command line defines)."
endif

if defined("cmddefined2")
	db $20
else
	error "Define 'cmddefined2' not found! (Check command line defines)."
endif

if defined("cmddefined3")
	; There is no space here on purpose so that we can test if quoted defines work properly
	db!cmddefined3
else
	error "Define 'cmddefined3' not found! (Check command line defines)."
endif
