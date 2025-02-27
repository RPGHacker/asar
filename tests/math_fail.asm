;`errEdivision_by_zero
;`errEnan
;`errEnan
;`errEnegative_shift
;`errEnegative_shift
;`errEoob
;`errEoob
;`errEargument_count
;`errEargument_count
;`errEargument_count
;`errEfile_offset_out_of_bounds
;`errEfile_offset_out_of_bounds
print dec(1/0)
print dec(log(-0.5))
print dec(sqrt(-1))
assert 1 << -1
assert 1 >> -1
assert char("helloworld", -5)
assert char("helloworld", 10)
assert sqrt(1,2,3)
assert sqrt()
assert read1($1234, $42, 123)

assert readfile1("data/filename with spaces.bin", -1)
assert readfile1("data/filename with spaces.bin", 16)
