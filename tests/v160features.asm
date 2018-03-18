;@00
;@EE
;@10
;@01
;@01
;@DD
;@01
;@00
;@00
;@01

@asar 1.60

lorom
org $008000

; This file should not exist
if getfilestatus("bogusfilename.lol") == 0
	db $FF
endif

db canreadfile1("bogusfilename.lol", 0)
db readfile1("bogusfilename.lol", 0, $EE)

; This file SHOULD exist
if getfilestatus("data/filename with spaces.bin") == 0
	db filesize("data/filename with spaces.bin")
endif

db canreadfile1("data/filename with spaces.bin", 0)
db readfile1("data/filename with spaces.bin", 0, $EE)
db readfile1("data/filename with spaces.bin", 20, $DD)

!testdefine = "poop"
db defined("testdefine")
db defined("shouldntexist")

undef "testdefine"

db defined("testdefine")

!testdefine = "oppo"

db defined("testdefine")