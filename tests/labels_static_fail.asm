;`errElabel_in_conditional
;`errElabel_in_conditional
;`errElabel_in_conditional
;`errElabel_in_conditional
;`errElabel_in_conditional
;`errEno_labels_here
;`errEno_labels_here
;`errElabel_cross_assignment
;`errEno_labels_here
;`warnWfeature_deprecated
;`warnWfeature_deprecated
;`errEdefine_label_math
org $008000
ANonStaticLabel:

struct TestStruct
	.first: skip 1
	.second: skip 1
	.count: skip 1
endstruct

struct NewStruct extends TestStruct
	.new: skip 1
endstruct


if ANonStaticLabel == $008000
	; This should fail
endif

if TestStruct.count == 2
	; This should fail
endif

if TestStruct[0].count == 2
	; This should fail
endif

if TestStruct.NewStruct.new == 3
	; This should fail
endif

if TestStruct.NewStruct[0].new == 3
	; This should fail
endif

incbin "data/64kb.bin":ANonStaticLabel..$8002
incbin "data/64kb.bin":$8000..ANonStaticLabel+2

ANewLabel = ANonStaticLabel

Here:
fillbyte Here
fill ANonStaticLabel

padbyte Here
pad ANonStaticLabel

!adefine #= ANonStaticLabel
