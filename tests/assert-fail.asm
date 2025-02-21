;`errEassertion_failed
;`errEassertion_failed
;`errEassertion_failed
;E>ass fail 42
assert 0
org $8002
assert pc() < $8002
lbl: assert 0, "ass fail ",hex(6*11)
