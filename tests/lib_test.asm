;`56 34 12
org $008000
if defined("cli_only")
	dl $123456
else
	incsrc "file.memory"
	!lib_test_file_define = $123456
	lib_test_file_label:
	dl !lib_test_file_define
endif
