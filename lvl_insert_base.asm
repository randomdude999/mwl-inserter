;!lvlnum = %LVL_NUM%
if read1($00FFD5) == $23
    sa1rom
else
    lorom
endif

org $05E000+(!lvlnum*3)
; layer 1 data
autoclean dl layer1

org $05E600+(!lvlnum*3)
; layer 2 data
autoclean dl layer2

org $05EC00+(!lvlnum*2)
; sprite word
autoclean sprite
dw sprite

org $0EF100+!lvlnum
; sprite bank
db sprite>>16

org $05F000+!lvlnum
incbin "secondary.bin":0-1
org $05F200+!lvlnum
incbin "secondary.bin":1-2
org $05F400+!lvlnum
incbin "secondary.bin":2-3
org $05F600+!lvlnum
incbin "secondary.bin":3-4
org $05DE00+!lvlnum
incbin "secondary.bin":4-5
org $0EF310+!lvlnum
incbin "secondary.bin":5-6

freedata
layer1:
incbin "layer1.bin"
layer2:
incbin "layer2.bin"
sprite:
incbin "sprite.bin"

