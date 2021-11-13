;P>This is a simple print.
;P>This is a print with some functions: 01010101, 0B, 0123
;E>This is a user error.
;W>This is a user warning.
;E>Oh no, 0 is not equal to 1!
;E>This is a user error with some functions: 01010101, 0B, 0123
;W>This is a user warning with some functions: 01010101, 0B, 0123
;E>Oh no, 0 is not equal to select(1, 1, 0)! So here's some functions: 01010101, 0B, 0123
;`errE5158
;`errE5159
;`warnW1012

print "This is a simple print."
print "This is a print with some functions: ",bin(%01010101, 8),", ",hex($0B, 2),", ",dec(0123, 4)

error
warn
assert 0 == 1

error "This is a user error."
warn "This is a user warning."
assert 0 == 1, "Oh no, 0 is not equal to 1!"

error "This is a user error with some functions: ",bin(%01010101, 8),", ",hex($0B, 2),", ",dec(0123, 4)
warn "This is a user warning with some functions: ",bin(%01010101, 8),", ",hex($0B, 2),", ",dec(0123, 4)
assert 0 == select(1, 1, 0), "Oh no, 0 is not equal to select(1, 1, 0)! So here's some functions: ",bin(%01010101, 8),", ",hex($0B, 2),", ",dec(0123, 4)