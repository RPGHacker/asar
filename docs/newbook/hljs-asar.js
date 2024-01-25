hljsAsar = {
    init: function() {
        // I know this is ugly, but I don't know how else to solve Asar's label rules...
        const asarOpcodes = ["db", "dw", "dl", "dd", "adc", "and", "asl", "bcc", "blt", "bcs", "bge", "beq", "bit", "bmi", "bne", "bpl", "bra", "brk", "brl", "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cop", "cpx", "cpy", "dec", "dea", "dex", "dey", "eor", "inc", "ina", "inx", "iny", "jmp", "jml", "jsr", "jsl", "lda", "ldx", "ldy", "lsr", "mvn", "mvp", "nop", "ora", "pea", "pei", "per", "pha", "phb", "phd", "phk", "php", "phx", "phy", "pla", "plb", "pld", "plp", "plx", "ply", "rep", "rol", "ror", "rti", "rtl", "rts", "sbc", "sec", "sed", "sei", "sep", "sta", "stp", "stx", "sty", "stz", "tax", "tay", "tcd", "tcs", "tdc", "trb", "tsc", "tsb", "tsx", "txa", "txs", "txy", "tya", "tyx", "wai", "wdm", "xba", "xce", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "add", "alt1", "alt2", "alt3", "asr", "bic", "cache", "cmode", "color", "div2", "fmult", "from", "getb", "getbh", "getbl", "getbs", "getc", "hib", "ibt", "iwt", "ldb", "ldw", "link", "ljmp", "lm", "lms", "lmult", "lob", "loop", "merge", "mult", "not", "or", "plot", "ramb", "romb", "rpix", "sbk", "sex", "sm", "sms", "stb", "stop", "stw", "sub", "swap", "to", "umult", "with", "xor", "addw", "ya", "and1", "bbc0", "bbc1", "bbc2", "bbc3", "bbc4", "bbc5", "bbc6", "bbc7", "bbs0", "bbs1", "bbs2", "bbs3", "bbs4", "bbs5", "bbs6", "bbs7", "call", "cbne", "clr0", "clr1", "clr2", "clr3", "clr4", "clr5", "clr6", "clr7", "clrc", "clrp", "clrv", "cmpw", "daa", "das", "dbnz", "decw", "di", "div", "ei", "eor1", "incw", "mov", "sp", "mov1", "movw", "mul", "not1", "notc", "or1", "pcall", "pop", "push", "ret", "reti", "set0", "set1", "set2", "set3", "set4", "set5", "set6", "set7", "setc", "setp", "sleep", "subw", "tcall", "tclr", "tset", "xcn", "lea", "move", "moves", "moveb", "movew"];

        const asarKeywords = ["lorom", "hirom", "exlorom", "exhirom", "sa1rom", "fullsa1rom", "sfxrom", "norom", /* "macro", */ "endmacro", "struct", "endstruct", "endstruct", "extends", "incbin", "incsrc", "fillbyte", "fillword", "filllong", "filldword", "fill", "padbyte", "pad", "padword", "padlong", "paddword", "cleartable", "skip", "namespace", "import", "print", "org", "warnpc", "base", "on", "off", "reset", "freespaceuse", "pc", "bytes", "hex", "freespace", "freecode", "freedata", "ram", "noram", "align", "cleaned", "static", "autoclean", "prot", "pushpc", "pullpc", "pushbase", "pullbase", "function", "if", "else", "elseif", "endif", "while", "endwhile", "for", "endfor", "assert", "arch", "65816", "spc700", "inline", "superfx", "bankcross", "full", "half", "bank", "noassume", "auto", "asar", "includefrom", "includeonce", "include", "error", "double", "round", "pushtable", "pulltable", "undef", "check", "title", "nested", "warnings", "push", "pull", "disable", "enable", "warn", "address", "dpbase", "optimize", "dp", "none", "always", "default", "mirrors", "global", "spcblock", "endspcblock", "nspc", "execute", "offset", "pushns", "pullns", "segment", "start", "freespacebyte"];

        const asarIntrinsicFunctions = ["read1", "read2", "read3", "read4", "canread1", "canread2", "canread4", "sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "arcsin", "arccos", "arctan", "log", "log10", "log2", "readfile1", "readfile2", "readfile3", "readfile4", "canreadfile1", "canreadfile2", "canreadfile3", "canreadfile4", "canreadfile", "filesize", "getfilestatus", "snestopc", "pctosnes", "max", "min", "clamp", "safediv", "select", "not", "equal", "notequal", "less", "lessequal", "greater", "greaterequal", "and", "or", "nand", "nor", "xor", "defined", "sizeof", "objectsize", "stringsequal", "stringsequalnocase"];

        const asarNumberLiteralsMode = {
            scope: "number",
            variants:
            [
                { begin: /(?<=\W|^)[0-9]+(\.[0-9]+)?(?=\W|$)/ },
                { begin: /(?<=\W|^)%[0-1]+(?=\W|$)/ },
                { begin: /(?<=\W|^)\$[0-9a-fA-F]+(?=\W|$)/ },
            ],
        };
        const asarOperatorsMode = {
            scope: "operator",
            begin: /\(|\)|\+|\-|\*|\/|\%|\<\<|\>\>|\&|\||\^|\~|\*\*/,
        };

        const asarFunctionCallMode = {
            scope: "title.function",
            begin: /[a-zA-Z0-9_]+(?=\()/,
        }

        // trying to highlight labels separately is more trouble than it's worth,
        // the syntax conflicts with like everything else lol
        const asarLabelReferenceMode = {
            scope: "label",
            variants: [
                { begin: /\??\.*[a-zA-Z0-9_]+/ },
                { begin: /\??(-+|\++)/ },
            ],
        };

        /*const asarLabelDefinitionMode = {
            scope: "label",
            variants: [
                // sublabels, macro labels, # labels - optional :
                { begin: /#?\??\.+[a-zA-Z0-9_]+:?/ },
                { begin: /#\??\.*[a-zA-Z0-9_]+:?/ },
                { begin: /#?\?\.*[a-zA-Z0-9_]+:?/ },
                // main labels - require :
                { begin: /[a-zA-Z0-9_]+:/ },
                // +- labels
                { begin: /\??(-+|\++):?/ },
            ]
        };*/

        const asarSimpleDefineInMacroArg = {
            scope: "variable.define",
            begin: /!\^*[a-zA-Z0-9_]+/,
        }

        const asarSimpleMacroArg = {
            scope: "variable.macro",
            begin: /<\^*[a-zA-Z0-9_]+>/,
        }

        let asarBracedDefineInMacroArg = {
            scope: "variable.define",
            begin: /!\^*{/,
            end: /}/,
            contains: ['self', asarSimpleDefineInMacroArg]
        };

        let asarExpression = [
            asarNumberLiteralsMode,
            asarFunctionCallMode,
            asarOperatorsMode,
            asarLabelReferenceMode,
        ];

        const asarVariadicMacroArg = {
            scope: "variable.macro.variadic",
            begin: /\<\^*\.\.\.\[/,
            end: /\]\>/,
            contains: [
                asarBracedDefineInMacroArg,
                asarSimpleDefineInMacroArg,
                ...asarExpression
            ],
        };

        let asarDefineOutsideMacro = {
            scope: "variable.define",
            begin: /!\^*(?=[a-zA-Z0-9_<])/,
            end: /(?=[^a-zA-Z0-9_<])/,
            contains: [asarVariadicMacroArg, asarSimpleMacroArg],
        }

        let asarBracedDefineOutsideMacro = {
            scope: "variable.define",
            begin: /!\^*{/,
            end: /}/,
            contains: ['self', asarDefineOutsideMacro, asarVariadicMacroArg, asarSimpleMacroArg]
        };

        hljs.registerLanguage("asar",
            function(hljs) {
                return {
                    case_insensitive: true,
                    contains:
                    [
                        hljs.COMMENT(/;\[\[/, /\]\]/),
                        hljs.COMMENT("[;]", "$"),
                        {
                            scope: "string",
                            variants: [
                                // terminate these at newlines to prevent missing an ending quote highlighting the rest of the file wrong.
                                { begin: "'", end: /'|$/ },
                                { begin: '"', end: /"|$/ },
                            ],
                            contains: [
                                { begin: /\\(!|\\)/ },
                                asarVariadicMacroArg, asarSimpleMacroArg,
                                asarDefineOutsideMacro,
                                asarBracedDefineOutsideMacro,
                            ]
                        },
                        //hljs.QUOTE_STRING_MODE,
                        //hljs.APOS_STRING_MODE,
                        {
                            scope: "keyword",
                            begin: asarKeywords.join('\\b|').concat('\\b')
                        },
                        // hack for macro definitions
                        {
                            begin: /\b(?=macro )/,
                            end: /(?=\()|$/,
                            contains: [
                                { scope: "keyword", begin: "macro "},
                                { scope: "title.macro", begin: /[a-zA-Z_][a-zA-Z0-9_]*/ },
                            ]
                        },
                        {
                            scope: "title.macro",
                            // RPG Hacker: The exclamation mark here is for the case when defines are used as functions.
                            // Probably not a very common case, but my VWF Dialogues Patch uses it a lot, and this makes
                            // stuff a lot more readable in those cases.
                            begin: /(%|!)[a-zA-Z0-9_]+(?=\()/,
                        },
                        {
                            scope: "built_in",
                            begin: asarOpcodes.join('(\\.[bwl]|\\b)|').concat('(\\.[bwl]|\\b)')
                        },
                        //asarLabelDefinitionMode,
                        asarBracedDefineOutsideMacro,
                        asarDefineOutsideMacro,
                        asarSimpleMacroArg,
                        asarVariadicMacroArg,
                        ...asarExpression,
                    ],
                    i: "/"
                }
            }
        );
    }
};

hljsAsar.init();
