;`A9 00 00 A9 03 00 A9 02 00
org $008000
	main:
		lda #<:main
		lda #<:test_label
		lda #bank(other_test)

base $038000
	test_label:

base $028000
	other_test:
