/***********************************************************************
 *
 *      EVAL.C
 *      Expression Evaluator for 68000 Assembler
 *
 *    Function: eval()
 *      Evaluates an expression. The p argument points
 *      to the string to be evaluated, and the function returns
 *      a pointer to the first character beyond the end of the
 *      expression. The value of the expression is returned via
 *      an output argument. A possible error condition is returned
 *      throught the function Error().
 *
 *   Usage: char *evaluate(p, valuePtr)
 *      char     *p;
 *      Value   **valuePtr;
 *
 *  Errors:
 *      DIV_BY_ZERO
 *      ASCII_TOO_BIG
 *      NUMBER_TOO_BIG
 *      INV_SYMBOL_IN_EXPR
 *      SYNTAX
 *      INV_OPCODE
 *      UNDEFINED_SYMBOL
 *
 *      Author: Paul McKee
 *      ECE492    North Carolina State University
 *
 *        Date: 9/24/86
 *************************************************************************
 * CHANGES
 *
 * 2003-07-23  FSCH  F. Schaeckermann  allowing spaces in expression
 *                                     using new symbol module 
 *                                     keeping track of kind of expression
 *                                     (code, data, local, parm, const)
 *                                     additional operators
 *
 ************************************************************************/


#include "pila.h"
#include "asm.h"
#include "parse.h"

#include "safe-ctype.h"

/* Largest number that can be represented in an unsigned int
   - MACHINE DEPENDENT */
#define INTLIMIT 0xFFFF
#define LONGLIMIT 0xFFFFFFFF

typedef enum
{
  invalid,		// not a valid operator nor an expression-terminator
  terminator,		// not a valid operator but an expression-terminator
  equal,		// ==
  lower,		// <
  lowerorequal,		// <= or =<
  higher,		// >
  higherorequal,	// >= or =>
  bitor,		// |
  bitand,		// &
  bitxor,		// ^
  plus,			// +
  minus,		// -
  multiply,		// *
  divide,		// /
  modulo,		// //
  shiftleft,		// <<
  shiftright,		// >>
} Operator;

// Maximum stack size for the evaluation stack.
#define MAX_EVAL_STACK 7
// the stack depth can never be more than the number of different
// operator precedences plus one. Therefore 7 will be sufficient.

typedef struct _StackEntry
{
  Value    value;	// value with kind specification
  Operator operator;	// operator following the operand stored in value
} StackEntry;

char *evaluateOperand(char *p, Value *val)
{
  long       base;
  long       x;
  char       name[SIGCHARS+1];
  SymbolDef *symbol;
  
  int     i;
  boolean endFlag;
  int     ch;
  boolean tooBig = false;
  char   *start = p;

  val->value = 0;
  val->kind  = symbolKindConst;
  val->type  = NULL;
  
  p = skipSpace(p); 

  // Unary minus
  if (*p == '-')
  { 
    /* Evaluate unary minus operator recursively */
    p = evaluateOperand(++p, val);
    val->value = -(val->value);
    return p;
  } 
  
  // One's complement
  else if (*p == '~')
  { 
    /* Evaluate one's complement operator recursively */
    p = evaluateOperand(++p, val);
    val->value = ~(val->value);
    return p;
  }
  
  // Parenthesized expression
  else if (*p == '(')
  {
    /* Evaluate parenthesized expressions recursively */
    p = evaluate(++p, val);
    if (!p || ErrorStatusIsSevere())
      return NULL;
    else if (*p!=')')
    {
      Error(EXPECTED_RIGHT_PAREN,p);
      return p;
    }
    else
      return ++p;
  }

  // Hex number
  else if ((*p == '$' && ISXDIGIT(*(p+1))) || 
           (*p=='0' && (*(p+1)=='x' || *(p+1)=='X') && ISXDIGIT(*(p+2))))
  {
    // Convert hex digits until another character is
    // found. (At least one hex digit is present.)
    if (*p=='0')
      p++;
    x = 0;
    while (ISXDIGIT(*++p) || *p=='.')
    {
      if (*p=='.')
        p++;
      else
      {
        if ((unsigned long)x > (unsigned long)LONGLIMIT/16)
          tooBig = true;
        ch = TOUPPER(*p);
        if (ch > '9') 
          x = 16 * x + (ch - 'A' + 10);
        else
          x = 16 * x + (ch - '0');
      }
    }
    if (*p=='u' || *p=='U')
      p++;
    if (*p=='l' || *p=='L')
      p++;
    if (tooBig)
      Error(NUMBER_TOO_BIG,start);
    val->value = x;
    return p;
  }

  // Binary, Octal, Decimal number
  else if (*p == '%' || ISDIGIT(*p))
  {
    // Convert digits in the appropriate base (binary,
    // octal, or decimal) until an invalid digit is found.
    if (*p=='%')
    {
      base = 2;
      p++;
    }
    else if (*p=='0')
    {
      base = 8;
    }
    else
    {
      base = 10;
    }
    /* Check that at least one digit is present */
    if (*p<'0' || *p>='0'+base)
    {
      Error(SYNTAX,p);
      return NULL;
    }
    
    x = 0;
    /* Convert the digits into an integer */
    while ((*p>='0' && *p<'0'+base) || *p=='.')
    {
      if (*p!='.')
      {
        if ((unsigned long)x > (LONGLIMIT-(*p-'0'))/base)
          tooBig = true;
        x = (long) ( (long) ((long) base * x) + (long) (*p - '0') );
      }
      p++;
    }
    if (*p=='u' || *p=='U')
      p++;
    if (*p=='l' || *p=='L')
      p++;
    if (tooBig)
      Error(NUMBER_TOO_BIG,start);
    val->value = x;
    return p;
  }

  // Quoted string
  else if (*p == '\'')
  {
// The following escape sequences allow special characters to be put into the string.
// Escape      Name             Meaning
// Sequence
// \a          Alert            Produces an audible or visible alert (ctrl-g)
// \b          Backspace        Moves the cursor back one position (non-destructive) (ctrl-h)
// \f          Form Feed        Moves the cursor to the first position of the next page (ctrl-l)
// \n          New Line         Moves the cursor to the first position of the next line (ctrl-j)
// \r          Carriage Return  Moves the cursor to the first position of the current line (ctrl-m)
// \t          Horizontal Tab   Moves the cursor to the next horizontal tabular position (ctrl-i)
// \v          Vertical Tab     Moves the cursor to the next vertical tabular position (ctrl- )????
// \'                           Produces a single quote
// \"                           Produces a double quote
// \?                           Produces a question mark
// \\                           Produces a single backslash
// \0                           Produces a null character
// \ddd                         Defines one character by the octal digits (base-8 number)
// \xdd                         Defines one character by the hexadecimal digit (base-16 number)
    endFlag = false;
    i = 0;
    x = 0;
    p++;
    while (!endFlag)
    {
      if (*p=='\\')
      {
        int c = *(++p);
        p++;
        switch (c)
        {
          case 'a': c='\x07'; break; // alert
          case 'b': c='\x08'; break; // backspace
          case 'f': c='\x0C'; break; // form feed
          case 'n': c='\x0A'; break; // new line
          case 'r': c='\x0D'; break; // carriage return
          case 't': c='\x09'; break; // tab
          case 'v': c='\x0B'; break; // vertical tab
          case '0': c='\x00'; break; // null charachter
          case '1':                  // octal number (max 3 digits)
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
            c -= '0';
            if (*p>='0' && *p<='7')
              c = c*8+*(p++)-'0';
            if (*p>='0' && *p<='7')
              c = c*8+*(p++)-'0';
            if (c>255)
            {
              Error(OCTAL_TOO_BIG,p-4);
              c &= 255;
            }
            break;
          case 'x':                  // hex number (max 2 digits)
            c = *(p++);
            if (c<'0' || (c>'9' && c<'A') || (c>'F' && c<'a') || c>'f')
            {
              c = c & 15;
              Error(INV_HEX_CONSTANT,p-3);
            }
            else
            {
              if (*p>='0' && *p<='9')
                c = c*16+(*(p++))-'0';
              else if (*p>='A' && *p<='F')
                c = c*16+(*(p++))-'A'+10;
              else if (*p>='a' && *p<='f')
                c = c*16+(*(p++))-'a'+10;
            }
            break;
          default:
            break;
        }
        x = (x << 8) + c;
        i++;
        p--; // put it back to the last processed character
      }
      else if (!*p)
      {
        Error(UNTERMINATED_STRING,start);
        endFlag = true;
      }
      else if (*p=='\'')
        endFlag = true;
      else
      {
        x = (x << 8) + *p;
        i++;
      }
      p++;
    }
    
    if (i == 0)
    {
      Error(SYNTAX,start);
      return NULL;
    }
    else if (i == 3)
    {
      x = x << 8;
    }
    else if (i > 4)
    {
      Error(ASCII_TOO_BIG,start);
    }
    val->value = x;
    return p;
  }
  
  // check for temporary label reference
  else if (*p=='.')
  {
    char c = *(p+1);
    if (c>='1' && c<='9')
    {
      c = *(p+2);
      if (c=='f' || c=='F' || c=='b' || c=='B')
      {
        c = *(p+3);
        if (!ISALNUM(c) && c!='.' && c!='_' && c!='$' && c!='?' && c!='@')
        {
          symbol = SymbolLookupTempLabel(*(p+1),*(p+2));
          if (!symbol)
          {
            Error(UNDEFINED_SYMBOL,start);
            val->kind = symbolKindUndefined;
          }
          else
          {
            val->value = SymbolGetValue(symbol);
            val->kind  = symbolKindCode;
          }
          return p+3;
        }
      }
    }
    Error(INVALID_TEMP_LABEL,p);
    return NULL;
  }

  // Symbol (symbols can start with an alphabetic char or '_', '?', '@'
  // and include letters, numbers, '.', '_', '$', '?', '@')
  else if (ISALPHA(*p) || *p=='?' || *p=='_' || *p=='@')
  {
    char *symbolStart = p;
    
    /* get id of symbol and check for builtin functions first */
    p = ParseId(p,name);
    
    // is it built-in function sizeof?
    if (strcmp(name,"sizeof")==0)
    {
      p = skipSpace(p);
      if (*p!='(')
        Error(EXPECTED_LEFT_PAREN,p);
      else
        p = skipSpace(++p);
      start = p;
      if (ISALPHA(*p) || *p=='?' || *p=='_' || *p=='@')
      {
        // get the symbol
        p = ParseSymbol(p,&symbol,val); // we don't need the symbol kind
        if (!symbol)
        {
          Error(UNDEFINED_SYMBOL,start);
          val->kind = symbolKindUndefined;
        }
        else
        {
          val->value = SymbolGetSize(symbol); // kind is symbolKindConst
          val->kind  = symbolKindConst;
        }
      }
      else
      {
        Error(EXPECTED_SYMBOL,start);
        return NULL;
      }
      
      p = skipSpace(p);
      if (*p!=')')
      {
        Error(EXPECTED_RIGHT_PAREN,p);
        return p;
      }
      return ++p;
    }
    
    // it is not a built-in function
    // therefore get the whole (multi-level) symbol and it's kind
    p = ParseSymbol(symbolStart,&symbol,val);
    
    if (symbol)
    {
      SymbolCategory cat = SymbolGetCategory(val->kind);
      if (cat==symbolCategoryNone || cat==symbolCategoryType)
      {
        // since the symbol can not be used in expressions, return error
        Error(INV_SYMBOL_IN_EXPR,SymbolGetId(symbol));
      }
      return p; // value, kind and type have been set by ParseSymbol
    }
    else
    {
      Error(UNDEFINED_SYMBOL,symbolStart);
      val->kind = symbolKindUndefined;
      return p;
    }
  }
  else // the character was not a valid operand
  {
    Error(SYNTAX,start);
    return NULL;
  }
  return NULL;
}

char *parseOperator(char *s,Operator *opCode)
{
  char c;
  switch(*(s++))
  {
    case '\0':
    case ',':
    case '(':
    case ')':
    case ';':
    case '.':
    case ']':
      s--;
      *opCode = terminator;
      break;

    case '=':
      c = *(s++);
      if (c=='=')
        *opCode = equal;	// ==
      else if (c=='<')
        *opCode = lowerorequal;	// =<
      else if (c=='>')
        *opCode = higherorequal; // =>
      else
      {
        s -= 2;
        *opCode = invalid;
      }
      break;
      
    case '<':
      c = *(s++);
      if (c=='=')
        *opCode = lowerorequal;	// <=
      else if (c=='<')
        *opCode = shiftleft;	// <<
      else
      {
        *opCode = lower;	// <
        s--;
      }
      break;

    case '>':
      c = *(s++);
      if (c=='=')
        *opCode = higherorequal; // >=
      else if (c=='>')
        *opCode = shiftright;	// >>
      else
      {
        *opCode = higher;	// >
        s--;
      }
      break;

    case '|':
      *opCode = bitor;		// |
      break;
      
    case '&':
      *opCode = bitand;		// &
      break;

    case '^':
      *opCode = bitxor;		// ^
      break;

    case '+':
      *opCode = plus;		// +
      break;

    case '-':
      *opCode = minus;		// -
      break;

    case '*':
      *opCode = multiply;	// *
      break;

    case '/':
      if (*s=='/')
      {
        *opCode = modulo;	// //
        s++;
      }
      else
        *opCode = divide;	// /
      break;
      
    default:
      s--;
      *opCode = invalid;
      break;
  }
  return skipSpace(s);
}

int precedence(Operator opCode)
{
  /* Compute the precedence of an operator. Higher numbers indicate
     higher precedence, e.g., precedence('*') > precedence('+').
     'invalid' and 'terminator' will be assigned a precedence of zero */
  switch (opCode)
  {
    case invalid:	// not a valid operator nor an expression-terminator
    case terminator:	// not a valid operator but an expression-terminator
      return 0;
    case equal:		// ==
    case lower:		// <
    case lowerorequal:	// <= or =<
    case higher:	// >
    case higherorequal:	// >= or =>
      return 1;
    case bitor:		// |
    case bitand:	// &
    case bitxor:	// ^
      return 2;
    case plus:		// +
    case minus:		// -
      return 3;
    case multiply:	// *
    case divide:	// /
    case modulo:	// //
      return 4;
    case shiftleft:	// <<
    case shiftright:	// >>
      return 5;
  }
  return 0;
}

// gets two pointer to stack entry
// first stack entry contains the operator to execute
// first stack entry is also used to store result in
void doOperation(StackEntry *stackFirst, StackEntry *stackSecond)
{
  /* Performs the operation of the operator on the two operands.
     Checks the category of the to operators and sets appropriate
     error if they don't fit. Otherwise the calculation is done and
     a check for DIV_BY_ZERO is done. */
     
  long val1,val2,*result;
  SymbolCategory cat1 = SymbolGetCategory(stackFirst->value.kind);
  SymbolCategory cat2 = SymbolGetCategory(stackSecond->value.kind);
  
  stackFirst->value.type = NULL; // the result of any calculation does not have a type anymore
  
  if (stackFirst->value.kind==symbolKindUndefined ||
      stackSecond->value.kind==symbolKindUndefined)
  {
    stackFirst->value.kind = symbolKindUndefined;
  }
  else switch (stackFirst->operator)
  {
    case invalid:	// not a valid operator nor an expression-terminator
    case terminator:	// not a valid operator but an expression-terminator
      break;
      
    case equal:		// ==
    case lower:		// <
    case lowerorequal:	// <= or =<
    case higher:	// >
    case higherorequal:	// >= or =>
      stackFirst->value.kind = symbolKindConst;
      break;
      
    case bitor:		// |
    case bitand:	// &
    case bitxor:	// ^
      if (cat1==symbolCategoryConst)
        stackFirst->value.kind = stackSecond->value.kind;
      else if (cat2!=symbolCategoryConst && cat1!=cat2)
         Error(INV_VALUE_CATEGORY,NULL);
      break;

    case plus:		// +
    case minus:		// -
      if (cat1==symbolCategoryConst)
        stackFirst->value.kind = stackSecond->value.kind;
      else if (cat2!=symbolCategoryConst && cat1!=cat2)
        Error(INV_VALUE_CATEGORY,NULL);
      else
        stackFirst->value.kind = symbolKindConst;
      break;
      
    case multiply:	// *
      if (cat1==symbolCategoryConst)
        stackFirst->value.kind = stackSecond->value.kind;
      else if (cat2!=symbolCategoryConst)
         Error(INV_VALUE_CATEGORY,NULL);
      break;
      
    case divide:	// /
    case modulo:	// //
    case shiftleft:	// <<
    case shiftright:	// >>
      if (cat2!=symbolCategoryConst)
        Error(INV_VALUE_CATEGORY,NULL);
      break;
  } // else switch
    
  val1 = stackFirst->value.value;
  val2 = stackSecond->value.value;
  result = &(stackFirst->value.value);

  switch (stackFirst->operator)
  {
    case invalid:	// not a valid operator nor an expression-terminator
    case terminator:	// not a valid operator but an expression-terminator
      Error(INTERNAL_ERROR_OP_UNKNOWN,NULL);
      break;
    case equal:		// ==
      if (val1==val2)
        *result = 1;
      else
        *result = 0;
      break;
    case lower:		// <
      if (val1<val2)
        *result = 1;
      else
        *result = 0;
      break;
    case lowerorequal:	// <= or =<
      if (val1<=val2)
        *result = 1;
      else
        *result = 0;
      break;
    case higher:	// >
      if (val1>val2)
        *result = 1;
      else
        *result = 0;
      break;
    case higherorequal:	// >= or =>
      if (val1>=val2)
        *result = 1;
      else
        *result = 0;
      break;
    case bitor:		// |
      *result = val1 | val2;
      break;
    case bitand:	// &
      *result = val1 & val2;
      break;
    case bitxor:	// ^
      *result = val1 ^ val2;
      break;
    case plus:		// +
      *result = val1 + val2;
      break;
    case minus:		// -
      *result = val1 - val2;
      break;
    case multiply:	// *
      *result = val1 * val2;
      break;
    case divide:	// /
      if (val2 != 0)
        *result = val1 / val2;
      else
        Error(DIV_BY_ZERO,NULL);
      break;
    case modulo:	// //
      if (val2 != 0)
        *result = val1 % val2;
      else
        Error(DIV_BY_ZERO,NULL);
      break;
    case shiftleft:	// <<
      *result = val1 << val2;
      break;
    case shiftright:	// >>
      *result = val1 >> val2;
      break;
  }
}


char *evaluate(char *p, Value *valuePtr)
{
  StackEntry stack[MAX_EVAL_STACK];
  int        stackPtr = MAX_EVAL_STACK;
  int        prec;
  Operator   operator;
  
  valuePtr->value = 0;
  valuePtr->kind  = symbolKindUndefined;
  valuePtr->type  = NULL;
  
  /* Loop until terminator character is found (loop is exited via return)  */
  /* or the expression has too many levels for our stack to hold them.     */
  while (stackPtr>0)
  {
    /************************************************
     *      EXPECT AN OPERAND                       *
     ************************************************/
    /* Use evaluateOperand to read in a number or symbol */
    p = evaluateOperand(p, &(stack[--stackPtr].value));
    
    if (!p || ErrorStatusIsSevere()) // any error that doesn't allow us to go on?
      return NULL;
    
    /************************************************
     *      EXPECT AN OPERATOR                      *
     ************************************************/
    p = parseOperator(skipSpace(p),&operator);    
    prec = precedence(operator);
    
    /* Do all stacked operations that are of higher */
    /* precedence than the operator just examined.  */

    while (stackPtr<MAX_EVAL_STACK-1 && prec<=precedence(stack[stackPtr+1].operator))
    {
      doOperation(&(stack[stackPtr+1]),&(stack[stackPtr]));
      stackPtr++;
      if (ErrorStatusIsSevere()) // any error that doesn't allow us to go on?
        return NULL;
    }
    if (prec)
      stack[stackPtr].operator = operator;
    else if (operator==terminator)
    {
      /* If the character was an expression terminator */
      /* then return the calculated result.            */
      *valuePtr = stack[stackPtr].value;
      return p;
    }
    else
    {
      /* Otherwise report the syntax error */
      Error(INV_OPERATOR,p);
      return NULL;
    }
  }
  // we should never get here if MAX_STACK_SIZE is set
  // to at least number of operator precedences plus one.
  Error(EXPR_NESTED_TOO_DEEP,NULL);
  return NULL;
}
