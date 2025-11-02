assert ?(1, "a", "b") == "a"
assert ?(0, "a", "b") == "b"
assert ?(1, 1, 1/0) == 1 ; short-circuiting
assert ?(1, "a", 0) == "a" ; mixed types
