;`+
;`07606 5C 00 80 91 00 80 92
;`07FD7 0A
;`87FF8
;`53 54 41 52 03 00 FC FF
;`AF 56 34 12
;`8FFF8
;`53 54 41 52 03 00 FC FF
;`AF 56 34 12
;`FFFFF 00
org $00F606
autoclean JML Mymain
autoclean dl Mymain2
freecode align
Mymain:
LDA $123456
freecode align
Mymain2:
LDA $123456
