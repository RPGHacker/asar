;`errEbroken_label_definition
;`errEbroken_label_definition
;`errEbroken_label_definition
;`errEbroken_label_definition
;`errEinvalid_label_name
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
