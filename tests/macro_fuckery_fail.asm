;`errE5205
;`errE5205
;`errE5099
;`errE5099
;`errE5099
;`errE5106
;`errE5106

org $008000


<what_arg>
<^what_arg>


macro wrong_layer_parent(parent_arg)
	macro wrong_layer_child(child_arg)
		db <child_arg>+<parent_arg>
	endmacro

	macro wrong_layer_child_2(child_arg)
		db <^child_arg>+<^parent_arg>
	endmacro

	macro wrong_layer_child_3(child_arg)
		db <child_arg>+<^prent_arg> ; <- intentional typo
	endmacro
endmacro

%wrong_layer_parent($01)
%wrong_layer_child($02)
%wrong_layer_child_2($03)
%wrong_layer_child_3($04)



macro first_unclosed()
	macro second_unclosed()