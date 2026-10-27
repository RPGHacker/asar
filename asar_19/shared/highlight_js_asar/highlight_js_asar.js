hljsAsar = {
  init : function()
  {
    hljs.registerLanguage("powershell", function(hljs)
	{
      return {
          aliases: ["ps"],
          case_insensitive: true,
          keywords:
		  {
            $pattern: /-?[A-z\.\-]+/
          }
      }
    });

    // I know this is ugly, but I don't know how else to solve Asar's label rules...
    const asarOpcodes = ["db", "dw", "dl", "dd", "adc", "and", "asl", "bcc", "blt", "bcs", "bge", "beq", "bit", "bmi", "bne", "bpl", "bra", "brk", "brl", "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cop", "cpx", "cpy", "dec", "dea", "dex", "dey", "eor", "inc", "ina", "inx", "iny", "jmp", "jml", "jsr", "jsl", "lda", "ldx", "ldy", "lsr", "mvn", "mvp", "nop", "ora", "pea", "pei", "per", "pha", "phb", "phd", "phk", "php", "phx", "phy", "pla", "plb", "pld", "plp", "plx", "ply", "rep", "rol", "ror", "rti", "rtl", "rts", "sbc", "sec", "sed", "sei", "sep", "sta", "stp", "stx", "sty", "stz", "tax", "tay", "tcd", "tcs", "tdc", "trb", "tsc", "tsb", "tsx", "txa", "txs", "txy", "tya", "tyx", "wai", "wdm", "xba", "xce", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "add", "alt1", "alt2", "alt3", "asr", "bic", "cache", "cmode", "color", "div2", "fmult", "from", "getb", "getbh", "getbl", "getbs", "getc", "hib", "ibt", "iwt", "ldb", "ldw", "link", "ljmp", "lm", "lms", "lmult", "lob", "loop", "merge", "mult", "not", "or", "plot", "ramb", "romb", "rpix", "sbk", "sex", "sm", "sms", "stb", "stop", "stw", "sub", "swap", "to", "umult", "with", "xor", "addw", "ya", "and1", "bbc0", "bbc1", "bbc2", "bbc3", "bbc4", "bbc5", "bbc6", "bbc7", "bbs0", "bbs1", "bbs2", "bbs3", "bbs4", "bbs5", "bbs6", "bbs7", "call", "cbne", "clr0", "clr1", "clr2", "clr3", "clr4", "clr5", "clr6", "clr7", "clrc", "clrp", "clrv", "cmpw", "daa", "das", "dbnz", "decw", "di", "div", "ei", "eor1", "incw", "mov", "sp", "mov1", "movw", "mul", "not1", "notc", "or1", "pcall", "pop", "push", "ret", "reti", "set0", "set1", "set2", "set3", "set4", "set5", "set6", "set7", "setc", "setp", "sleep", "subw", "tcall", "tclr", "tset", "xcn", "lea", "move", "moves", "moveb", "movew"];
    
    const asarKeywords = ["lorom", "hirom", "exlorom", "exhirom", "sa1rom", "fullsa1rom", "sfxrom", "norom", "macro", "endmacro", "struct", "endstruct", "extends", "incbin", "incsrc", "fillbyte", "fillword", "filllong", "filldword", "fill", "padbyte", "pad", "padword", "padlong", "paddword", "table", "cleartable", "ltr", ",rtl", "skip", "namespace", "import", "print", "org", "warnpc", "base", "on", "off", "reset", "freespaceuse", "pc", "bytes", "hex", "freespace", "freecode", "freedata", "ram", "noram", "align", "cleaned", "static", "autoclean", "autoclear", "prot", "pushpc", "pullpc", "pushbase", "pullbase", "function", "if", "else", "elseif", "endif", "while", "endwhile", "for", "endfor", "assert", "arch", "65816", "spc700", "inline", "superfx", "math", "pri", "round", "xkas", "bankcross", "bank", "noassume", "auto", "asar", "includefrom", "includeonce", "include", "error", "skip", "double", "round", "pushtable", "pulltable", "undef", "check", "title", "nested", "warnings", "push", "pull", "disable", "enable", "warn", "address", "dpbase", "optimize", "dp", "none", "always", "default", "mirrors", "global", "spcblock", "endspcblock", "nspc", "execute", "\\.\\."];
    
    const asarIntrinsicFunctions = ["read1", "read2", "read3", "read4", "canread1", "canread2", "canread4", "sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "arcsin", "arccos", "arctan", "log", "log10", "log2", "_read1", "_read2", "_read3", "_read4", "_canread1", "_canread2", "_canread4", "_sqrt", "_sin", "_cos", "_tan", "_asin", "_acos", "_atan", "_arcsin", "_arccos", "_arctan", "_log", "_log10", "_log2", "readfile1", "_readfile1", "readfile2", "_readfile2", "readfile3", "_readfile3", "readfile4", "_readfile4", "canreadfile1", "_canreadfile1", "canreadfile2", "_canreadfile2", "canreadfile3", "_canreadfile3", "canreadfile4", "_canreadfile4", "canreadfile", "_canreadfile", "filesize", "_filesize", "getfilestatus", "_getfilestatus", "snestopc", "_snestopc", "pctosnes", "_pctosnes", "max", "_max", "min", "_min", "clamp", "_clamp", "safediv", "_safediv", "select", "_select", "not", "_not", "equal", "_equal", "notequal", "_notequal", "less", "_less", "lessequal", "_lessequal", "greater", "_greater", "greaterequal", "_greaterequal", "and", "_and", "or", "_or", "nand", "_nand", "nor", "_nor", "xor", "_xor", "defined", "_defined", "sizeof", "_sizeof", "objectsize", "_objectsize", "stringsequal", "_stringsequal", "stringsequalnocase", "_stringsequalnocase"];
    
	const asarNumberLiteralsMode = 
    {
    	scope: "number",
    	variants:
    	[
    		{
    			begin: /(?<=\W|^)#?[+\-~]?0x[0-9a-fA-F]+(?=\W|$)/
    		},
    		{
    			begin: /(?<=\W|^)#?[+\-~]?[0-9]+(\.[0-9]+)?(?=\W|$)/
    		},
    		{
    			begin: /(?<=\W|^)#?[+\-~]?%[0-1]+(?=\W|$)/
    		},
    		{
    			begin: /(?<=\W|^)#?[+\-~]?\$[0-9a-fA-F]+(?=\W|$)/
    		}/*,
    		{
    			begin: /(?<=\W|^)#?(\(|\)|\+|\-|\*|\/|\%|\<\<|\>\>|\&|\||\^|\~|\*\*)+/
    		}*/
    	],
    	relevance: 0
    };
	
	const asarLabelsMode =
    {
    	scope: "label",
    	variants:
    	[
    		{
    			begin: /#?\??\.*[a-zA-Z0-9_]+:?/
    		},				
    		{
    			begin: /\??(-+|\++):?/
    		}
    	],
    	relevance: 0
    }
	
    hljs.registerLanguage("65c816_asar",
    	function(hljs)
    	{
    		return {
    			case_insensitive: true,
    			aliases: ["asar"],
    			keywords:
    			{
    				opcodes: asarOpcodes.join(' '),
    				keywords: asarKeywords.join(' '),
    				builtin: asarIntrinsicFunctions.join(' '),
					$pattern: hljs.IDENT_RE
    			},
    			contains:
    			[
    				hljs.COMMENT("[;]", "$"),
    				hljs.C_BLOCK_COMMENT_MODE,
    				hljs.QUOTE_STRING_MODE,
    				hljs.APOS_STRING_MODE,
    				{
    					scope: "special",
    					begin: /\s*^@/,
    					end: /$/,
    					relevance: 0
    				},
    				{
    					scope: "keywords",
    					begin: asarKeywords.join('\\b|').concat('\\b')
    				},
    				asarNumberLiteralsMode,
    				{
    					scope: "builtin",
    					begin: "(" + asarIntrinsicFunctions.join('|') + ")\\(",
    					end: "\\)",
						contains:
						[
							hljs.QUOTE_STRING_MODE,
							hljs.APOS_STRING_MODE,
							asarNumberLiteralsMode,
							'self',
							asarLabelsMode
						],
    				},
    				{
    					scope: "function",
    					variants:
    					[
    						{
								// RPG Hacker: The exclamation mark here is for the case when defines are used as functions.
								// Probably not a very common case, but my VWF Dialogues Patch uses it a lot, and this makes
								// stuff a lot more readable in those cases.
    							begin: /(?:%|!)?[a-zA-Z0-9_]+\(/,
    							end: /\)/
    						}
    					],
						contains:
						[
							hljs.QUOTE_STRING_MODE,
							hljs.APOS_STRING_MODE,
							asarNumberLiteralsMode,
							'self',
							asarLabelsMode
						],
    					relevance: 0
    				},
    				{
    					scope: "define",
    					variants:
    					[
    						{
    							begin: /!\^*[a-zA-Z0-9_{}]+/
    						},
    						{
    							begin: /<\^*[a-zA-Z0-9_]+>/
    						},
    					],
    					relevance: 10
    				},
    				{
    					scope: "opcodes",
    					begin: asarOpcodes.join('(\\.[bwl]|\\b)|').concat('(\\.[bwl]|\\b)')
    				},
					asarLabelsMode
    			],
    			i: "/"
    		}
    	}
    );
  }
};