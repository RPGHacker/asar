#pragma once


enum ir_command
{
	COMMAND_LABEL,
	COMMAND_ASSERT,
	COMMAND_IF,
	COMMAND_WHILE,
	COMMAND_ELSEIF,
	COMMAND_ENDWHILE,
	COMMAND_ENDIF,
	COMMAND_ELSE,
	COMMAND_OPCODE,
	COMMAND_DB,
	COMMAND_DW,
	COMMAND_DL,
	COMMAND_DD,
	COMMAND_MACRO,
	COMMAND_CALLMACRO,
	INTERNAL_COMMAND_NUMVARARGS,
	INTERNAL_COMMAND_ENDMACRO,
	COMMAND_ENDMACRO,
	COMMAND_UNDEF,
	COMMAND_ERROR,
	COMMAND_WARN,
	COMMAND_WARNINGS,
	COMMAND_GLOBAL,
	COMMAND_CHECK,
	COMMAND_ASAR,
	COMMAND_INCLUDE,
	COMMAND_INCLUDEFROM,
	COMMAND_INCLUDEONCE,
	COMMAND_SETLABEL,
	COMMAND_ORG,
	COMMAND_STRUCT,
	COMMAND_ENDSTRUCT,
	COMMAND_SPCBLOCK,
	COMMAND_ENDSPCBLOCK,
	COMMAND_STARTPOS,
	COMMAND_BASE,
	COMMAND_DPBASE,
	COMMAND_OPTIMIZE,
	COMMAND_BANK,
	COMMAND_FREESPACE,
	COMMAND_FREECODE,
	COMMAND_FREEDATA,
	COMMAND_PROT,
	COMMAND_AUTOCLEAN,
	COMMAND_PUSHPC,
	COMMAND_PULLPC,
	COMMAND_PUSHBASE,
	COMMAND_PULLBASE,
	COMMAND_PUSHNS,
	COMMAND_PULLNS,
	COMMAND_NAMESPACE,
	COMMAND_WARNPC,
	COMMAND_TABLE,
	INTERNAL_COMMAND_FILENAME,
	COMMAND_INCSRC,
	COMMAND_INCBIN,
	COMMAND_FILL,
	COMMAND_SKIP,
	COMMAND_CLEARTABLE,
	COMMAND_PUSHTABLE,
	COMMAND_PULLTABLE,
	COMMAND_FUNCTION,
	COMMAND_PRINT,
	COMMAND_RESET,
	COMMAND_PADBYTE,
	COMMAND_PADWORD,
	COMMAND_PADLONG,
	COMMAND_PADDWORD,
	COMMAND_PAD,
	COMMAND_FILLBYTE,
	COMMAND_FILLWORD,
	COMMAND_FILLLONG,
	COMMAND_FILLDWORD,
	COMMAND_ARCH,
	COMMAND_OPEN_BRACE,
	COMMAND_CLOSE_BRACE,
	COMMAND_LOROM,
	COMMAND_HIROM,
	COMMAND_EXLOROM,
	COMMAND_EXHIROM,
	COMMAND_SFXROM,
	COMMAND_NOROM,
	COMMAND_FULLSA1ROM,
	COMMAND_SA1ROM,
	COMMAND_EOL
};

enum ir_param_type
{
	IR_TYPE_NUM,
	IR_TYPE_DOUBLE,
	IR_TYPE_STRING
};

struct ir_tagged
{
	ir_param_type type;
	union{
		int64_t n;
		double d;
		string s;
	};

	ir_tagged()
	{
		type = IR_TYPE_NUM;
		n = 0;
	}

	ir_tagged(const ir_tagged &other)
	{
		*this = other;
	}

	ir_tagged &operator=(const ir_tagged &other)
	{
		type = other.type;
		if(type == IR_TYPE_STRING) new(&s) string((string &&)(other.s));
		else if(type == IR_TYPE_NUM) n = other.n;
		else if(type == IR_TYPE_DOUBLE) d = other.d;
		return *this;
	}

	template <typename T> ir_tagged(T v)
	{
		if constexpr(std::is_same_v<string, T> || std::is_same_v<char *, T>){ type = IR_TYPE_STRING;  new(&s) string(v); }
		else if constexpr(std::is_integral_v<T>){ type = IR_TYPE_NUM; n = v; }
		else if constexpr(std::is_same_v<double, T>){ type = IR_TYPE_DOUBLE;  d = v; }
		else static_assert(std::is_same_v<T,T> && 0, "unsupported IR type");
	}

	int64_t as_num()
	{
		return n;
	}

	double as_double()
	{
		return d;
	}

	string &as_string()
	{
		return s;
	}

	~ir_tagged()
	{
		if(type == IR_TYPE_STRING) s.~string();
	}
};

struct ir_block
{
	char *block;
	char **word;
	int numwords;
	ir_command command;
	autoarray<ir_tagged> params;

	~ir_block()
	{
		free(block);
		free(word);
	}
};
