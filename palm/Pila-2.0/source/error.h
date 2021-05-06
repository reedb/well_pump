/****************************************************************
 * 
 ****************************************************************/

#ifndef __ERROR_H__
#define __ERROR_H__

#define ERROR_CODE_LIST \
  \
  /* No Error */ \
  ERRCODE(OK,							"OK") \
										\
  /* Warnings */ \
  ERRCODE(WARNING,						"warning") \
  \
  ERRCODE(ASCII_TOO_BIG,				"ASCII constant exceeds 4 characters") \
  ERRCODE(NUMBER_TOO_BIG,				"numeric constant exceeds 32 bits") \
  ERRCODE(OCTAL_TOO_BIG,				"octal constant exceeds 8 bits") \
  ERRCODE(TYPE_IGNORED,					"specified type was ignored") \
  ERRCODE(LABEL_IGNORED,				"label ignored") \
  ERRCODE(GLOBAL_DATA_ADDRESS_NOT_A5,	"global data field not addressed through A5") \
  ERRCODE(STACK_ADDRESS_NOT_A6,			"stack data not addressed through A6") \
  ERRCODE(CODE_ADDRESS_NOT_PC,			"code label not addressed through PC") \
  ERRCODE(PC_WITH_NON_CODE_ADDR,		"PC indirect displacement not a code address") \
  ERRCODE(ABSOLUTE_ADDRESS,				"absolute address used") \
  ERRCODE(IMMEDIATE_NOT_A_CONSTANT,		"immediate value not a constant") \
  ERRCODE(INSTR_AND_OPER_SIZE_MISMATCH,	"mismatch between size of instruction and size of operand") \
  ERRCODE(INTERNAL_ERROR_GUARD_NOT_DEF,	"internal error - guard symbol not found on pass 2") \
  ERRCODE(ALIGNMENT_WARNING,			"implicit alignment to word boundary") \
										\
  /* Minor errors */ \
  ERRCODE(MINOR,						"minor Error") \
  \
  ERRCODE(INV_SIZE_CODE,				"invalid size code") \
  ERRCODE(INV_QUICK_CONST,				"MOVEQ instruction constant out of range") \
  ERRCODE(INV_VECTOR_NUM,				"invalid vector number") \
  ERRCODE(INV_BRANCH_DISP,				"branch instruction displacement is out of range or invalid") \
  ERRCODE(INV_DISP,						"displacement out of range") \
  ERRCODE(INV_ABS_ADDRESS,				"absolute address exceeds 16 bits") \
  ERRCODE(INV_8_BIT_DATA,				"immediate data exceeds 8 bits") \
  ERRCODE(INV_16_BIT_DATA,				"immediate data exceeds 16 bits") \
  ERRCODE(ODD_ADDRESS,					"origin value is odd (location counter set to next highest address)") \
  ERRCODE(NOT_REG_LIST,					"the symbol specified is not a register list symbol") \
  ERRCODE(REG_LIST_SPEC,				"register list symbol used in an expression") \
  ERRCODE(INV_SHIFT_COUNT,				"invalid constant shift count") \
										\
  /* Errors */ \
  ERRCODE(ERROR_RANGE,					"error") \
  \
  ERRCODE(UNDEFINED_SYMBOL,				"undefined symbol") \
  ERRCODE(DIV_BY_ZERO,					"division by zero attempted") \
  ERRCODE(MULTIPLE_DEFS,				"symbol multiply defined") \
  ERRCODE(REG_MULT_DEFS,				"register list name used multiple times") \
  ERRCODE(REG_LIST_UNDEF,				"register list symbol not defined") \
  ERRCODE(INV_FORWARD_REF,				"forward references not allowed with this directive") \
  ERRCODE(INV_LENGTH,					"block length is less that zero") \
  ERRCODE(KIND_DIFFERENT,				"using the same id for two different things") \
  ERRCODE(INV_VALUE_CATEGORY,			"value has an invalid type") \
  ERRCODE(INV_SYMBOL_IN_EXPR,			"symbol can not be used in expression") \
  ERRCODE(INV_HEX_CONSTANT,				"hex constant begins with invalid character") \
  ERRCODE(UNDEFINED_TYPE,				"undefined type") \
  ERRCODE(INV_PARM_SIZE,				"invalid or missing parameter size") \
  ERRCODE(NOT_A_PROCEDURE_NOR_TRAP,		"symbol undefined or not a procedure nor a trap") \
  ERRCODE(DECLARED_BUT_UNDEFINED_PROC,	"procedure is declared but no entry point defined") \
  ERRCODE(ID_TOO_LONG,					"identifier too long") \
  ERRCODE(STRING_TOO_LONG,				"string too long") \
  ERRCODE(GLOBAL_NOT_IN_DATA,			"GLOBAL directive not in data block") \
  ERRCODE(BIT_COUNT_TOO_BIG,			"number of bits exceeds specified type length") \
  ERRCODE(INVALID_BITMAP_MEMBER_TYPE,	"different member types within one bitmap") \
  ERRCODE(BITMAP_INCOMPLETE,			"bitmap structure does not span specified type") \
  ERRCODE(UNMATCHING_TYPE_SIZES,		"unmatching type sizes") \
  ERRCODE(USER_ERROR,					"Error") \
  ERRCODE(TEMP_LABEL_CODE_ONLY,			"temporary labels can only be used for code labels") \
  \
  /* Severe Errors */ \
  ERRCODE(SEVERE,						"severe Error") \
  \
  ERRCODE(SYNTAX,						"invalid syntax") \
  ERRCODE(UNSUCCESSFULL_SHORT_BRANCH,   "add .l to branch instruction to suppress optimization failure") \
  ERRCODE(INCOMPLETE_PARAMETER_LIST,	"incomplete parameter list") \
  ERRCODE(TOO_MANY_PARAMETERS,			"too many parameters specified") \
  ERRCODE(MISSING_PARAMETERS,			"missing parameter") \
  ERRCODE(EXPECTED_PAREN_OR_PARM,		"expected \')\' or parameter specification") \
  ERRCODE(EXPECTED_PARAMETER_ID,		"expected parameter identifier") \
  ERRCODE(EXPECTED_PAREN_OR_COMMA,		"expected \')\' or \',\'") \
  ERRCODE(EXPECTED_ENUM_NAME,			"expected ENUM name") \
  ERRCODE(EXPECTED_ENUM_MEMBER,			"expected ENUM member or endenum") \
  ERRCODE(EXPECTED_STRUCT_NAME,			"expected STRUCT name") \
  ERRCODE(EXPECTED_STRUCT_MEMBER,		"expected STRUCT member or endstruct") \
  ERRCODE(EXPECTED_UNION_NAME,			"expected UNION name") \
  ERRCODE(EXPECTED_UNION_MEMBER,		"expected UNION member or endunion") \
  ERRCODE(UNEXPECTED_ENDSTRUCT,			"unexpected ENDENUM/ENDSTRUCT/ENDUNION") \
  ERRCODE(EXPECTED_LOCAL_VAR_ID,		"expected local variable id") \
  ERRCODE(EXPECTED_GLOBAL_VAR_ID,		"expected global variable id") \
  ERRCODE(EXPECTED_EXTERN_VAR_ID,		"expected external variable id") \
  ERRCODE(EXPECTED_EXPRESSION,			"expected expression") \
  ERRCODE(UNEXPECTED_ENDPROC,			"unexpected ENDPROC") \
  ERRCODE(GUARD_ERROR,					"value changed from pass 1 to pass 2") \
  ERRCODE(UNTERMINATED_STRING,			"unterminated string") \
  ERRCODE(UNMATCHED_RIGHT_PAREN,		"unmatched right parenthesis") \
  ERRCODE(UNMATCHED_RIGHT_BRACKET,		"unmatched right bracket") \
  ERRCODE(INCOMPLETE_PARAMETER_SPEC,	"incomplete parameter specification") \
  ERRCODE(UNEXPECTED_ENDENUM,			"unexpected ENDENUM found") \
  ERRCODE(UNEXPECTED_ENDUNION,			"unexpected ENDUNION found") \
  ERRCODE(UNEXPECTED_ENUM,				"unexpected ENUM found") \
  ERRCODE(UNEXPECTED_STRUCT,			"unexpected STRUCT found") \
  ERRCODE(UNEXPECTED_UNION,				"unexpected UNION found") \
  ERRCODE(UNEXPECTED_BEGINPROC,			"unexpected BEGINPROC found") \
  ERRCODE(UNEXPECTED_LOCAL,				"unexpected LOCAL directive") \
  ERRCODE(INVALID_BOOLEAN_VALUE,		"expression does not calculate to 0 or 1") \
  ERRCODE(UNMATCHED_ENDIF,				"ENDIF without prior IF/IFDEF/IFNDEF") \
  ERRCODE(MISSING_ENDIF,				"missing ENDIF for prior IF/IFDEF/IFNDEF") \
  ERRCODE(UNEXPECTED_ELSE_MISSING_IF,	"ELSE found without prior IF/IFDEF/IFNDEF") \
  ERRCODE(UNEXPECTED_ELSE_MULTIPLE,		"multiple ELSE for one IF/IFDEF/IFNDEF") \
  ERRCODE(UNEXPECTED_ENTRY_DEFINITION,	"unexpected entry point definition") \
  ERRCODE(UNEXPECTED_ENDPROXY,			"unexpected ENDPROXY found") \
  ERRCODE(INVALID_TEMP_LABEL,			"invalid temporary label") \
  ERRCODE(MISSING_APPL,					"required directive APPL is missing") \
  ERRCODE(MISSING_APPL_NAME,			"missing application name in APPL directive") \
  ERRCODE(MISSING_CREATOR_ID,			"missing application ID in APPL directive") \
  ERRCODE(LABEL_REQUIRED,				"label required with this directive") \
  ERRCODE(INV_OPCODE,					"invalid opcode") \
  ERRCODE(INV_OPERATOR,					"invalid operator") \
  ERRCODE(INV_ADDR_MODE,				"invalid addressing mode") \
  ERRCODE(PHASE_ERROR,					"symbol value differs between first and second pass") \
  ERRCODE(RESOURCE_OPEN_FAILED,			"failed to open resource file") \
  ERRCODE(RESOURCE_TOO_BIG,				"resource file too big") \
  ERRCODE(INCLUDE_OPEN_FAILED,			"failed to open include file") \
  ERRCODE(INCLUDE_NESTED_TOO_DEEP,		"include files neted too deep") \
  ERRCODE(MISSING_TRAP_DEF,				"missing trap definition") \
  ERRCODE(MISSING_TYPE_SPEC,			"missing type specification") \
  ERRCODE(INTERNAL_ERROR_SYMBOL_KIND,	"internal error - invalid symbol kind for procedure name space") \
  ERRCODE(INTERNAL_ERROR_NO_CURR_PROC,	"internal error - name space creation without current procedure") \
  ERRCODE(INTERNAL_ERROR_OP_UNKNOWN,	"internal error - invalid operator in doOperation") \
  ERRCODE(INTERNAL_ERROR_OUT_OF_MEMORY,	"internal error - unable to allocate necessary memory") \
  ERRCODE(EXPR_NESTED_TOO_DEEP,			"expression nested too deep") \
  ERRCODE(INTERNAL_ERROR_INVALID_MEMBERED_TYPE,		"internal error - invalid membered type") \
  ERRCODE(INTERNAL_ERROR_INVALID_DERIVATION_TYPE,	"internal error - invalid derivation type") \
  ERRCODE(EXPECTED_LEFT_PAREN,			"expected \')\'") \
  ERRCODE(EXPECTED_RIGHT_PAREN,			"expected \'(\'") \
  ERRCODE(EXPECTED_SYMBOL,				"expected symbol") \
  ERRCODE(EXPECTED_STRING,				"expected string") \
  ERRCODE(EXPECTED_CONSTANT,			"expected constant") \
  ERRCODE(EXPECTED_TYPE_NAME,			"expected type name") \
  ERRCODE(EXPECTED_PROC_NAME,			"expected procedure/trap name") \
  ERRCODE(EXPECTED_LEFT_BRACKET,		"expected \'[\'") \
  ERRCODE(EXPECTED_RIGHT_BRACKET,		"expected \']\'") \
  ERRCODE(EXPECTED_PERIOD_BEFORE_TYPE,	"expected \'.\' between variable and type") \
  /****** EOL ******/

/* Error Codes */
typedef enum _ErrorCode
{
#define ERRCODE(x,y) x,
	ERROR_CODE_LIST
#undef ERRCODE
} ErrorCode;

void ErrorStartReporting();
int  ErrorGetWarningCount();
int  ErrorGetErrorCount();
int  ErrorStatusIsSevere();
int  ErrorStatusIsError();
int  ErrorStatusIsMinor();
int  ErrorStatusIsWarning();
int  ErrorStatusIsOK();
void ErrorStatusReset();
void ErrorWrite();
void Error(ErrorCode,char *);

#endif
