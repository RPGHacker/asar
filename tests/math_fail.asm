;`errEdivision_by_zero
print dec(1/0)
;`errEnan
print dec(log(-0.5))
;`errEnan
print dec(sqrt(-1))
;`errEnegative_shift
assert 1 << -1
;`errEnegative_shift
assert 1 >> -1
;`errEoob
assert char("helloworld", -5)
;`errEoob
assert char("helloworld", 10)
;`errEargument_count
assert sqrt(1,2,3)
;`errEargument_count
assert sqrt()
;`errEargument_count
assert read1($1234, $42, 123)

;`errEfile_offset_out_of_bounds
assert readfile1("data/filename with spaces.bin", -1)
;`errEfile_offset_out_of_bounds
assert readfile1("data/filename with spaces.bin", 16)

;`errEbad_type
assert "1" + 2
;`errEbad_type
assert datasize(2)
;`errEbad_type
assert "1" - "2"
;`errEbad_type
assert "1"%"2"
