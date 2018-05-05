;`10 20 30 40 FF
;`50 60 70 80 FF

org $008000

!mycompletedefine = $10,$20,$30,$40,$

!first = complete
!second = de
!third = ne

!fourth = !{second}fi!{third}

!fifth = ur

!combined = !{my!{first}!{fo!{fifth}th}}FF

db !combined

!{my!{first}!{fo!{fifth}th}} = $50,$60,$70,$80,$

db !combined
