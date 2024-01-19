;`skip
;`56 34 12
;This define is provided by a memory file via lib interface.
org $008000
if defined("cli_only")
	dl $123456
else
	incsrc "file.memory"
	dl !fancy_memory_file_define
endif
