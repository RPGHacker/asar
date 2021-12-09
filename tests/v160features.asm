;`00
;`EE
;`10
;`01
;`01
;`DD
;`01
;`00
;`00
;`01
;`78
;`21 61
;`5C 78
;`01 90 02
;`00 80 00
;`01 90 02
;`8a
;`ac

asar 1.60

lorom
org $008000


base $029001

BaseTest1:

pushbase

base off

BaseTest2:

pullbase

BaseTest3:

base off


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


!a = "x"

db "!a"
db "\!a"
db "\\!a"

dl BaseTest1
dl BaseTest2
dl BaseTest3

function readfile1_incremented(filename, pos) = readfile1(filename, pos)+1

db readfile1_incremented("data/64kb.bin",0)
db readfile1_incremented("data/64kb.bin",1)
