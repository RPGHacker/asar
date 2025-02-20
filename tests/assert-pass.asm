assert 1
org $8000
assert pc() < $8001
lbl: assert 1 == 1, "wat"
