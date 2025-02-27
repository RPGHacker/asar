assert round(sin(1),2) == 0.84

assert 1 || (1/0) ; short-circuiting, shouldn't error
assert (0 && (1/0)) == 0

assert 0.1 + 0.3 == 0.4
assert 0.1 + 0.2 != 0.3

assert 0 != 1
assert 1 == 1 != 0

assert log2(8) == 3.0
assert log10(10) == 1.0

assert ceil(0.5) == 1
assert ceil(1) == 1
assert ceil(1.0) == 1
assert floor(0.5) == 0
assert floor(-0.1) == -1

assert stringlength("helloworld") == 10
assert char("helloworld", 0) == 'h'

assert canread(42, $8000) == 0
; i suppose this kinda makes sense....
assert canread(0, $8000) == 1
; this doesn't tho
assert canread(0, $6969) == 0
; okay never mind
assert canread(0, $8001) == 0
