assert 1
org $8000
assert pc() < $8001
assert lbl != 0
lbl: assert 1 == 1, "wat"
; math errors in non-failing assert error messages ought to not throw
assert 1, "lol", dec(1/0)
