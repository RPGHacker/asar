;`63 12 FD  63 12 FA
;`63 12 F7  63 12 F4
;`EA 12 60  EA 12 60
;`EA 12 60  EA 12 60
;`EA 12 60  EA 12 60
warnings disable Wfeature_deprecated
arch spc700-raw
org 0
L:
bbs1 $12.3,L : bbs $12.3,L
bbs1 $12.3,L : bbs3 $12,L
not1 c,$12.3 : not1 $12.3
not1 c,$12.3 : not3 c,$12
not1 c,$12.3 : not3 $12
