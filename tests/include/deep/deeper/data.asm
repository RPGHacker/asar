
macro include_in_macro()
	db $04
	incbin "deepest/data.bin"
	db $06
endmacro

%include_in_macro()