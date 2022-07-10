;`errE5205
;`errE5205
;`errE5205
;`errE5206
;`errE5206
;`errE5206
;`errE5206
;`errE5206
;`errE5206
;`errE5098
;`errE5098
;`errE5098
;`errE5098
;`errE5098
;`errE5098
;`errE5106
;`errE5106
;`errE5181
;`errE5181

org $008000


db <what_arg>
db <^what_arg>
db <^^what_arg>

!test_def #= 1

db !^test_def


macro wrong_layer_parent(parent_arg, ...)
	db <^what_arg>
	db <^^what_arg>

	db !^^test_def

	macro wrong_layer_child(child_arg)
		db !^^^test_def
		db <^^what_arg>
		db <child_arg>+<parent_arg>
	endmacro

	macro wrong_layer_child_2(child_arg)
		db <^child_arg>+<^parent_arg>
	endmacro

	macro wrong_layer_child_3(child_arg)
		db <child_arg>+<^prent_arg> ; <- intentional typo
	endmacro

	macro wrong_layer_child_4(...)
		db <0>+<1>
	endmacro

	macro wrong_layer_inbeteen(unused, ...)
		macro wrong_layer_grand_child(child_arg, ...)
			db <parent_arg>
			db <^parent_arg>
			db <^^child_arg>
			db <2>
		endmacro
	endmacro
endmacro

%wrong_layer_parent($01, $11, $12, $13)
%wrong_layer_child($02)
%wrong_layer_child_2($03)
%wrong_layer_child_3($04)
%wrong_layer_child_4($05)
%wrong_layer_inbeteen($20, $21, $22)
%wrong_layer_grand_child($30, $31)



macro first_unclosed()
	macro second_unclosed()