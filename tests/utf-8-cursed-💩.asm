;`EF BE AD DE
;`10 11 12 12 13 14 40 30 20
;`AB CD EF
;P>日本語🇯🇵
;P>日本語🇯🇵

org $008000
incsrc "data/日本語😳.tbl"

'💩' = $DEADBEEF

dd "💩"

db "Waddup? 🤣"

db "日本語"

print "日本語🇯🇵"

print "!cmdl_define_utf8"
