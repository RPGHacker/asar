;`ea 5c 08 80 00 ea ea ea ea ea ea 22 00 80 00 22
;`01 80 00 22 05 80 00 22 06 80 00 22 07 80 00 22
;`08 80 00 22 09 80 00 22 0a 80 00 22 0b 80 00   

lorom
org $008000

namespace nested on
global label1: : nop
namespace main
    label1:
    jml label2
    namespace second
    label2: : nop
        namespace third
        label3: : nop
        .sublabel : nop
        global label2: : nop
        .sublabel : nop
        global #label3: : nop
        ..sublabel
        namespace off
    namespace off
namespace off
namespace nested off
jsl label1
jsl main_label1
jsl main_second_label2
jsl main_second_third_label3
jsl main_second_third_label3_sublabel
jsl label2
jsl main_second_third_label2_sublabel
jsl label3
jsl main_second_third_label2_sublabel_sublabel