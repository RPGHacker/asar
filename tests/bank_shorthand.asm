;`A9 00 00 A9 03 00 A9 02 00
org $008000
	main:
		lda.w #<:main
		lda.w #<:test_label
		lda.w #bank(other_test)

base $038000
	test_label:

base $028000
	other_test:
