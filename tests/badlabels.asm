;`errE5075
;`errE5075
;`errE5059
;`errE5075
;`errE5075
org $008000
test[2]:
struct asdf
	.othertest[2]:
	othertest2[2]:
endstruct

lol:
.asdf[2]
.asdf[2]:

struct ddd
	.nice: skip 69
endstruct

lda ddd[420].nice	;should not error, but since there are other errors, no bytes written
