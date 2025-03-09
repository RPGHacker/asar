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


;`errEno_labels_here
if ANonStaticLabel == $008000
	; This should fail
endif

;`errEno_labels_here
if TestStruct.count == 2
	; This should fail
endif

;`errEno_labels_here
if TestStruct[0].count == 2
	; This should fail
endif

;`errEno_labels_here
if TestStruct.NewStruct.new == 3
	; This should fail
endif

;`errEno_labels_here
if TestStruct.NewStruct[0].new == 3
	; This should fail
endif

OtherLabel = ANonStaticLabel ; this should be fine
;`errEno_labels_here
if OtherLabel == $008000
  ; but this should not
endif

;`errEno_labels_here
!adefine #= ANonStaticLabel
