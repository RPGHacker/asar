;`errEunknown_operator
;`errEunknown_operator
;`errEunknown_operator

; RPG Hacker: Is this text actually supposed to fail, or was 0x supported by Asar at some point?
org $008000
AND #~0x8000
db ~0x80
db 0x80
db 010