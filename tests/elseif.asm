;`01

!count = 0

!y = "!count #= !count+1"
!n = "db 0"

org $008000

if 1
	if 1
		!y
	elseif 1
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 1
		!y
	else
		!n
	endif


	if 1
		!y
	elseif 0
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 0
		!n
	else
		!y
	endif
elseif 1
	!n
else
	!n
endif



if 0
	!n
elseif 1
	if 1
		!y
	elseif 1
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 1
		!y
	else
		!n
	endif


	if 1
		!y
	elseif 0
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 0
		!n
	else
		!y
	endif
else
	!n
endif



if 1
	if 1
		!y
	elseif 1
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 1
		!y
	else
		!n
	endif


	if 1
		!y
	elseif 0
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 0
		!n
	else
		!y
	endif
elseif 0
	!n
else
	!n
endif



if 0
	!n
elseif 0
	!n
else
	if 1
		!y
	elseif 1
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 1
		!y
	else
		!n
	endif


	if 1
		!y
	elseif 0
		!n
	else
		!n
	endif


	if 0
		!n
	elseif 0
		!n
	else
		!y
	endif
endif

if !count == 4**2 : db 1 : endif
