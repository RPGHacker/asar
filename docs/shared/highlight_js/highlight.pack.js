/*! highlight.js v9.12.0 | BSD3 License | git.io/hljslicense */
!function(e){var n="object"==typeof window&&window||"object"==typeof self&&self;"undefined"!=typeof exports?e(exports):n&&(n.hljs=e({}),"function"==typeof define&&define.amd&&define([],function(){return n.hljs}))}(function(e){function n(e){return e.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")}function t(e){return e.nodeName.toLowerCase()}function r(e,n){var t=e&&e.exec(n);return t&&0===t.index}function a(e){return k.test(e)}function i(e){var n,t,r,i,o=e.className+" ";if(o+=e.parentNode?e.parentNode.className:"",t=B.exec(o))return w(t[1])?t[1]:"no-highlight";for(o=o.split(/\s+/),n=0,r=o.length;r>n;n++)if(i=o[n],a(i)||w(i))return i}function o(e){var n,t={},r=Array.prototype.slice.call(arguments,1);for(n in e)t[n]=e[n];return r.forEach(function(e){for(n in e)t[n]=e[n]}),t}function u(e){var n=[];return function r(e,a){for(var i=e.firstChild;i;i=i.nextSibling)3===i.nodeType?a+=i.nodeValue.length:1===i.nodeType&&(n.push({event:"start",offset:a,node:i}),a=r(i,a),t(i).match(/br|hr|img|input/)||n.push({event:"stop",offset:a,node:i}));return a}(e,0),n}function c(e,r,a){function i(){return e.length&&r.length?e[0].offset!==r[0].offset?e[0].offset<r[0].offset?e:r:"start"===r[0].event?e:r:e.length?e:r}function o(e){function r(e){return" "+e.nodeName+'="'+n(e.value).replace('"',"&quot;")+'"'}s+="<"+t(e)+E.map.call(e.attributes,r).join("")+">"}function u(e){s+="</"+t(e)+">"}function c(e){("start"===e.event?o:u)(e.node)}for(var l=0,s="",f=[];e.length||r.length;){var g=i();if(s+=n(a.substring(l,g[0].offset)),l=g[0].offset,g===e){f.reverse().forEach(u);do c(g.splice(0,1)[0]),g=i();while(g===e&&g.length&&g[0].offset===l);f.reverse().forEach(o)}else"start"===g[0].event?f.push(g[0].node):f.pop(),c(g.splice(0,1)[0])}return s+n(a.substr(l))}function l(e){return e.v&&!e.cached_variants&&(e.cached_variants=e.v.map(function(n){return o(e,{v:null},n)})),e.cached_variants||e.eW&&[o(e)]||[e]}function s(e){function n(e){return e&&e.source||e}function t(t,r){return new RegExp(n(t),"m"+(e.cI?"i":"")+(r?"g":""))}function r(a,i){if(!a.compiled){if(a.compiled=!0,a.k=a.k||a.bK,a.k){var o={},u=function(n,t){e.cI&&(t=t.toLowerCase()),t.split(" ").forEach(function(e){var t=e.split("|");o[t[0]]=[n,t[1]?Number(t[1]):1]})};"string"==typeof a.k?u("keyword",a.k):x(a.k).forEach(function(e){u(e,a.k[e])}),a.k=o}a.lR=t(a.l||/\w+/,!0),i&&(a.bK&&(a.b="\\b("+a.bK.split(" ").join("|")+")\\b"),a.b||(a.b=/\B|\b/),a.bR=t(a.b),a.e||a.eW||(a.e=/\B|\b/),a.e&&(a.eR=t(a.e)),a.tE=n(a.e)||"",a.eW&&i.tE&&(a.tE+=(a.e?"|":"")+i.tE)),a.i&&(a.iR=t(a.i)),null==a.r&&(a.r=1),a.c||(a.c=[]),a.c=Array.prototype.concat.apply([],a.c.map(function(e){return l("self"===e?a:e)})),a.c.forEach(function(e){r(e,a)}),a.starts&&r(a.starts,i);var c=a.c.map(function(e){return e.bK?"\\.?("+e.b+")\\.?":e.b}).concat([a.tE,a.i]).map(n).filter(Boolean);a.t=c.length?t(c.join("|"),!0):{exec:function(){return null}}}}r(e)}function f(e,t,a,i){function o(e,n){var t,a;for(t=0,a=n.c.length;a>t;t++)if(r(n.c[t].bR,e))return n.c[t]}function u(e,n){if(r(e.eR,n)){for(;e.endsParent&&e.parent;)e=e.parent;return e}return e.eW?u(e.parent,n):void 0}function c(e,n){return!a&&r(n.iR,e)}function l(e,n){var t=N.cI?n[0].toLowerCase():n[0];return e.k.hasOwnProperty(t)&&e.k[t]}function p(e,n,t,r){var a=r?"":I.classPrefix,i='<span class="'+a,o=t?"":C;return i+=e+'">',i+n+o}function h(){var e,t,r,a;if(!E.k)return n(k);for(a="",t=0,E.lR.lastIndex=0,r=E.lR.exec(k);r;)a+=n(k.substring(t,r.index)),e=l(E,r),e?(B+=e[1],a+=p(e[0],n(r[0]))):a+=n(r[0]),t=E.lR.lastIndex,r=E.lR.exec(k);return a+n(k.substr(t))}function d(){var e="string"==typeof E.sL;if(e&&!y[E.sL])return n(k);var t=e?f(E.sL,k,!0,x[E.sL]):g(k,E.sL.length?E.sL:void 0);return E.r>0&&(B+=t.r),e&&(x[E.sL]=t.top),p(t.language,t.value,!1,!0)}function b(){L+=null!=E.sL?d():h(),k=""}function v(e){L+=e.cN?p(e.cN,"",!0):"",E=Object.create(e,{parent:{value:E}})}function m(e,n){if(k+=e,null==n)return b(),0;var t=o(n,E);if(t)return t.skip?k+=n:(t.eB&&(k+=n),b(),t.rB||t.eB||(k=n)),v(t,n),t.rB?0:n.length;var r=u(E,n);if(r){var a=E;a.skip?k+=n:(a.rE||a.eE||(k+=n),b(),a.eE&&(k=n));do E.cN&&(L+=C),E.skip||(B+=E.r),E=E.parent;while(E!==r.parent);return r.starts&&v(r.starts,""),a.rE?0:n.length}if(c(n,E))throw new Error('Illegal lexeme "'+n+'" for mode "'+(E.cN||"<unnamed>")+'"');return k+=n,n.length||1}var N=w(e);if(!N)throw new Error('Unknown language: "'+e+'"');s(N);var R,E=i||N,x={},L="";for(R=E;R!==N;R=R.parent)R.cN&&(L=p(R.cN,"",!0)+L);var k="",B=0;try{for(var M,j,O=0;;){if(E.t.lastIndex=O,M=E.t.exec(t),!M)break;j=m(t.substring(O,M.index),M[0]),O=M.index+j}for(m(t.substr(O)),R=E;R.parent;R=R.parent)R.cN&&(L+=C);return{r:B,value:L,language:e,top:E}}catch(T){if(T.message&&-1!==T.message.indexOf("Illegal"))return{r:0,value:n(t)};throw T}}function g(e,t){t=t||I.languages||x(y);var r={r:0,value:n(e)},a=r;return t.filter(w).forEach(function(n){var t=f(n,e,!1);t.language=n,t.r>a.r&&(a=t),t.r>r.r&&(a=r,r=t)}),a.language&&(r.second_best=a),r}function p(e){return I.tabReplace||I.useBR?e.replace(M,function(e,n){return I.useBR&&"\n"===e?"<br>":I.tabReplace?n.replace(/\t/g,I.tabReplace):""}):e}function h(e,n,t){var r=n?L[n]:t,a=[e.trim()];return e.match(/\bhljs\b/)||a.push("hljs"),-1===e.indexOf(r)&&a.push(r),a.join(" ").trim()}function d(e){var n,t,r,o,l,s=i(e);a(s)||(I.useBR?(n=document.createElementNS("http://www.w3.org/1999/xhtml","div"),n.innerHTML=e.innerHTML.replace(/\n/g,"").replace(/<br[ \/]*>/g,"\n")):n=e,l=n.textContent,r=s?f(s,l,!0):g(l),t=u(n),t.length&&(o=document.createElementNS("http://www.w3.org/1999/xhtml","div"),o.innerHTML=r.value,r.value=c(t,u(o),l)),r.value=p(r.value),e.innerHTML=r.value,e.className=h(e.className,s,r.language),e.result={language:r.language,re:r.r},r.second_best&&(e.second_best={language:r.second_best.language,re:r.second_best.r}))}function b(e){I=o(I,e)}function v(){if(!v.called){v.called=!0;var e=document.querySelectorAll("pre code");E.forEach.call(e,d)}}function m(){addEventListener("DOMContentLoaded",v,!1),addEventListener("load",v,!1)}function N(n,t){var r=y[n]=t(e);r.aliases&&r.aliases.forEach(function(e){L[e]=n})}function R(){return x(y)}function w(e){return e=(e||"").toLowerCase(),y[e]||y[L[e]]}var E=[],x=Object.keys,y={},L={},k=/^(no-?highlight|plain|text)$/i,B=/\blang(?:uage)?-([\w-]+)\b/i,M=/((^(<[^>]+>|\t|)+|(?:\n)))/gm,C="</span>",I={classPrefix:"hljs-",tabReplace:null,useBR:!1,languages:void 0};return e.highlight=f,e.highlightAuto=g,e.fixMarkup=p,e.highlightBlock=d,e.configure=b,e.initHighlighting=v,e.initHighlightingOnLoad=m,e.registerLanguage=N,e.listLanguages=R,e.getLanguage=w,e.inherit=o,e.IR="[a-zA-Z]\\w*",e.UIR="[a-zA-Z_]\\w*",e.NR="\\b\\d+(\\.\\d+)?",e.CNR="(-?)(\\b0[xX][a-fA-F0-9]+|(\\b\\d+(\\.\\d*)?|\\.\\d+)([eE][-+]?\\d+)?)",e.BNR="\\b(0b[01]+)",e.RSR="!|!=|!==|%|%=|&|&&|&=|\\*|\\*=|\\+|\\+=|,|-|-=|/=|/|:|;|<<|<<=|<=|<|===|==|=|>>>=|>>=|>=|>>>|>>|>|\\?|\\[|\\{|\\(|\\^|\\^=|\\||\\|=|\\|\\||~",e.BE={b:"\\\\[\\s\\S]",r:0},e.ASM={cN:"string",b:"'",e:"'",i:"\\n",c:[e.BE]},e.QSM={cN:"string",b:'"',e:'"',i:"\\n",c:[e.BE]},e.PWM={b:/\b(a|an|the|are|I'm|isn't|don't|doesn't|won't|but|just|should|pretty|simply|enough|gonna|going|wtf|so|such|will|you|your|they|like|more)\b/},e.C=function(n,t,r){var a=e.inherit({cN:"comment",b:n,e:t,c:[]},r||{});return a.c.push(e.PWM),a.c.push({cN:"doctag",b:"(?:TODO|FIXME|NOTE|BUG|XXX):",r:0}),a},e.CLCM=e.C("//","$"),e.CBCM=e.C("/\\*","\\*/"),e.HCM=e.C("#","$"),e.NM={cN:"number",b:e.NR,r:0},e.CNM={cN:"number",b:e.CNR,r:0},e.BNM={cN:"number",b:e.BNR,r:0},e.CSSNM={cN:"number",b:e.NR+"(%|em|ex|ch|rem|vw|vh|vmin|vmax|cm|mm|in|pt|pc|px|deg|grad|rad|turn|s|ms|Hz|kHz|dpi|dpcm|dppx)?",r:0},e.RM={cN:"regexp",b:/\//,e:/\/[gimuy]*/,i:/\n/,c:[e.BE,{b:/\[/,e:/\]/,r:0,c:[e.BE]}]},e.TM={cN:"title",b:e.IR,r:0},e.UTM={cN:"title",b:e.UIR,r:0},e.METHOD_GUARD={b:"\\.\\s*"+e.UIR,r:0},e});

hljs.registerLanguage("powershell", function(e) {
    return {
        aliases: ["ps"],
        l: /-?[A-z\.\-]+/,
        cI: !0,
        k: {
        }
    }
});

// I know this is ugly, but I don't know how else to solve Asar's label rules...
var asar_opcodes = ["db", "dw", "dl", "dd", "adc", "and", "asl", "bcc", "blt", "bcs", "bge", "beq", "bit", "bmi", "bne", "bpl", "bra", "brk", "brl", "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cop", "cpx", "cpy", "dec", "dea", "dex", "dey", "eor", "inc", "ina", "inx", "iny", "jmp", "jml", "jsr", "jsl", "lda", "ldx", "ldy", "lsr", "mvn", "mvp", "nop", "ora", "pea", "pei", "per", "pha", "phb", "phd", "phk", "php", "phx", "phy", "pla", "plb", "pld", "plp", "plx", "ply", "rep", "rol", "ror", "rti", "rtl", "rts", "sbc", "sec", "sed", "sei", "sep", "sta", "stp", "stx", "sty", "stz", "tax", "tay", "tcd", "tcs", "tdc", "trb", "tsc", "tsb", "tsx", "txa", "txs", "txy", "tya", "tyx", "wai", "wdm", "xba", "xce", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "add", "alt1", "alt2", "alt3", "asr", "bic", "cache", "cmode", "color", "div2", "fmult", "from", "getb", "getbh", "getbl", "getbs", "getc", "hib", "ibt", "iwt", "ldb", "ldw", "link", "ljmp", "lm", "lms", "lmult", "lob", "loop", "merge", "mult", "not", "or", "plot", "ramb", "romb", "rpix", "sbk", "sex", "sm", "sms", "stb", "stop", "stw", "sub", "swap", "to", "umult", "with", "xor", "addw", "ya", "and1", "bbc0", "bbc1", "bbc2", "bbc3", "bbc4", "bbc5", "bbc6", "bbc7", "bbs0", "bbs1", "bbs2", "bbs3", "bbs4", "bbs5", "bbs6", "bbs7", "call", "cbne", "clr0", "clr1", "clr2", "clr3", "clr4", "clr5", "clr6", "clr7", "clrc", "clrp", "clrv", "cmpw", "daa", "das", "dbnz", "decw", "di", "div", "ei", "eor1", "incw", "mov", "sp", "mov1", "movw", "mul", "not1", "notc", "or1", "pcall", "pop", "push", "ret", "reti", "set0", "set1", "set2", "set3", "set4", "set5", "set6", "set7", "setc", "setp", "sleep", "subw", "tcall", "tclr", "tset", "xcn", "lea", "move", "moves", "moveb", "movew"];

var asar_keywords = ["lorom", "hirom", "exlorom", "exhirom", "sa1rom", "fullsa1rom", "sfxrom", "norom", "macro", "endmacro", "struct", "endstruct", "extends", "incbin", "incsrc", "fillbyte", "fillword", "filllong", "filldword", "fill", "padbyte", "pad", "padword", "padlong", "paddword", "table", "cleartable", "ltr", ",rtl", "skip", "namespace", "import", "print", "org", "warnpc", "base", "on", "off", "reset", "freespaceuse", "pc", "bytes", "hex", "freespace", "freecode", "freedata", "ram", "noram", "align", "cleaned", "static", "autoclean", "autoclear", "prot", "pushpc", "pullpc", "pushbase", "pullbase", "function", "if", "else", "elseif", "endif", "while", "assert", "arch", "65816", "spc700", "inline", "superfx", "math", "pri", "round", "xkas", "bankcross", "bank", "noassume", "auto", "asar", "includefrom", "includeonce", "include", "error", "skip", "double", "round", "pushtable", "pulltable", "undef", "check", "title", "nested", "warnings", "push", "pull", "disable", "enable", "warn", "address", "dpbase", "optimize", "dp", "none", "always", "default", "mirrors"];

var asar_intrinsic_functions = ["read1", "read2", "read3", "read4", "canread1", "canread2", "canread4", "sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "arcsin", "arccos", "arctan", "log", "log10", "log2", "_read1", "_read2", "_read3", "_read4", "_canread1", "_canread2", "_canread4", "_sqrt", "_sin", "_cos", "_tan", "_asin", "_acos", "_atan", "_arcsin", "_arccos", "_arctan", "_log", "_log10", "_log2", "readfile1", "_readfile1", "readfile2", "_readfile2", "readfile3", "_readfile3", "readfile4", "_readfile4", "canreadfile1", "_canreadfile1", "canreadfile2", "_canreadfile2", "canreadfile3", "_canreadfile3", "canreadfile4", "_canreadfile4", "canreadfile", "_canreadfile", "filesize", "_filesize", "getfilestatus", "_getfilestatus", "snestopc", "_snestopc", "pctosnes", "_pctosnes", "max", "_max", "min", "_min", "clamp", "_clamp", "safediv", "_safediv", "select", "_select", "not", "_not", "equal", "_equal", "notequal", "_notequal", "less", "_less", "lessequal", "_lessequal", "greater", "_greater", "greaterequal", "_greaterequal", "and", "_and", "or", "_or", "nand", "_nand", "nor", "_nor", "xor", "_xor", "defined", "_defined", "sizeof", "_sizeof", "objectsize", "_objectsize", "stringsequal", "_stringsequal", "stringsequalnocase", "_stringsequalnocase"];

var asar_opcodes_not = asar_opcodes.join("|");
var asar_keywords_not = asar_keywords.join("|");
var asar_intrinsic_functions_not = asar_intrinsic_functions.join("|");

hljs.registerLanguage("65c816_asar",
	function(s)
	{
		return {
			cI: !0,
			aliases: ["asar"],
			l: s.IR,
			k:
			{
				opcodes: asar_opcodes.join(' '),
				keywords: asar_keywords.join(' '),
				built_in: asar_intrinsic_functions.join(' ')
			},
			c:
			[
				{
					cN: "built_in",
					b: "(" + asar_intrinsic_functions.join('|') + ")\\(",
					e: "\\)"
				},
				s.C("[;]", "$"),
				s.CBCM,
				s.QSM,
				{
					cN: "string",
					b: '"',
					e: '"',
					r: 0
				},
				{
					cN: "string",
					b: '\'',
					e: '\'',
					r: 0
				},
				{
					cN: "special",
					b: '\\s*^@',
					e: '$',
					r: 0
				},
				{
					cN: "keywords",
					b: asar_keywords.join('\\b|').concat('\\b')
				},
				{
					cN: "number",
					v:
					[
						{
							b: "#?[+\\-~]?0x[0-9a-fA-F]+"
						},
						{
							b: "#?[+\\-~]?[0-9]+(\\.[0-9]+)?"
						},
						{
							b: "#?[+\\-~]?%[0-1]+"
						},
						{
							b: "#?[+\\-~]?\\$[0-9a-fA-F]+"
						}/*,
						{
							b: "#?(\\(|\\)|\\+|\\-|\\*|\\/|\\%|\\<\\<|\\>\\>|\\&|\\||\\^|\\~|\\*\\*)+"
						}*/
					],
					r: 0
				},
				{
					cN: "function",
					v:
					[
						{
							b: "%?[a-zA-Z0-9_]+\\(",
							e: "\\)"
						}
					],
					r: 0
				},
				{
					cN: "define",
					v:
					[
						{
							b: "![a-zA-Z0-9_{}]+"
						},
						{
							b: "<[a-zA-Z0-9_]+>"
						},
					],
					r: 0
				},
				{
					cN: "opcodes",
					b: asar_opcodes.join('(\\.[bwl]|\\b)|').concat('(\\.[bwl]|\\b)')
				},
				{
					cN: "label",
					v:
					[
						{
							b: "#?\\??\\.*[a-zA-Z0-9_]+:?"
						},				
						{
							b: "\\??(-+|\\++):?"
						}
					],
					r: 0
				}
			],
			i: "/"
		}
	}
);