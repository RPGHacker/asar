assert "a" + "b" == "ab"
assert "" < "a"
assert "A" < "a"
assert "bbbbbbb" > "aaaaaaaaaaaa"
assert "a" >= "a"
assert "a" != "a" + "b"
assert "2" != 2

assert char( "a""b" , 1 ) == '"'
;P>asdf"""x
print "asdf""""""x"
