;`errElabel_forward
struct a pos
;`errEendstruct_without_struct
endstruct
pos = $1234

struct b $123
skip 5
;`errEinvalid_label_name
lbl:
endstruct
