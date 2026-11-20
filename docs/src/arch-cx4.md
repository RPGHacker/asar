# Supported Cx4 instructions

As there is no official documentation on the Cx4 chip (the Hitachi HG51B169), liberty has been taken to design the Cx4 assembly syntax.

Some of the warnings Asar gives, are based on how bsnes emulates the chip. These are subject to changes in case Cx4 emulation on bsnes is improved even more.

## Registers

| Register     | Code      | Bits | Name                    | Description |
|--------------|-----------|------|-------------------------|-------------|
| `A`          | `$00`     | 24   | Accumulator             | The result of most instructions. |
| `MACH`       | `$01`     | 24   | Multiply Result High    | High 24 bits of the multiply result. |
| `MACL`       | `$02`     | 24   | Multiply Result Low     | Low 24 bits of the multiply result. |
| `MDR`        | `$03`     | 24   | Memory Data Register    | The data for bus reads and writes. |
| `ROMB`       | `$08`     | 24   | ROM Buffer              | The Cx4 data ROM entry read by `RDROM`. |
| `RAMB`       | `$0C`     | 24   | RAM Buffer              | The Cx4 RAM bytes accessed by `RDRAM` and `WRRAM`. |
| `MAR`        | `$13`     | 24   | Memory Address Register | The address for bus reads and writes. |
| `DPR`        | `$1C`     | 24   | Data Pointer Register   | The Cx4 RAM address used by `RDRAM` and `WRRAM` (recommended values are $000-$BFF). |
| `IP`         | `$20`     | 8    | Instruction Pointer     | The word index of the next instruction in the current page. |
| `P`          | `$28`     | 15   | Page Register           | The page used by far branches and when execution runs off the end of a page. |
| `BUSROM`     | `$2E`     | -    | ROM Bus Access          | Used to access the cartridge's ROM data through the bus. |
| `BUSRAM`     | `$2F`     | -    | RAM Bus Access          | Used to access the cartridge's SRAM data through the bus. |
| `IR0`-`IR15` | `$50-$5F` | 24   | Immediate Registers     | Read-only constants, listed below. |
| `R0`-`R15`   | `$60-$6F` | 24   | General-Purpose Registers | General-purpose registers. |

Each `IR` constant can also be written as `#` followed by its 6-digit value:

| Register | Value      | Register | Value      |
|----------|------------|----------|------------|
| `IR0`    | `#$000000` | `IR8`    | `#$008000` |
| `IR1`    | `#$FFFFFF` | `IR9`    | `#$007FFF` |
| `IR2`    | `#$00FF00` | `IR10`   | `#$FF7FFF` |
| `IR3`    | `#$FF0000` | `IR11`   | `#$FFFF7F` |
| `IR4`    | `#$00FFFF` | `IR12`   | `#$010000` |
| `IR5`    | `#$FFFF00` | `IR13`   | `#$FEFFFF` |
| `IR6`    | `#$800000` | `IR14`   | `#$000100` |
| `IR7`    | `#$7FFFFF` | `IR15`   | `#$00FEFF` |

## Syntax notes

- **Shifted A.** `CMP`, `CMPR`, `ADD`, `SUB`, `SUBR`, `XNOR`, `XOR`, `AND` and `OR` can shift A left before the operation: `A`, `A << 1`, `A << 8` or `A << 16`. Writing `A << 0` is the same as just `A`. The shift count may be a math expression, but it must evaluate to one of these values.
- **Reversed operations.** `CMPR` and `SUBR` compute the `operand-A` of `A-operand`, and are written with A on the right: `SUBR R0, A << 1`.
- **Shifts.** `SHR`, `ASR`, `ROR` and `SHL` take a shift count of `#0` to `#31`.
- **Branches and jumps.** `BRA`/`BEQ`/`BCS`/`BMI`/`BVS` jump to the label, `JSR`/`JEQ`/`JCS`/`JMI`/`JVS` call the label (use `RTS` to return). Prefix the target with a `P:` for a far branch or jump, which goes to the page held in the `P` register at the moment (e.g. `BRA P:mylabel`).
- **Cx4 RAM.** `RDRAM` and `WRRAM` always needs to specify `RAMB+0`, `RAMB+1` or `RAMB+2`. The address is either `A`, or `DPR+` with an offset; If you don't want to specify an offset for DPR, you must use `DPR+0`. The `LD` instruction must address `RAMB` as such; it loads all 24 bits of RAMB at once.
- **Cx4 Data ROM.** `RDROM` either uses `A` or a fixed 10-bit address (`$000`-`$3FF`).
- **Page register.** `PH` only takes `#$00`-`#$7F`.
- **Stores.** `ST reg, A` and `ST reg, MDR` write A or MDR to any register. Stores to `A` and `IR0`-`IR15` do nothing, and a store into `BUSROM`/`BUSRAM` is highly unconventional and not recommended.
- **Page ends.** When execution reaches the last word of a 512-byte page, the Cx4 switches to page `P` (or stops) instead of running into the next page. Asar warns the user about this.

Operands are separated by a comma, and register names, mnemonics and `RAMB`/`DPR`/`P:` are case-insensitive. Spaces around `+` and `<<` are allowed.

## Instruction list

Wherever the list below uses `R0` as the source register, any register can be used, except that `LD P` and `SWAP` __only__ take `R0`-`R15`.

```asar
NOP
BRA mylabel
BRA P:mylabel
BEQ mylabel
BEQ P:mylabel
BCS mylabel
BCS P:mylabel
BMI mylabel
BMI P:mylabel
BVS mylabel
BVS P:mylabel
WAIT
SKIPVC
SKIPVS
SKIPCC
SKIPCS
SKIPZC
SKIPZS
SKIPNC
SKIPNS
JSR mylabel
JSR P:mylabel
JEQ mylabel
JEQ P:mylabel
JCS mylabel
JCS P:mylabel
JMI mylabel
JMI P:mylabel
JVS mylabel
JVS P:mylabel
RTS
INC MAR
CMPR R0, A
CMPR R0, A << 1
CMPR R0, A << 8
CMPR R0, A << 16
CMPR #$12, A
CMPR #$12, A << 1
CMPR #$12, A << 8
CMPR #$12, A << 16
CMP A, R0
CMP A << 1, R0
CMP A << 8, R0
CMP A << 16, R0
CMP A, #$12
CMP A << 1, #$12
CMP A << 8, #$12
CMP A << 16, #$12
SXB
SXW
LD A, R0
LD MDR, R0
LD MAR, R0
LD P, R0
LD MDR, BUSROM
LD MDR, BUSRAM
LD A, #$12
LD MDR, #$12
LD MAR, #$12
LD P, #$12
RDRAM RAMB+0, A
RDRAM RAMB+1, A
RDRAM RAMB+2, A
RDRAM RAMB+0, DPR+$12
RDRAM RAMB+1, DPR+$12
RDRAM RAMB+2, DPR+$12
RDROM A
RDROM $123
LD PL, #$12
LD PH, #$12
ADD A, R0
ADD A << 1, R0
ADD A << 8, R0
ADD A << 16, R0
ADD A, #$12
ADD A << 1, #$12
ADD A << 8, #$12
ADD A << 16, #$12
SUBR R0, A
SUBR R0, A << 1
SUBR R0, A << 8
SUBR R0, A << 16
SUBR #$12, A
SUBR #$12, A << 1
SUBR #$12, A << 8
SUBR #$12, A << 16
SUB A, R0
SUB A << 1, R0
SUB A << 8, R0
SUB A << 16, R0
SUB A, #$12
SUB A << 1, #$12
SUB A << 8, #$12
SUB A << 16, #$12
SMUL A, R0
SMUL A, #$12
XNOR A, R0
XNOR A << 1, R0
XNOR A << 8, R0
XNOR A << 16, R0
XNOR A, #$12
XNOR A << 1, #$12
XNOR A << 8, #$12
XNOR A << 16, #$12
XOR A, R0
XOR A << 1, R0
XOR A << 8, R0
XOR A << 16, R0
XOR A, #$12
XOR A << 1, #$12
XOR A << 8, #$12
XOR A << 16, #$12
AND A, R0
AND A << 1, R0
AND A << 8, R0
AND A << 16, R0
AND A, #$12
AND A << 1, #$12
AND A << 8, #$12
AND A << 16, #$12
OR A, R0
OR A << 1, R0
OR A << 8, R0
OR A << 16, R0
OR A, #$12
OR A << 1, #$12
OR A << 8, #$12
OR A << 16, #$12
SHR A, R0
SHR A, #$12
ASR A, R0
ASR A, #$12
ROR A, R0
ROR A, #$12
SHL A, R0
SHL A, #$12
ST R0, A
ST R0, MDR
ST BUSROM, MDR
ST BUSRAM, MDR
WRRAM A, RAMB+0
WRRAM A, RAMB+1
WRRAM A, RAMB+2
WRRAM DPR+$12, RAMB+0
WRRAM DPR+$12, RAMB+1
WRRAM DPR+$12, RAMB+2
SWAP A, R0
CLEAR
STOP
```
