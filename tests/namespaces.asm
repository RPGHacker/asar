;`00 00
;`00 00
;`00 00
;`00 00
;`06 80 00
;`04 80 00
;`06 80 00
;`02 80 00
;`04 80 00
;`06 80 00
;`00 80 00
;`02 80 00
;`04 80 00
;`06 80 00
;`00 00
;`26 80 00



org $008000


namespace nested on


Main:
	db $00,$00

namespace First
{
	Main:
		db $00,$00
		
	namespace Second
	{
		Main:
			db $00,$00
			
			namespace Third
			{
				Main:
					db $00,$00
				
					dl Main
			
			}			
			namespace off
			
		dl Main
		dl Third_Main
	}	
	namespace off
		
	dl Main
	dl Second_Main
	dl Second_Third_Main
}
namespace off
		
dl Main
dl First_Main
dl First_Second_Main
dl First_Second_Third_Main


namespace nested off


namespace First
{
	namespace Second
	{
		namespace Third
		{		
			Main2:
				db $00,$00
	
		}
	}
}
namespace off


dl Third_Main2
