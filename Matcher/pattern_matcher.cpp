/* ******************************************************************** **
** @@ 
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

/* ******************************************************************** **
** uses precompiled headers
** ******************************************************************** */

#include "stdafx.h"

#include "pattern_matcher.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* ******************************************************************** **
** @@ internal defines
** ******************************************************************** */

// Useless stuff
#define  MachineWord          (4)
#define  BitsPerWord          (32)

/*> Pattern internal representation

Statement / Atom Oct. Mnemonic
-------------------- ---- -----------------*/
#define SUCCESS 0000 /* S */
/*>>Substrings<<<*/
#define FIRST_ASCII_CHARACTER 0001 /* soh */
#define LAST_ASCII_CHARACTER 0177 /* del */
/*>>>Repeater<<<<*/
#define MAX_REPEATER_COUNT 077 /* small one */
#define MAX_FINITE_REPEATER 0277 /* 0..63 */
/*>>>>Atoms<<<<<<*/
#define END_OF_STRING 0300 /* . */
#define ANY 0301 /* % */
#define BLANK 0302 /* + */
#define DIGIT 0303 /* # */
#define UPPER_CASE_LETTER 0304 /* U */
#define LOWER_CASE_LETTER 0305 /* W */
#define LETTER 0306 /* L */
#define CHARACTER 0307 /* C */
#define FAILURE 0310 /* F */
#define END_OF_KEYWORD 0311 /* _ */
#define NOOP 0312 /* empty pattern */
#define NEW_LINE 0313 /* / */
#define CASE_DEAF_HOLERITH 0314 /* <literal> */
#define HOLERITH 0315 /* 'literal' */
#define ANY_OF 0316 /* {literal} */
#define MAX_HOLERITH 0400 /* max length */
/*>>Statements<<<*/
#define FIRST_STATEMENT_CODE 0363 /* */
#define ARB 0363 /* ... */
#define ASSIGN 0364 /* = */
#define GO_TO 0365 /* goto label */
#define FINITE_REPEATER 0366 /* 0..2**32 */
#define EXTERNAL_PATTERN 0367 /* call pattern */
#define QUERY 0370 /* no empty */
#define DO_NOT_RETURN 0371 /* : */
#define INVERSE 0372 /* not */
#define LITTLE_REPEATER 0373 /* * */
#define BIG_REPEATER 0374 /* $ */
#define RIGHT_BRACE 0375 /* ) */
#define OR 0376 /* | */
#define LEFT_BRACE 0377 /* ( */
/*>

In most cases the statement or atom consists of exactly one BYTE.
But there are several exceptions when a defined sequence of bytes
follows the significant BYTE (one of described above). Here is the
list of the exceptions:

(o) The CASE_DEAF_HOLERITH or the HOLERITH bytes must be followed
by the BYTE counter (one BYTE) and the literal body with the
length specified by that counter.

(o) The ANY_OF BYTE is followed by the pair of bytes. The first of
them gives 5 high order bits of the lowest code of characters
represented in the `any of' set. The second one gives size of
the bit-map table that follows it. The table contains a bit set
for each character from the `any of'.

(o) The ASSIGN BYTE is followed by the four bytes containing the
variable identifier number. The number format is machine depen-
dent.

(o) The GO_TO BYTE is followed by the four bytes containing signed
relative offset to the target pattern. The offset is counted in
the assumption that the next BYTE has offset 0. It has portable
format. The low significant bit of the first BYTE is the offset
sign. Other 7 bits are low significant bits of the offset. The
next BYTE contains the next 8 bits of the offset and so on.

(o) The EXTERNAL_PATTERN BYTE is followed by several bytes contain-
ing the address of the target pattern. The number of bytes and
the address format are machine dependent.

(o) The FINITE_REPEATER BYTE is followed by the four bytes contain-
ing the repetition count. The count format is the same as the
offset format of GO_TO statement (see). It is portable.

<*/

/* ******************************************************************** **
** @@ internal prototypes
** ******************************************************************** */

/* ******************************************************************** **
** @@ external global variables
** ******************************************************************** */

/* ******************************************************************** **
** @@ static global variables
** ******************************************************************** */

static E_STATUS_CODE    _MatchError = eST_FAILURE;
                  
/* ******************************************************************** **
** @@ real code
** ******************************************************************** */

/* ******************************************************************** **
** @@ IsPrintable()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update : 
** @  Notes  : 
** ******************************************************************** */

static boolean IsPrintable(unsigned char byChr)
{
   if (byChr < 0x20)
   {
      return FALSE;
   }
   
   if (byChr == 0x7F)
   {
      return FALSE;
   }
   
   return TRUE;
}

/* ******************************************************************** **
** @@ PatternMatcher::PatternMatcher()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  : Constructor
** ******************************************************************** */

PatternMatcher::PatternMatcher()
{
   _pszUserAlphas = NULL;  // User defined alphabeticals

   _iUserBase = 10;      // Digits are 0..9

//   _MatchError = eST_FAILURE;
}

/* ******************************************************************** **
** @@ PatternMatcher::~PatternMatcher()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  : Destructor
** ******************************************************************** */

PatternMatcher::~PatternMatcher()
{  
   Free();
}

/* ******************************************************************** **
** @@ 
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

/*
Symbol DEBUG defines debugging level i.e. how much verbose must be
the program. A working version should be compiled without DEBUG defined.

#define DEBUG 9

Symbol PTR_SIZE forces exactly specified number bytes to be used for
internal data pointers representation. By default the number of used
bytes is either 4 (for 16/32-bit machines) or 8 (for more than 32-bit
machines). You can define it as 2 for a 16-bit processor without
memory control unit.

#define PTR_SIZE 2
*/

#ifdef _DEBUG
#define DEBUG                 (9)
#endif

// Definition of useful types
#define PTR_SIZE              (4)

#define WORD_SIZE             (4)

#define APOSTROPHE            '\''
#define CIRCUMFLEX            '^'
#define EQUATION              '='
#define LEFT_ANGLE            '<'
#define RABBIT_EARS           '"'
#define RIGHT_ANGLE           '>'
#define SPACE                 ' '
#define TAB                   '\t'
#define LF                    '\n'

#define NUMBER_OF_ERRORS            (15)
#define NUMBER_OF_EXT_PATTERNS      (16)
#define NUMBER_OF_PATTERNS          (9)
#define NUMBER_OF_DISPLAYS          (1)
#define MAX_PATTERN_LENGTH          (75 * 27 + 3)
#define NUMBER_OF_VARIABLES         (8)

// Stack control parameters
#define MAX_STACK_FRAME       (4096)

#define STACK_FRAME_SIZE MAX_STACK_FRAME + PTR_SIZE * 2

// Some masks for managing statement header tag
#define HEADER_TEST_MASK      ( 0x81)
#define HEADER_SLICE_MASK     (~0x81)

// Other constants 
#define MAX_INT_DIV_10        214748364
#define LAST_MSS_MASK         128
#define KEY_LIST_SIZE         38
#define MAX_STRING_LENGTH     0x4FFFFFFF

/*^v^v^v^v^v^v^v^v^v^v^v^v^v^v^ MACROS ^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^*/

/*
PopPtr -- Pop pointer in reverse order
PushPtr -- Push pointer in reverse order

BPtr - Pointer to BYTE following the last BYTE

This macro copies subsequent bytes in reverse order. After completion
pointer will point to the first BYTE. The copy is stored in the
Take.Byte array.

*/
#if PTR_SIZE == 2
#define PopPtr(BPtr) \
{ \
Take.Byte [1] = *--BPtr; \
Take.Byte [0] = *--BPtr; \
}
#define PushPtr(BPtr, AVal) \
{ \
Take.Ptr = AVal; \
*--BPtr = Take.Byte [1]; \
*--BPtr = Take.Byte [0]; \
}
#else
#if PTR_SIZE == 4
#define PopPtr(BPtr) \
{ \
Take.Byte [3] = *--BPtr; \
Take.Byte [2] = *--BPtr; \
Take.Byte [1] = *--BPtr; \
Take.Byte [0] = *--BPtr; \
}
#define PushPtr(BPtr, AVal) \
{ \
Take.Ptr = AVal; \
*--BPtr = Take.Byte [3]; \
*--BPtr = Take.Byte [2]; \
*--BPtr = Take.Byte [1]; \
*--BPtr = Take.Byte [0]; \
}
#else
#define PopPtr(BPtr) \
{ \
Take.Byte [7] = *--BPtr; \
Take.Byte [6] = *--BPtr; \
Take.Byte [5] = *--BPtr; \
Take.Byte [4] = *--BPtr; \
Take.Byte [3] = *--BPtr; \
Take.Byte [2] = *--BPtr; \
Take.Byte [1] = *--BPtr; \
Take.Byte [0] = *--BPtr; \
}
#define PushPtr(BPtr, AVal) \
{ \
Take.Ptr = AVal; \
*--BPtr = Take.Byte [7]; \
*--BPtr = Take.Byte [6]; \
*--BPtr = Take.Byte [5]; \
*--BPtr = Take.Byte [4]; \
*--BPtr = Take.Byte [3]; \
*--BPtr = Take.Byte [2]; \
*--BPtr = Take.Byte [1]; \
*--BPtr = Take.Byte [0]; \
}
#endif
#endif
/*

PickUpWord -- Assembly a DWORD from four subsequential bytes

BPtr - Pointer to the fisrt BYTE

This macro takes four subsequential bytes and stores them into
Take.Byte variable. Pointer is advanced to the BYTE following the
last BYTE.

*/
#define PickUpWord(BPtr) \
( \
Take.Byte [0] = *BPtr++, \
Take.Byte [1] = *BPtr++, \
Take.Byte [2] = *BPtr++, \
Take.Byte [3] = *BPtr++ \
)
/*

PushWord -- Put four bytes

BPtr - Pointer to the fisrt destination BYTE

This macro stores Take.Byte into four subsequent bytes. The pointer
is advanced to the BYTE following the last destination BYTE.

*/
#define PushWord(BPtr) \
{ \
*BPtr++ = Take.Byte [0]; \
*BPtr++ = Take.Byte [1]; \
*BPtr++ = Take.Byte [2]; \
*BPtr++ = Take.Byte [3]; \
}
/*

PutWord -- Store a DWORD into four subsequent bytes
PutPtr -- Store a pointer into subsequent bytes

BPtr - Destination pointer
WVal - Word/Ptr

This macro stores WVal into subsequent bytes. The pointer is advanced
to the next free BYTE.

*/
#define PutWord(BPtr,WVal) \
{ \
Take.Word = WVal; \
PushWord (BPtr); \
}
#if PTR_SIZE == 2
#define PutPtr(BPtr,WVal) \
{ \
Take.Ptr = WVal; \
*BPtr++ = Take.Byte [0]; \
*BPtr++ = Take.Byte [1]; \
}
#else
#if PTR_SIZE == 4
#define PutPtr(BPtr,WVal) \
{ \
Take.Ptr = WVal; \
*BPtr++ = Take.Byte [0]; \
*BPtr++ = Take.Byte [1]; \
*BPtr++ = Take.Byte [2]; \
*BPtr++ = Take.Byte [3]; \
}
#else
#define PutPtr(BPtr,WVal) \
{ \
Take.Ptr = WVal; \
*BPtr++ = Take.Byte [0]; \
*BPtr++ = Take.Byte [1]; \
*BPtr++ = Take.Byte [2]; \
*BPtr++ = Take.Byte [3]; \
*BPtr++ = Take.Byte [4]; \
*BPtr++ = Take.Byte [5]; \
*BPtr++ = Take.Byte [6]; \
*BPtr++ = Take.Byte [7]; \
}
#endif
#endif
/*

GetWord -- Get a DWORD from four subsequent bytes
GetPtr -- Get a pointer from subsequent bytes

BPtr - Byte pointer

This macro assemblies DWORD/pointer from subsequent bytes. The pointer
is advanced to the next free BYTE.

*/
#define GetWord(BPtr) \
( \
PickUpWord (BPtr), \
Take.Word \
)
#if PTR_SIZE == 2
#define GetPtr(BPtr) \
( \
Take.Byte [0] = *BPtr++, \
Take.Byte [1] = *BPtr++, \
Take.Ptr \
)
#else
#if PTR_SIZE == 4
#define GetPtr(BPtr) \
( \
Take.Byte [0] = *BPtr++, \
Take.Byte [1] = *BPtr++, \
Take.Byte [2] = *BPtr++, \
Take.Byte [3] = *BPtr++, \
Take.Ptr \
)
#else
#define GetPtr(BPtr) \
( \
Take.Byte [0] = *BPtr++, \
Take.Byte [1] = *BPtr++, \
Take.Byte [2] = *BPtr++, \
Take.Byte [3] = *BPtr++, \
Take.Byte [4] = *BPtr++, \
Take.Byte [5] = *BPtr++, \
Take.Byte [6] = *BPtr++, \
Take.Byte [7] = *BPtr++, \
Take.Ptr \
)
#endif
#endif
/*

GetInt -- Assembling a signed integer from four subsequent bytes
PutInt -- Storing a signed integer into four subsequent bytes

Pointer - To the first BYTE

The Pointer is advanced so that after GetInt/PutInt it will point to
four bytes far from initial position. The value is stored/read from
Take.Int variable. The integer has portable format:

Byte Content
1 Sign (low significant bit) and bits 1..7
2 Bits 8..15
3 Bits 16..23
4 Bits 24..31

*/
#define GetInt(BPtr) \
{ \
Take.Int = 0x0000007FL & ((long) *BPtr++ >> 1); \
Take.Int += 0x00007F80L & ((long) *BPtr++ << 7); \
Take.Int += 0x007F8000L & ((long) *BPtr++ << 15); \
Take.Int += 0x7F800000L & ((long) *BPtr++ << 23); \
if (BPtr [-4] & 0x01) Take.Int = -Take.Int; \
}
#define PutInt(BPtr) \
{ \
if (0 > Take.Int) \
{ \
Take.Int = -Take.Int; \
*BPtr++ = \
(BYTE) (((Take.Int & 0x7F) << 1) | 0x01); \
} \
else \
{ \
*BPtr++ = (BYTE) ((Take.Int & 0x7F) << 1); \
} \
*BPtr++ = (BYTE) ((Take.Int >> 7 ) & 0xFF); \
*BPtr++ = (BYTE) ((Take.Int >> 15) & 0xFF); \
*BPtr++ = (BYTE) ((Take.Int >> 23) & 0xFF); \
}
/*
Statement header format.
------------------------
This format is used when a statement becomes active or matched.
The purpose is to save some data that are necessary for the
statement processing. Any header contains a tag that defines its
type. This tag occupies one BYTE. Therefore, some kludges are
used to avoid alignment of the C compiler. Noone can say that it
is a good style of coding. It should be also mentioned that some
compilers always align data to the DWORD boundary. Moreover some
wonderful compilers have bugs in the sizeof function for a
struct items. Function SizeOf is used to avoid these bugs.

*/
#define Hdr Header_Tag
#define Tag Header_Tag
#define SizeOf(Who) \
((BYTE) (&Who.Header_Tag - (BYTE *) &Who + 1))

typedef union
{
   #define RANGE 0
   struct /* Pseudo statement */
   {
      BYTE  Tag; /* Header tag */
   } Range;

   #define SELECT 1
   struct
   {
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } Select;

   #define NOT 2
   struct
   {
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } Not;

   #define NOEMPTY 3
   struct
   {
      DWORD  Stub; /* Saved string offset */
      BYTE  Tag; /* Header tag */
   } Noempty;

   #define REPEATER 4
   struct
   {
      BYTE* Pat_ptr; /* Pattern pointer */
      DWORD  Counter; /* Repeatition count */
      DWORD  Wanted; /* Wanted count */
      BYTE  Tag; /* Header tag */
   } Repeater;

   #define AS_MUCH 5
   struct
   {
      DWORD  Counter; /* Repeatition count */
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } As_much;

   #define AS_LITTLE 6
   struct
   {
      DWORD  Counter; /* Repeatition count */
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } As_little;

   #define CALL 7
   struct
   {
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } Call;

   #define ASSIGN_SUBSTR 8
   struct
   {
      DWORD  Id; /* User provided id */
      DWORD  Offset; /* String offset */
      BYTE  Tag; /* Header tag */
   } Assign;

   #define FENCED_ASSIGN 9
   #define SLEEPING_ASSIGN 10
   struct /* Sleeping form */
   {
      DWORD  Id; /* User provided id */
      BYTE  Tag; /* Header tag */
   } Sleeping_Assign;

   #define ELLIPSIS 11
   struct
   {
      BYTE* Pat_ptr; /* Pattern pointer */
      BYTE  Tag; /* Header tag */
   } Ellipsis;
   #define TOTAL_STATEMENTS 12

   /* This format is used for pattern translation */
   struct
   {
      BYTE*             Name; /* Name string pointer */
      DWORD              Destination_offset; /* Offset in pattern */
   } Label;
} HeaderUnion;

/*
Stack manipulation

PopStack - poppes some data up from the stack
FreeStack - just the same, but without data moving
PushStack - pushes some data onto the stack
GetHdr - returns top stack BYTE
SetStat - takes header tag from stack and sets ThisStat
StatTag - makes header tag of the statement (ThisStat)

In purpose of efficiency this code was made as macro. This has
some disadvantages... !!WARNING!!

1. Popping data must really be in the stack. No empty stack control is made.
2. Data may cross only one stack frame margin.
3. The following variables must be visible (i.e. declared) and scratch: WORD DataLen, BYTE * DataPtr, BYTE * DataFrame
4. Label NoMoreMemory must be visible too. You will be there on malloc's failure.
*/

#define PopStack(Stack_ptr, Stack_used, Target, Size) \
{ \
DataLen = (Size); \
DataPtr = (BYTE *) (Target) + DataLen; \
if (DataLen >= Stack_used) \
{ \
DataLen = (WORD) (DataLen - Stack_used); \
while (Stack_used--) \
*--DataPtr = *--Stack_ptr; \
PopPtr (Stack_ptr); \
Stack_ptr = Take.Ptr; \
Stack_used = MAX_STACK_FRAME; \
} \
Stack_used = (WORD) (Stack_used - DataLen); \
while (DataLen--) \
*--DataPtr = *--Stack_ptr; \
}

#define FreeStack(Stack_ptr, Stack_used, Size) \
{ \
DataLen = (Size); \
if (DataLen >= Stack_used) \
{ \
DataLen = (WORD) (DataLen - Stack_used); \
Stack_ptr -= Stack_used; \
PopPtr (Stack_ptr); \
Stack_ptr = Take.Ptr; \
Stack_used = MAX_STACK_FRAME; \
} \
Stack_used = (WORD) (Stack_used - DataLen); \
Stack_ptr -= DataLen; \
}

#define PushStack(Stack_ptr, Stack_used, Target, Size) \
{ \
DataLen = (Size); \
DataPtr = (BYTE *) (Target); \
if (DataLen > MAX_STACK_FRAME - Stack_used) \
{ \
Stack_used = \
(WORD) (MAX_STACK_FRAME - Stack_used); \
DataLen = (WORD) (DataLen - Stack_used); \
while (Stack_used--) \
*Stack_ptr++ = *DataPtr++; \
if (GetPtr (Stack_ptr) != 0) \
{ \
Stack_ptr = Take.Ptr; \
} \
else \
{ \
if ( NULL \
== ( DataFrame \
= (BYTE *) malloc (STACK_FRAME_SIZE) \
) ) goto NoMoreMemory; \
PushPtr (Stack_ptr, DataFrame + PTR_SIZE); \
PutPtr (DataFrame, Stack_ptr); \
Stack_ptr = DataFrame; \
DataFrame += MAX_STACK_FRAME; \
PutPtr (DataFrame, 0); \
} \
Stack_used = 0; \
} \
Stack_used = (WORD) (Stack_used + DataLen); \
while (DataLen--) \
*Stack_ptr++ = *DataPtr++; \
}

#define GetHdr(Pointer) (((BYTE *) Pointer) [-1])

#define SetStat(Pointer) \
(ThisStat = (HEADER_SLICE_MASK & GetHdr (Pointer)) >>1)

#define StatTag \
((BYTE) (HEADER_TEST_MASK | (ThisStat<<1)))

/* Common Stack processing

SaveStack - saves current stack state (reentrant call)
RestoreStack - restores stack state
StackData - defines data to save stack state
Done - an equivalent to ordinal return
ErrorDone - set MatchError and Done
*/

#define SaveStack \
{ \
First_ASS_ptr = ASS_ptr; \
First_MSS_ptr = MSS_ptr; \
First_ASS_used = ASS_used; \
First_MSS_used = MSS_used; \
}

#define RestoreStack \
{ \
ASS_ptr = Old_ASS_ptr; \
MSS_ptr = Old_MSS_ptr; \
ASS_used = Old_ASS_used; \
MSS_used = Old_MSS_used; \
First_ASS_ptr = Old_1st_ASS_ptr; \
First_MSS_ptr = Old_1st_MSS_ptr; \
First_ASS_used = Old_1st_ASS_used; \
First_MSS_used = Old_1st_MSS_used; \
}

#define StackData \
BYTE * Old_ASS_ptr = ASS_ptr; \
BYTE * Old_MSS_ptr = MSS_ptr; \
WORD Old_ASS_used = ASS_used; \
WORD Old_MSS_used = MSS_used; \
BYTE * Old_1st_ASS_ptr = First_ASS_ptr; \
BYTE * Old_1st_MSS_ptr = First_MSS_ptr; \
WORD Old_1st_ASS_used = First_ASS_used; \
WORD Old_1st_MSS_used = First_MSS_used;

#define Done(Expression) \
{ \
RestoreStack; \
return (Expression); \
}

#define ErrorDone(Expression) \
{ \
RestoreStack; \
return (_MatchError = (Expression)); \
}

/* Active Statement Stack processing

EmptyASS - returns true if ASS is empty
PopASS - takes the statement (ThisStat) header from ASS
PushASS - pushes the statement header onto ASS
PopLabel - pop a label descriptor from ASS (Patran only)
PushLabel - push a label descriptor onto ASS
*/

#define EmptyASS \
(ASS_ptr == First_ASS_ptr && ASS_used == First_ASS_used)

#define PopASS \
PopStack \
( ASS_ptr, ASS_used, \
(BYTE *) &Header, \
StatSize [ThisStat] \
)

#define PushASS \
PushStack \
( \
ASS_ptr, ASS_used, \
(BYTE *) &Header, \
StatSize [ThisStat] \
)

#define PopLabel \
PopStack \
( \
ASS_ptr, ASS_used, \
(BYTE *) &Header.Label, \
sizeof (Header.Label) \
)

#define PushLabel \
PushStack \
( \
ASS_ptr, ASS_used, \
(BYTE *) &Header.Label, \
sizeof (Header.Label) \
)

/* Matched Statement Stack processing

EmptyMSS - returns true if MSS is empty
PopMSS - takes the statement (ThisStat) header from MSS
PushMSS - pushes the statement header onto MSS
StubOnMSS - returns true if a stub lies on the top of MSS
KillStub - just removes a stub from the MSS
CheckString - order Str_ptr if Length and Pointer are incorrect
SaveStr - saves string as a stub on the MSS
RestoreStr - restores string pointer from the stub
MakeStub - makes a stub value
*/
#define EmptyMSS \
(MSS_ptr == First_MSS_ptr && MSS_used == First_MSS_used)

#define PopMSS \
PopStack \
( \
MSS_ptr, MSS_used, \
(BYTE *) &Header, \
StatSize [ThisStat] \
)

#define PushMSS \
PushStack \
( \
MSS_ptr, MSS_used, \
(BYTE *) &Header, \
StatSize [ThisStat] \
)

#define StubOnMSS \
(0 == (GetHdr (MSS_ptr) & 0x01))

#define KillStub FreeStack (MSS_ptr, MSS_used, WORD_SIZE)

#define MakeStub \
(((DWORD) (Str_ptr - Str_Beg) + Str_offset) << 1)

#define CheckString \
{ \
   if (iSize < 0) Str_End = Str_Beg; \
   if (*piPointer < 0) \
   Str_ptr = Str_Beg; \
   else \
   if (*piPointer > iSize) \
   Str_ptr = Str_End; \
}

#define SaveStr \
{ \
Temp = MakeStub; \
DataLen = WORD_SIZE; \
if (DataLen > MAX_STACK_FRAME - MSS_used) \
{ \
MSS_used = \
(WORD) (MAX_STACK_FRAME - MSS_used); \
DataLen = (WORD) (DataLen - MSS_used); \
switch (MSS_used) \
{ \
case 4 : MSS_ptr [3] =(BYTE)(0xFF &(Temp ));\
case 3 : MSS_ptr [2] =(BYTE)(0xFF &(Temp>> 8));\
case 2 : MSS_ptr [1] =(BYTE)(0xFF &(Temp>>16));\
case 1 : MSS_ptr [0] =(BYTE)(0xFF &(Temp>>24));\
} \
MSS_ptr += MSS_used; \
if (GetPtr (MSS_ptr) != 0) \
{ \
MSS_ptr = Take.Ptr; \
} \
else \
{ \
if ( NULL \
== ( DataFrame \
= (BYTE *) malloc (STACK_FRAME_SIZE) \
) ) goto NoMoreMemory; \
PushPtr (MSS_ptr, DataFrame + PTR_SIZE); \
PutPtr (DataFrame, MSS_ptr); \
MSS_ptr = DataFrame; \
DataFrame += MAX_STACK_FRAME; \
PutPtr (DataFrame, 0); \
} \
MSS_used = 0; \
} \
MSS_used = (WORD) (MSS_used + DataLen); \
MSS_ptr += DataLen; \
switch (DataLen) \
{ \
case 4 : MSS_ptr [-4] = (BYTE)(0xFF &(Temp>>24)); \
case 3 : MSS_ptr [-3] = (BYTE)(0xFF &(Temp>>16)); \
case 2 : MSS_ptr [-2] = (BYTE)(0xFF &(Temp>> 8)); \
case 1 : MSS_ptr [-1] = (BYTE)(0xFF &(Temp )); \
} \
}
#define RestoreStr \
{ \
Temp = 0; \
DataLen = WORD_SIZE; \
if (DataLen >= MSS_used) \
{ \
DataLen = (WORD) (DataLen - MSS_used); \
switch (MSS_used) \
{ \
case 4 : Temp += (DWORD) MSS_ptr [-4] << 24; \
case 3 : Temp += (DWORD) MSS_ptr [-3] << 16; \
case 2 : Temp += (DWORD) MSS_ptr [-2] << 8; \
case 1 : Temp += (DWORD) MSS_ptr [-1]; \
} \
MSS_ptr -= MSS_used; \
PopPtr (MSS_ptr); \
MSS_ptr = Take.Ptr; \
MSS_used = MAX_STACK_FRAME; \
} \
MSS_used = (WORD) (MSS_used - DataLen); \
MSS_ptr -= DataLen; \
switch (DataLen) \
{ \
case 4 : Temp += (DWORD) MSS_ptr [3]; \
case 3 : Temp += (DWORD) MSS_ptr [2] << 8; \
case 2 : Temp += (DWORD) MSS_ptr [1] << 16; \
case 1 : Temp += (DWORD) MSS_ptr [0] << 24; \
} \
if (Str_offset > (Temp >>= 1)) \
{ \
int Length = 0; \
\
while (Temp < Str_offset) \
{ \
if ( GetPreviousLine == NULL \
|| ! (* GetPreviousLine) \
( (const char **) &Str_Beg, &Length \
) ) goto CantReturn; \
Str_offset -= Length + 1; \
} \
Str_End = Str_Beg + Length; \
} \
Str_ptr = Str_Beg + (DWORD) (Temp - Str_offset); \
}

/* Source string processing

DoCircumflex - BYTE conversion by a circumflex
EndOfString - returns true if end of the string seen
Error - error logging (fatal)
IsUserAlpha - is character is user defined alphabeticals
KeyWord - returns true if specified keyword is matched
PassSpace - skip spaces and tabs
PassId - skip identifier

*/
#define DoCircumflex(Code) \
((Code) ^= 0100)

#define EndOfString (Str_ptr >= Str_End)

#define IsUserAlpha(Char) \
(isalpha(Code = (Char)) \
|| (_pszUserAlphas != NULL) \
&& (strchr(_pszUserAlphas,(Code)) != NULL) \
)

#define KeyWord(Text) (Lex_ptr != (Str_ptr = MatchKeyWord(Lex_ptr,Str_End,(Text))))

#define PassSpace \
{ \
while ( !EndOfString \
&& ( (Code = *Str_ptr) == SPACE \
|| Code == TAB \
) ) Str_ptr++; \
}

#define PassId \
{ \
while ( !EndOfString \
&& ( IsUserAlpha (*Str_ptr) \
|| isdigit (Code) \
) ) Str_ptr++; \
Lex_End = Str_ptr; \
PassSpace; \
}

#define Error(Code, Pointer) \
{ \
*Error_code = Code; \
*Error_ptr = Pointer; \
return (0); \
}

/* Brace processing

PopBrace - pop a brace (resulted in 0 - (), 1 - [] or 2)

!!WARNING!! Variable Zero must contain 0 and be visible for
PushXB.

*/
#define PopBrace(Result) \
{ \
SAY (4, "\n ^ MSS_mask = %X (Hex)", MSS_mask); \
if (!MSS_mask) \
{ \
Result = 2; \
} \
else \
{ \
Result = \
(BYTE) (0 != (*(MSS_ptr - 1) & MSS_mask)); \
if ((MSS_mask >>= 1) == 0) \
{ \
FreeStack (MSS_ptr, MSS_used, 1); \
if (!EmptyMSS) MSS_mask = LAST_MSS_MASK; \
} } }

#ifdef DEBUG
#define SAY(level, format, data) \
if (DEBUG > level) fprintf (stderr, (format), (data))
#else
#define SAY(level, format, data)
#endif

/* Statement codes to detect indefinite loops (see Second_pass) */

#define EMPTY_ATOM ( 0 ) /* matches null string */
#define NON_EMPTY_ATOM ( 1 ) /* does not match null */
#define NON_SELECT_MASK ( 2 ) /* non-select if set */
#define RETURN_EMPTY ( 2+( 0<<2 )) /* stat. can match null */
#define RETURN_NON_EMPTY ( 2+( 1<<2 )) /* stat. can't do it */
#define SON_WILL_SAY ( 2+( 2<<2 )) /* depending on subpat */
#define INDEFINITE ( 2+( 3<<2 )) /* $ or * repeater */
#define SELECT_SHIFT ( 4 ) /* shift to empty flag */
#define SELECT_MASK (~( 1<<4 )-1) /* mask for empty flag */
#define SELECT_BRANCH ( 3<<4 ) /* select statement */
#define ERROR_STATUS ( 2<<5 ) /* error flag */

/*^v^v^v^v^v^v^v^ STATIC AND COMMON VARIABLES ^v^v^v^v^v^v^v^v^v^v^v^v^*/

static BYTE PowerOf2[8] =
{
   1, 2, 4, 8, 16, 32, 64, 128
};

static BYTE   Preallocated_ASS_frame[STACK_FRAME_SIZE];
static BYTE   Preallocated_MSS_frame[STACK_FRAME_SIZE];

static BYTE*  ASS_ptr        = &Preallocated_ASS_frame[PTR_SIZE];
static BYTE*  First_ASS_ptr  = &Preallocated_ASS_frame[PTR_SIZE];
static BYTE*  First_MSS_ptr  = &Preallocated_MSS_frame[PTR_SIZE];
static BYTE*  MSS_ptr        = &Preallocated_MSS_frame[PTR_SIZE];
static WORD   First_ASS_used = 1; // to prevent deallocation of
static WORD   First_MSS_used = 1; // the preallocated frames
static WORD   ASS_used       = 1; // the ASS frame used bytes
static WORD   MSS_used       = 1; // the MSS frame used bytes
static WORD   MSS_mask; // the bit mask of MSS top BYTE

static BYTE*  DataPtr   = NULL;
static BYTE*  DataFrame = NULL;
static WORD   DataLen   = NULL;

#define Shared

int   (*GetNextLine)       (const char** Line,int* Length) = 0;
int   (*GetPreviousLine)   (const char** Line,int* Length) = 0;
char* (*AllocatePattern)   (DWORD Size) = 0;
char* (*GetExternalPattern)(char* Name,int Length) = 0;
DWORD (*GetVariableId)     (char* Name,int Length) = 0;
void  (*AssignVariable)    (DWORD VariableId,DWORD Offset,DWORD Length) = 0;
void  (*DeAssignVariable)  (DWORD VariableId) = 0;

struct KEY_LIST
{
   BYTE        Code;
   char*       Name;
};

static KEY_LIST KeyList[KEY_LIST_SIZE] =
{
   { ANY, "ANY" }, 
   { ANY, "%" }, 
   { ARB, "ARB" }, 
   { ARB, "..." }, 
   { ARB, ".." }, 
   { BIG_REPEATER, "$" }, 
   { BLANK, "BLANK" }, 
   { BLANK, "+" }, 
   { CHARACTER, "CHARACTER" }, 
   { CHARACTER, "C" }, 
   { DIGIT, "DIGIT" }, 
   { DIGIT, "#" }, 
   { DO_NOT_RETURN, "FENCE" },
   { DO_NOT_RETURN, ":" }, 
   { END_OF_KEYWORD, "BREAK" }, 
   { END_OF_KEYWORD, "_" }, 
   { END_OF_STRING, "END" }, 
   { END_OF_STRING, "." }, 
   { FAILURE, "FAILURE" }, 
   { FAILURE, "F" }, 
   { INVERSE, "NOT" }, 
   { INVERSE, "^" }, 
   { LETTER, "LETTER" }, 
   { LOWER_CASE_LETTER, "LCL" }, 
   { LETTER, "L" }, 
   { LITTLE_REPEATER, "*" }, 
   { LOWER_CASE_LETTER, "W" }, 
   { NEW_LINE, "NL" }, 
   { NEW_LINE, "/" }, 
   { OR, "OR" }, 
   { OR, "!" }, 
   { OR, "|" }, 
   { QUERY, "NOEMPTY" }, 
   { QUERY, "?" }, 
   { SUCCESS, "SUCCESS" }, 
   { SUCCESS, "S" }, 
   { UPPER_CASE_LETTER, "UCL" }, 
   { UPPER_CASE_LETTER, "U" }, 
}; 

static BYTE StatSize[TOTAL_STATEMENTS]; /* Sizes of headers */

static union
{
   long        Int;
   DWORD        Word;
   BYTE*       Ptr;
   BYTE        Byte[PTR_SIZE];
} Take;

HeaderUnion Header;

/* Macros ...

PushStatement - push a statement code
PopStatement - pop it
FindForSelect - pop until select and test empty branch existing
CloseBranch - accumulate branch status
*/

#define PushStatement \
PushStack (MSS_ptr, MSS_used, &EmptyStatus, 1)

#define PopStatement \
PopStack (MSS_ptr, MSS_used, &EmptyStatus, 1);

#define FindForSelect(Status) \
if ( ( EmptyStatus = CloseStatement (Status) \
) == ERROR_STATUS \
) goto IndefiniteLoop;

#define CloseBranch(Status) \
FindForSelect(Status); \
*(MSS_ptr-1) &= ( ( *(MSS_ptr-1) << SELECT_SHIFT ) \
| SELECT_MASK \
);

/*^v^v^v^v^v^v^v^v^v^v^v^ PSEUDO STATEMENTS ^v^v^v^v^v^v^v^v^v^v^v^v^v^*/

#define SkipOneItem_and_Success \
{ ReturnTo = 0 ; goto SkipOneItem; }

#define SkipOneItem_and_IsAlternative \
{ ReturnTo = 1 ; goto SkipOneItem; }

#define SkipOneItem_and_EndOfSelect \
{ ReturnTo = 2 ; goto SkipOneItem; }

#define SkipOneItem_and_DoLittleRepeater \
ReturnTo = 3 ; goto SkipOneItem; DoLittleRepeater :

#define CleanMSS_and_Success \
ReturnTo = 0 ; goto CleanMSS;

#define CleanMSS_and_Failure \
ReturnTo = 1 | 0x10 | 0x20; goto CleanMSS;

#define CleanMSS_and_Go \
ReturnTo = 2 | 0x10; goto CleanMSS;

#define CleanMSS_and_ContinueLittleRepeater \
ReturnTo = 3 | 0x10; goto CleanMSS; \
ContinueLittleRepeater :

/*^v^v^v^v^v^v^v^v^v^v^v^v^v^v^ FUNCTIONS ^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v

External functions

freedm -- releases dynamic memory occupied by working stacks
match -- matches string with a pattern
Translate -- pattern translator
patmaker -- pattern constructor


freedm -- releases dynamic memory occupied by ASS and MSS

After execution ASS and MSS pointers will be restored to previous
values (First_ASS_ptr, First_ASS_used, ...). All stack frames allo-
cated later will be released.

Will be released >>>>>>>>>>>>>>>>>>>>>>>
._________. ._________. ._________.
| | | | | |
--->| frame |----->| frame |----->| frame |----> nil
|_________| |_________| |_________|
| |
First_xSS_ptr __| xSS_ptr __| x = A or M

*/

/* ******************************************************************** **
** @@ PatternMatcher::Free()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  : 
** ******************************************************************** */

void PatternMatcher::Free()
{  
   BYTE*    ThisBlock = NULL;
   BYTE*    NextBlock = NULL;

   int      StackNo = 0;

   for (StackNo = 0; StackNo < 2; StackNo++)
   {
      ThisBlock = ((StackNo ? First_ASS_ptr - First_ASS_used : First_MSS_ptr - First_MSS_used) + MAX_STACK_FRAME);
      NextBlock = GetPtr(ThisBlock);
   
      PushPtr(ThisBlock,0);

      while (NextBlock)
      {
         ThisBlock  = NextBlock + MAX_STACK_FRAME;
         NextBlock  = GetPtr(ThisBlock);
         ThisBlock -= STACK_FRAME_SIZE;

         free(ThisBlock);
      }
   }

   ASS_ptr  = First_ASS_ptr;
   MSS_ptr  = First_MSS_ptr;
   ASS_used = First_ASS_used;
   MSS_used = First_MSS_used;
}

/*
match -- match a pattern

Length - of a character string to be matched
String - points to the string (maybe not NUL terminated)
Pointer - number of 1st character to be matched (0..Length)
Pattern - points to a pattern (in internal format)

Returns :
eST_SUCCESS - Success
eST_FAILURE - Failure of matching
eST_WRONG_PATTERN_FORMAT - Wrong pattern format
eST_TOO_LONG_STRING - String length exceeds 1Gbyte
eST_CANNOT_RETURN - Cannot return to previously left string
eST_NO_DYNAMIC_MEMORY - Dynamic memory overflow

After successful completion String [*Pointer] will be the 1st charac-
ter following matched substring. Note that after successful matching
the new line atom (/) Pointer will be still counted as an offset from
beginning of the String. The offset includes line ends: +1 for each
end.
*/

/* ******************************************************************** **
** @@ PatternMatcher::Match()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  : 
** ******************************************************************** */

int PatternMatcher::Match
(
   const char* const       pszText,
   int                     iSize,
   int*                    piPointer,
   BYTE*                   pPattern
)
{           
//   ASSERT(iSize > 0);
//   ASSERT(pszText);
   ASSERT(pPattern);

   BYTE*    Str_ptr = (BYTE*)&pszText[*piPointer];
   BYTE*    Pat_ptr = pPattern;

   BYTE     Code = 0;

   BYTE*    Str_Beg = (BYTE*)pszText;
   BYTE*    Str_End = (BYTE*)&pszText[iSize];

   BYTE     ReturnTo = 0;

   DWORD    Str_offset = 0;
   DWORD    Temp       = 0;
   
   int      ThisStat = 0;

   char  cPrintChar = 0;

   HeaderUnion Header;                                  

   StackData;

   ASSERT(SizeOf(Header.Select) == sizeof(Header.Select.Pat_ptr) + 1);

   // Something to initialize
   if (!StatSize[RANGE])
   {
      StatSize[RANGE]           = SizeOf(Header.Range);
      StatSize[SELECT]          = SizeOf(Header.Select);
      StatSize[NOT]             = SizeOf(Header.Not);
      StatSize[NOEMPTY]         = SizeOf(Header.Noempty);
      StatSize[REPEATER]        = SizeOf(Header.Repeater);
      StatSize[AS_MUCH]         = SizeOf(Header.As_much);
      StatSize[AS_LITTLE]       = SizeOf(Header.As_little);
      StatSize[CALL]            = SizeOf(Header.Call);
      StatSize[ASSIGN_SUBSTR]   = SizeOf(Header.Assign);
      StatSize[FENCED_ASSIGN]   = SizeOf(Header.Sleeping_Assign);
      StatSize[SLEEPING_ASSIGN] = SizeOf(Header.Sleeping_Assign);
      StatSize[ELLIPSIS]        = SizeOf(Header.Ellipsis);
   }

   ASSERT(SizeOf(Header.Select) == sizeof(Header.Select.Pat_ptr) + 1);
   ASSERT(MAX_STACK_FRAME > StatSize[RANGE]);
   ASSERT(MAX_STACK_FRAME > StatSize[SELECT]);
   ASSERT(MAX_STACK_FRAME > StatSize[NOT]);
   ASSERT(MAX_STACK_FRAME > StatSize[NOEMPTY]);
   ASSERT(MAX_STACK_FRAME > StatSize[REPEATER]);
   ASSERT(MAX_STACK_FRAME > StatSize[AS_MUCH]);
   ASSERT(MAX_STACK_FRAME > StatSize[AS_LITTLE]);
   ASSERT(MAX_STACK_FRAME > StatSize[CALL]);
   ASSERT(MAX_STACK_FRAME > StatSize[ASSIGN_SUBSTR]);
   ASSERT(MAX_STACK_FRAME > StatSize[FENCED_ASSIGN]);
   ASSERT(MAX_STACK_FRAME > StatSize[SLEEPING_ASSIGN]);
   ASSERT(MAX_STACK_FRAME > StatSize[ELLIPSIS]);
   ASSERT(MAX_STACK_FRAME > WORD_SIZE);

   CheckString;      
   SaveStack;

   ThisStat = RANGE;
   
   Header.Range.Hdr = StatTag;      
   
   PushASS;        
   PushMSS;
   
   ThisStat = SELECT;
   
   Header.Select.Hdr     = StatTag;
   Header.Select.Pat_ptr = Pat_ptr;

   SAY (4,"\nMatch Machine word of %d bytes",MachineWord);

   Activate: /* Here a statement indicated by the ThisStat be-
              comes active. Its header is pushed to the ASS
              and current string pointer is saved in the MSS.
              */          
   PushASS;

Go: 
   // Save string and take the next item of the pattern
   SaveStr;

Process:

   Code = *Pat_ptr++;

//   SAY(4,"\nMatch PROCESS %o (Oct)",Code);
   cPrintChar = IsPrintable(Code)  ?  Code  :  '.';

   SAY(4,"\nMatch      PROCESS %02X (Hex)",Code);
   SAY(4,"  : [%c]",cPrintChar);

   if (Code <= LAST_ASCII_CHARACTER)
   {
      // Match a 7-bit ASCII text constant
      SAY(2,"\nMatch Do literal [%02X] (Hex): ",Code);

      while (Code != SUCCESS)
      {
         if (EndOfString || Code != *Str_ptr++)
         {
            goto Failure;
         }

         Code = *Pat_ptr++;
         
         SAY(2,"%c",Code);
         
         if (Code > LAST_ASCII_CHARACTER)
         {
            --Pat_ptr;
            goto Success;
         }
      }

      SAY(2,"\nMatch GENERAL SUCCESS [%02X] (Hex)",Code);

      goto GeneralSuccess; // Atom 'success' seen 
   }

   if ((DWORD)Code <= MAX_FINITE_REPEATER)
   {
      Header.Repeater.Wanted = Code - LAST_ASCII_CHARACTER - 1;

MakeRepeater:

      SAY(2,"\nMatch Do FINITE REPEATER for %ld times",Header.Repeater.Wanted);

      if (Header.Repeater.Wanted)
      {
         ThisStat = REPEATER;
         Header.Repeater.Counter = 0;
         Header.Repeater.Pat_ptr = Pat_ptr;
         Header.Repeater.Hdr = StatTag;                  
         PushASS;

         goto Process;
      }
      else
      {
         SkipOneItem_and_Success;
      }
   }

   if ((DWORD) Code < FIRST_STATEMENT_CODE)
   {
      SAY(2,"\nMatch Do atom [%02X] (Hex) ",Code);

      switch (Code)
      {
         case END_OF_STRING:
         {
            SAY(2," - End of string [%02X] (Hex)",Code);

            if (EndOfString)
            {
               goto Success;
            }

            goto Failure;
         }
         case END_OF_KEYWORD:
         {
            SAY(2," - End of keyword [%02X] (Hex)",Code);

            if (EndOfString)
            {
               goto Success;
            }

            Code = *Str_ptr++;

            if (Code == SPACE || Code == TAB)
            {
               PassSpace;
               goto Success;
            }

            if (--Str_ptr == Str_Beg)
            {
               goto Success;
            }

            if (isalnum(Code) && isalnum(*(Str_ptr - 1)))
            {
               goto Failure;
            }
         }
         case NOOP:
         {
            goto Success;
         }
         case NEW_LINE :
         {
            char* NewLine;
            int   Length;

            SAY(2," - New Line [%02X] (Hex)",Code);

            if (GetNextLine == NULL || !(*GetNextLine) ((const char * *) &NewLine,& Length))
            {
               goto Failure;
            }

            Temp = Str_End - Str_Beg + 1;

            if (Str_offset > MAX_STRING_LENGTH - Temp)
            {
               ErrorDone(eST_TOO_LARGE_STRING);
            }

            Str_offset += Temp;
            Str_Beg = (BYTE *) NewLine;
            Str_End = Str_Beg + Length;
            Str_ptr = Str_Beg;

            goto Success;
         }
         case FAILURE:
         {
            ErrorDone(eST_FAILURE);
         }
      }

      SAY(2," - [%02X] (Hex) ",Code);

      if (EndOfString)
      {
         goto Failure;
      }
      
      switch (Code)
      {
         case ANY:
         {
            SAY(2,"ANY [%02X] (Hex)",Code);
            Str_ptr++;
            goto Success;
         }
         case BLANK:
         {
            SAY(2,"BLANK [%02X] (Hex)",Code);
            Code = *Str_ptr++;

            if (Code != SPACE && Code != TAB)
            {
               goto Failure;
            }                            

            PassSpace;
            
            goto Success;
         }
         case DIGIT:
         {
            SAY(2,"DIGIT [%02X] (Hex)",Code);

            Code = *Str_ptr++;
            
            if (_iUserBase <= 10)
            {
               // Bases 1..10 
               if (Code >= '0' && Code < '0' + _iUserBase)
               {
                  goto Success;
               }
            }
            else
            {
               // Bases 11.. 
               if ((Code >= '0' && Code <= '9') || ((Code = (BYTE)tolower(Code)) >= 'a' && Code <= 'a' + _iUserBase - 11))
               {
                  goto Success;
               }
            }
            
            goto Failure;
         }
         case UPPER_CASE_LETTER:
         {
            SAY(2,"UPPER CASE LETTER [%02X] (Hex)",Code);

            Code = *Str_ptr++;
            
            if (isupper(Code))
            {
               goto Success;
            }
            
            goto Failure;
         }
         case LOWER_CASE_LETTER:
         {
            SAY(2,"LOWER CASE LETTER [%02X] (Hex)",Code);

            Code = *Str_ptr++;

            if (islower(Code))
            {
               goto Success;
            }
            
            goto Failure;
         }
         case LETTER:
         {
            SAY(2,"LETTER [%02X] (Hex)",Code);

            Code = *Str_ptr++;
            
            if (isalpha(Code))
            {
               goto Success;
            }
            
            goto Failure;
         }
         case CHARACTER:
         {
            SAY(2,"CHARACTER [%02X] (Hex)",Code);
            
            Code = *Str_ptr++;
            
            if (isalnum(Code))
            {
               goto Success;
            }
            
            goto Failure;
         }
         case ANY_OF:
         {
            int   Index, Size;

            SAY(2,"ANY OF [%02X] (Hex)",Code);
            
            if ((0 > (Index = ((int) (Code = *Str_ptr++) >> 3) - *Pat_ptr++)) || Index >= (Size = *Pat_ptr++) || 0 == (Pat_ptr[Index] & PowerOf2[Code & 0x07]))
            {
               goto Failure;
            }
            
            Pat_ptr += Size;

            goto Success;
         }
         case HOLERITH:
         {
             int Length;

            SAY(2,"HOLERITH's literal [%02X] (Hex)",Code);

            Length = 1 + (int) * Pat_ptr++;
            
            if ((Str_ptr + Length) > Str_End)
            {
               goto Failure;
            }
            
            while (Length--)
            {
               if (*Str_ptr++ != *Pat_ptr++)
               {
                  goto Failure;
               }
            }
            
            goto Success;
         }
         case CASE_DEAF_HOLERITH:
         {
             int Length;

            SAY(2,"CASE DEAF HOLERITH's literal [%02X] (Hex)",Code);

            Length = 1 + (int) * Pat_ptr++;
            
            if ((Str_ptr + Length) > Str_End)
            {
               goto Failure;
            }
            
            while (Length--)
            {
               if (tolower(*Str_ptr++) != *Pat_ptr++)
               {
                  goto Failure;
               }
            }
            
            goto Success;
         }
         default:
         {
            // Any other leads to failure
            ErrorDone(eST_WRONG_PATTERN_FORMAT);
            break;
         }
      }
   }

   switch (Code)
   {
      case EXTERNAL_PATTERN:
      {
         SAY(2,"\nMatch Do absolute CALL [%02X] (Hex)",Code);
         
         ThisStat = CALL;
         
         Header.Call.Hdr = StatTag;
         Header.Call.Pat_ptr = Pat_ptr + PTR_SIZE;
         
         Pat_ptr = GetPtr(Pat_ptr);
         
         SAY(8,"\nMatch New Pat_ptr = %lX (Hex)",Pat_ptr);
         SAY(2,"\n Now, emulate a left brace occurence [%02X] (Hex)",Code);                    
       
         SaveStr;                    
         PushASS;
      }
      case LEFT_BRACE:
      {
         SAY(2,"\nMatch Do left brace [%02X] (Hex)",Code);
         
         ThisStat = SELECT;
       
         Header.Select.Hdr = StatTag;
         Header.Select.Pat_ptr = Pat_ptr;
         
         goto Activate;
      }
      case RIGHT_BRACE:
      {
         SetStat(ASS_ptr);
         
         if (ThisStat != SELECT)
         {
            --Pat_ptr;
            goto Success;
         } // It's true select statement

         // We'll member current pattern pointer
         PopASS; // to optimize next alternative search
       
         Header.Select.Pat_ptr = Pat_ptr - 1;
         
         SAY(2,"\nMatch Do right brace [%02X] (Hex)",Code);
         
         goto Sleep;
      }
      case OR:
      {
         SetStat(ASS_ptr);

         if (ThisStat != SELECT)
         {
            --Pat_ptr;
            goto Success;
         } // It's true select statement

         // We'll member current pattern pointer
         PopASS; // to optimize next alternative search
         
         Header.Select.Pat_ptr = Pat_ptr - 1;
         
         SAY(2,"\nMatch Do | [%02X] (Hex)",Code);

         EndOfSelect: /* Find for end of the select statement */

         switch (*Pat_ptr)
         {
            case RIGHT_BRACE:
            {
               /* Ok, now we reach the end of select */
               Pat_ptr++; /* Skip the right brace and continue */
               goto Sleep;
            }
            case OR:
            {
               Pat_ptr++; /* Skip '|' and the next item too */
            }
            default:
            {
               SkipOneItem_and_EndOfSelect;
               break;
            }
         }
      }
      case ARB:
      {
         SAY(2,"\nMatch Do ... - arbitrary number of chars [%02X] (Hex)",Code);                  

         SaveStr;

         ThisStat = ELLIPSIS;
         
         Header.Ellipsis.Hdr = StatTag;
         Header.Ellipsis.Pat_ptr = Pat_ptr;
         
         goto Sleep;
      }
      case FINITE_REPEATER:
      {
         GetInt(Pat_ptr);
       
         Header.Repeater.Wanted = Take.Int;
         
         goto MakeRepeater;
      }
      case GO_TO:
      {
         ThisStat = CALL;

         Header.Call.Hdr = StatTag;
         
         GetInt(Pat_ptr);
         
         SAY(2,"\nMatch Do relative CALL at %ld",Take.Int);
         
         Header.Call.Pat_ptr = Pat_ptr;
         
         Pat_ptr += Take.Int;
         
         goto Activate;
      }
      case ASSIGN:
      {
         SAY(2,"\nMatch Do immediate assignment [%02X] (Hex)",Code);

         ThisStat = ASSIGN_SUBSTR;
         
         Header.Assign.Hdr    = StatTag;
         Header.Assign.Id     = GetWord(Pat_ptr);
         Header.Assign.Offset = (DWORD) (Str_ptr - Str_Beg) + Str_offset;

         goto Activate;
      }
      case QUERY:
      {
         SAY(2,"\nMatch Do NOEMPTY check [%02X] (Hex)",Code);

         ThisStat = NOEMPTY;
         
         Header.Noempty.Hdr = StatTag;
         Header.Noempty.Stub = MakeStub;
         
         goto Activate;
      }
      case DO_NOT_RETURN:
      {
         CleanMSS_and_Success;
      }
      case INVERSE:
      {
         SAY(2,"\nMatch Do NOT [%02X] (Hex)",Code);

         ThisStat = NOT;
         
         Header.Not.Hdr = StatTag;
         Header.Not.Pat_ptr = Pat_ptr;
         
         goto Activate;
      }
      case LITTLE_REPEATER:
      {
         SAY(2,"\nMatch Do * REPEATER [%02X] (Hex)",Code);                  

         SaveStr;

         ThisStat = AS_LITTLE;
         
         Header.As_little.Hdr     = StatTag;
         Header.As_little.Pat_ptr = Pat_ptr;
         Header.As_little.Counter = 0;                    
         
         PushMSS;
            
         SkipOneItem_and_DoLittleRepeater;

         if (*Pat_ptr == DO_NOT_RETURN)
         {
            Pat_ptr++;
         }
         
         goto Success;
      }
      case BIG_REPEATER:
      {
         SAY(2,"\nMatch Do $ REPEATER [%02X] (Hex)",Code);

         ThisStat = AS_MUCH;
         
         Header.As_much.Hdr = StatTag;
         Header.As_much.Pat_ptr = Pat_ptr;
         Header.As_much.Counter = 0;                  
         
         SaveStr;
         
         goto Activate;
      }
   }

Sleep: 

   // Here, if it is necessary to push the statement to the MSS. The statement is indicated by the ThisStat variable.
   PushMSS;

Success: 

   // Any temporary success while matching process leads here ...
   SetStat(ASS_ptr);

   SAY(2,"\nMatch SUCCESS - active statement is %d ",ThisStat);
   
   if (ThisStat == SELECT) // Most frequent case
   {
      goto Process;
   }

   /*
   Now, the statement (ThisStat) achives its local success. Actions:
   (o) Pop the statement header from the ASS
   (o) Make some specific actions
   (o) Push the header to the MSS (most frequent case)
   */
   if (ThisStat != AS_LITTLE)
   {
      PopASS;
   }

   switch (ThisStat) // Select active statement
   {
      case NOT:
      {
         // Clean the MSS then alarm
         SAY(6,"\nMatch NOT: success --> failure [%02X] (Hex)",Code);                    

         CleanMSS_and_Failure;
      }
      case NOEMPTY:
      {
         SAY(6,"\nMatch NOEMPTY check  [%02X] (Hex)",Code);

         if (Header.Noempty.Stub == MakeStub)
         {
            goto WakeUp;
         }

         goto Sleep;
      }
      case REPEATER:
      {
         SAY(6,"\nMatch FINITE REPEATER: %ld times",Header.Repeater.Counter + 1);
       
         if ((++Header.Repeater.Counter) == Header.Repeater.Wanted)
         {
            goto Sleep;
         }
         
         Pat_ptr = Header.Repeater.Pat_ptr;
         
         goto Activate;
      }
      case AS_MUCH:
      {
         SAY(6,"\nMatch $ REPEATER [%02X] (Hex)",Code);

         if (*Pat_ptr != DO_NOT_RETURN)
         {
            Pat_ptr = Header.As_much.Pat_ptr;
            
            Header.As_much.Counter++;
         
            SAY(6," - %ld times OK, just one more",Header.As_much.Counter);
            
            goto Activate;
         }
         else // Disable returns if the FENCE follows the repeater
         {
            SAY(6," - optimized by FENCE [%02X] (Hex)",Code);
            
            Pat_ptr = Header.As_much.Pat_ptr;                        
            
            PushASS;                          
            
            CleanMSS_and_Go;
         }
      }
      case AS_LITTLE:
      {
         SAY(6,"\nMatch * REPEATER [%02X] (Hex)",Code);

         if (*Pat_ptr != DO_NOT_RETURN)
         {
            PopASS;
            Header.As_little.Counter++;

            SAY(6," - %ld times OK",Header.As_little.Counter);
         }
         else
         {
            SAY(6," - optimized by FENCE (skipped) [%02X] (Hex)",Code);

            Pat_ptr++;                            
            CleanMSS_and_ContinueLittleRepeater;                        
            PopASS;
         }
                             
         SaveStr;
         
         goto Sleep;
      }
      case CALL:
      {
         SAY(6,"\nMatch CALL: OK [%02X] (Hex)",Code);

         Pat_ptr = Header.Call.Pat_ptr;/* Return to parent pattern */
         
         SAY(8,"\nMatch Return to Pat_ptr = %lX (Hex)",Pat_ptr);
         
         goto Sleep;
      }
      case ASSIGN_SUBSTR:
      {
         SAY(6,"\nMatch ASSIGN: OK [%02X] (Hex)",Code);

         if (AssignVariable)
         {
            (*AssignVariable) (Header.Assign.Id,Header.Assign.Offset,(DWORD) (Str_ptr - Str_Beg) + Str_offset - Header.Assign.Offset);
         }

         ThisStat = SLEEPING_ASSIGN;
         
         Header.Sleeping_Assign.Hdr = StatTag;
         
         goto Sleep;
      }
      case RANGE:
      {

GeneralSuccess:

         SAY(2,"\nMatch GENERAL SUCCESS [%02X] (Hex)",Code);

         *piPointer = (DWORD) (Str_ptr - Str_Beg) + Str_offset;
         
         ErrorDone(eST_SUCCESS);
      }
      default:
      {
         SAY(0,"\nINTERNAL ERROR - Bad ThisStat %d [S]",ThisStat);

         goto Crash;
      }
   }

WakeUp: 
   
   // Make the statement (ThisStat) active and signal a failure for it ...
   PushASS;

Failure: 
   
   // Any temporaty failure of matching process leads here ... 
   SAY(2,"\nMatch FAILURE [%02X] (Hex) ",Code);

   if (StubOnMSS)
   {
      /*
      Failure of a statement matching. The statement header is on the
      Active Statement Stack. The stub saves string pointer before the
      statement start. Actions:
      (o) Restore string pointer
      (o) Pop statement header from the ASS
      (o) Switch to the statement
      */
      SAY(2,"- stub on MSS [%02X] (Hex)",Code);              

      RestoreStr;

      SAY(4,"\nMatch Unmatch up to Str_ptr = %lX (Hex)",Str_ptr);
      SAY(4,":'%s'",Str_ptr);

      SetStat(ASS_ptr);              
      
      PopASS; /* Pop a statement from the ASS */

      switch (ThisStat) /* Select failed statement */
      {
         case SELECT:
         {
            /* Try other alternative */
            SAY(6,"\nMatch SELECT tries next alternative [%02X] (Hex)",Code);

            Pat_ptr = Header.Select.Pat_ptr;

            IsAlternative : /* Find for next alternative */

            if (*Pat_ptr == OR) /* Exists ... */
            {
               Header.Select.Pat_ptr = ++Pat_ptr;
               goto Activate;
            }
            
            if (*Pat_ptr == RIGHT_BRACE)
            {
               goto Failure;
            }                            
            
            SkipOneItem_and_IsAlternative;
         }
         case NOT:
         {
            /* Successful matching ... */
            SAY(6,"\nMatch NOT: failure --> success [%02X] (Hex)",Code);
          
            Pat_ptr = Header.Not.Pat_ptr;                          
            
            SkipOneItem_and_Success;
         }
         case NOEMPTY:
         {
            // Failure matching ...
            SAY(6,"\nMatch NOEMPTY failed [%02X] (Hex)",Code);

            goto Failure;
         }
         case REPEATER:
         {
            /* Failure matching ... */
            SAY(6,"\nMatch Failure under FINITE REPEATER [%02X] (Hex)",Code);

            if (Header.Repeater.Counter--)
            {
               SAY(6," - %ld times left",Header.Repeater.Counter);
               goto WakeUp;
            }

            SAY(6," - FAILED itself [%02X] (Hex)",Code);

            goto Failure;
         }
         case AS_MUCH:
         {
            /* Always OK ... */
            SAY(6,"\nMatch $ REPEATER is OK [%02X] (Hex)",Code);

            Pat_ptr = Header.As_much.Pat_ptr;                        
            PushMSS;                          
            
            SkipOneItem_and_Success;
         }
         case AS_LITTLE:
         {
            /* Can't match it */
            SAY(6,"\nMatch * REPEATER [%02X] (Hex)",Code);

            if (Header.As_little.Counter--)
            {
               SAY(6," - %ld times left",Header.As_little.Counter);
               goto WakeUp;
            }
            else
            {
               SAY(6," failed [%02X] (Hex)",Code);
               goto Failure;
            }
         }
         case CALL:
         {
            SAY(6,"\nMatch CALL statement failed [%02X] (Hex)",Code);

            /*
            The parent statement will restore its own Pat_ptr. Hence
            there is no necessity to restore Pat_ptr here.
            Pat_ptr = Header.Call.Pat_ptr;
            SAY (8, "\nMatch Return to Pat_ptr = %lX (Hex)", Pat_ptr);
            */
            goto Failure;
         }
         case ASSIGN_SUBSTR:
         {
            SAY(6,"\nMatch ASSIGN failed [%02X] (Hex)",Code);

            goto Failure;
         }
         default:
         {
            SAY(0,"\nINTERNAL ERROR - Bad HDR code %02X (Hex) [F]",ThisStat);

            goto Crash;
         }
      }
   }
   else
   {
      /*
      Awake a statement on the MSS stack. There are bad news for it ...
      Actions:
      (o) Pop sleeping statement header from the MSS
      (o) Switch to avaked statement
      */
      SetStat(MSS_ptr);

      SAY(2,"- awake statement no.%d",ThisStat);            
      
      PopMSS; /* Pop statement from the MSS */
      
      switch (ThisStat) /* Go to the statement */
      {
         case SELECT:
         {
            SAY(6,"\nMatch wake SELECT up [%02X] (Hex)",Code);

            goto WakeUp;
         }
         case ELLIPSIS:
         {
            SAY(6,"\nMatch wake ARB (...) up [%02X] (Hex)",Code);

            Pat_ptr = Header.Ellipsis.Pat_ptr;                            
            
            RestoreStr;
            
            if (EndOfString)
            {
               goto Failure;
            }
            
            Str_ptr++;                            
            SaveStr;
            
            goto Sleep;
         }
         case REPEATER:
         {
            SAY(6,"\nMatch wake FINITE REPEATER up [%02X] (Hex)",Code);

            goto WakeUp;
         }
         case AS_MUCH:
         {
            SAY(6,"\nMatch wake $ REPEATER up [%02X] (Hex)",Code);
            SAY(6," - %ld times left",Header.As_much.Counter - 1);

            if (Header.As_much.Counter--)
            {
               goto WakeUp;
            }
          
            goto Failure;
         }
         case AS_LITTLE:
         {
            SAY(6,"\nMatch wake * REPEATER up [%02X] (Hex)",Code);

            Pat_ptr = Header.As_little.Pat_ptr;                          
            RestoreStr;
            
            goto Activate;
         }
         case CALL:
         {
            SAY(6,"\nMatch wake CALL statement up [%02X] (Hex)",Code);

            goto WakeUp;
         }
         case NOEMPTY:
         {
            SAY(6,"\nMatch wake NOEMPTY up [%02X] (Hex)",Code);

            goto WakeUp;
         }
         case FENCED_ASSIGN:
         {
            SAY(6,"\nMatch wake ASSIGN (fenced) up [%02X] (Hex)",Code);

            if (DeAssignVariable)
            {
               (*DeAssignVariable) (Header.Sleeping_Assign.Id);
            }

            goto Failure;
         }
         case SLEEPING_ASSIGN:
         {
            SAY(6,"\nMatch wake ASSIGN up [%02X] (Hex)",Code);

            if (DeAssignVariable)
            {
               (*DeAssignVariable) (Header.Sleeping_Assign.Id);
            }

            ThisStat = ASSIGN_SUBSTR;
            
            Header.Assign.Hdr = StatTag;
            Header.Assign.Offset = (DWORD) (Str_ptr - Str_Beg) + Str_offset;
            
            goto WakeUp;
         }
         case RANGE:
         {
            SAY(6,"\nMatch GENERAL FAILURE (RANGE awaked) [%02X] (Hex)",Code);

            ErrorDone(eST_FAILURE);
         }
         default:
         {
            SAY(0,"\nINTERNAL ERROR - Bad HDR code [%02X] (Hex) [A]",ThisStat);
          
            goto Crash;
         }
      }
   }

SkipOneItem:

   {
      /*
      Skip one statement of the pattern. In purpose of maximal speed
      achivement this subroutine is implemented as a plain code. The Re-
      turnTo variable is used to specify return point label :
      Value Label
      0 Success
      1 IsAlternative
      2 EndOfSelect
      3 DoLittleRepeater */

      bool     OneMore = false;
      DWORD    Nesting  = 0;

      do
      {
         OneMore = false;

         if ((Code = *Pat_ptr++) <= LAST_ASCII_CHARACTER)
         {
            if (Code)
            {
               while (((Code = *Pat_ptr++) <= LAST_ASCII_CHARACTER) && Code);
               --Pat_ptr;
            }
         }
         else if ((DWORD) Code < FIRST_STATEMENT_CODE)
         {
            if ((DWORD) Code <= MAX_FINITE_REPEATER)
            {
               OneMore = true;
            }
            else if (Code == HOLERITH || Code == CASE_DEAF_HOLERITH)
            {
               Code = *Pat_ptr++;
               Pat_ptr += Code;
            }
            else if (Code == ANY_OF)
            {
               Pat_ptr += 2 + Pat_ptr[1];
            }
            else if (Code > ANY_OF)
            {
               ErrorDone(eST_WRONG_PATTERN_FORMAT);
            }
         }
         else
         {
            switch (Code)
            {
               case LEFT_BRACE:
               {
                  Nesting++;
                  break;
               }
               case OR:
               {
                  if (!OneMore && !Nesting)
                  {
                     --Pat_ptr;
                  }
                
                  break;
               }
               case RIGHT_BRACE:
               {
                  if (!OneMore && !Nesting)
                  {
                     --Pat_ptr;
                  }
                  else
                  {
                     --Nesting;
                  }

                  break;
               }
               case EXTERNAL_PATTERN:
               {
                  Pat_ptr += PTR_SIZE;
                  break;
               }
               case GO_TO:
               case ASSIGN:
               {
                  Pat_ptr += WORD_SIZE;
               }
               case FINITE_REPEATER:
               case LITTLE_REPEATER:
               case DO_NOT_RETURN:
               case BIG_REPEATER:
               case INVERSE:
               case QUERY:
               {
                  OneMore = true;
               }
               case ARB:
               {
                  break;
               }
               default:
               {
                  ErrorDone(eST_WRONG_PATTERN_FORMAT);
                  break;
               }
            }
         }
      }
      while (OneMore || Nesting);
   }

   switch (ReturnTo)
   {
      case 0:
      {
         goto Success;
      }
      case 1:
      {
         goto IsAlternative;
      }
      case 2:
      {
         goto EndOfSelect;
      }
      case 3:
      {
         goto DoLittleRepeater;
      }
      default:
      {
         SAY(0,"\nINTERNAL ERROR [R]",Code); goto Crash;
      }
   }

CleanMSS:

   {
      /*
      To avoid any return in the frame of the current alternative branch
      the MSS must be cleaned up to the stub of the last active statement.
      In the purpose of maximal speed achivement this subroutine
      is implemented as a plain code. The ReturnTo variable is used to
      specify return point label:
      Value Label
      0 Success
      1 Failure
      2 Go
      2 ContinueLittleRepeater
      3 ContinueFence
      4 ContinueBigRepeater
      Additional bits set
      0x10 One more stub is deleted
      0x20 It is deassigned (not saved)
      */

      DWORD    StubCount   = 0;
      DWORD    AssignCount = 0;
      int      ThisStat    = 0;

      SAY(2,"\nMatch Do FENCE",Code);

      while (true)
      {
         if (StubOnMSS)
         {
            if (!StubCount--)
            {
               break;
            }                            

            KillStub;
         }
         else
         {
            SetStat(MSS_ptr);                          

            PopMSS;

            StubCount++;

            switch (ThisStat)
            {
               case CALL:
               case SELECT:
               case NOEMPTY:
               case ELLIPSIS:
                  continue;

               case REPEATER:
                  SAY(8,"\nMatch Delete FINITE REPEATER",Code);
                  StubCount += Header.Repeater.Counter;
                  continue;

               case AS_MUCH:
                  SAY(8,"\nMatch Delete $ REPEATER",Code);
                  StubCount += Header.As_much.Counter;
                  continue;

               case AS_LITTLE:
                  SAY(8,"\nMatch Delete * REPEATER",Code);
                  StubCount += Header.As_little.Counter;
                  continue;

               case FENCED_ASSIGN:
                  --StubCount;

               case SLEEPING_ASSIGN:
               {
                  SAY(8,"\nMatch Deal ASSIGN",Code);

                  if (ReturnTo & 0x20)
                  {
                     /* Deassign */
                     SAY(8,"... deassigned",Code);

                     if (DeAssignVariable != NULL)
                     {
                        (*DeAssignVariable) (Header.Sleeping_Assign.Id);
                     }

                     continue;
                  }
                  else
                  {
                     /* Preserve */
                     SAY(8,"... fenced",Code);

                     ThisStat = FENCED_ASSIGN;
                     
                     Header.Sleeping_Assign.Hdr = StatTag;                                              
                     
                     PushASS;
                     
                     AssignCount++;
                     
                     continue;
                  }
               }
               default:
               {
                  SAY(0,"INTERNAL ERROR [FENCE]!",Code);
                  goto Crash;
               }
            }
         }
      }

      if (ReturnTo & 0x10)
      {
         KillStub;
      }

      while (AssignCount--) /* This is necessary for DeAssign */
      {
         PopASS;                      
         PushMSS;
      }
   }

   switch (ReturnTo & 0x0F)
   {
      case 0:
      {
         goto Success;
      }
      case 1:
      {
         goto Failure;
      }
      case 2:
      {
         goto Go;
      }
      case 3:
      {
         goto ContinueLittleRepeater;
      }
      default:
      {
         SAY(0,"\nINTERNAL ERROR [R]",Code); 
         goto Crash;
      }
   }

CantReturn:

   ErrorDone(eST_CANNOT_RETURN);

NoMoreMemory:

   Free(); // release memory allocated by myself
   
   ErrorDone(eST_NO_DYNAMIC_MEMORY);

Crash:

   ErrorDone(eST_INTERNAL_ERROR);
}

/*
MatchKeyWord -- Compare Keyword

String - points to a string
Str_End - points to a BYTE followed the string
Text - points to a keyword. The keyword can be
followed by NUL, SPACE, TAB or RIGHT_ANGLE.

Returns :

Pointer to a character followed the keyword. A failure may be
encountered as MatchKeyWord == String.

Matching is not case sensive. Keyword string ended by letter, digit
or underline (_) must be followed by other from mentioned above characters.
*/

/* ******************************************************************** **
** @@ PatternMatcher::MatchKeyWord()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

BYTE* PatternMatcher::MatchKeyWord(BYTE* String,BYTE* Str_End,BYTE* Text)
{
   BYTE* Str_ptr = String;

   BYTE  Code = 0;

   while ((Code = (BYTE)tolower(*Text++)) && (Code != RIGHT_ANGLE) && (Code != TAB) && (Code != SPACE))
   {
      if (EndOfString || Code != (BYTE) tolower(*Str_ptr++))
      {
         return String;
      }
   }

   if (!EndOfString && (IsUserAlpha(*(Str_ptr - 1)) || isdigit(Code)) && (IsUserAlpha(*Str_ptr) || isdigit(Code)))
   {
      return String;
   }

   return Str_ptr;
}

/*
First_pass - First pass of the pattern translation.
Task of the first pass is to build label table
and to estimate size of pattern's internal representation.
Some errors are recognized during this pass: brace errors, duplicate labels and so on.

Str_ptr - Source string pointer
Str_End - Points to the first BYTE following source string
New_Str_ptr - Points to the new string pointer
Error_code - Error code (not changed if OK)
Error_ptr - Points to an error location

Returns: Length of pattern's internal representation
*/

/* ******************************************************************** **
** @@ PatternMatcher::First_pass()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

DWORD PatternMatcher::First_pass
(
   BYTE*                Str_ptr,
   BYTE*                Str_End,
   BYTE**               New_Str_ptr,
   E_STATUS_CODE*       Error_code,
   BYTE**               Error_ptr
)
{
   BYTE        Code           = 0;
   BYTE*       Lex_ptr        = NULL;
   BYTE*       Lex_End        = NULL;
   DWORD       Pat_size       = 0;
   BYTE        Zero           = 0; // for PushXB only
   bool        AfterLiteral   = false;

   HeaderUnion    Header;

   MSS_mask = 0;

   while (!EndOfString)
   {
      Lex_ptr = Str_ptr;

      if (RABBIT_EARS == (Code = *Str_ptr++) || APOSTROPHE == Code || LEFT_ANGLE == Code)
      {  
         bool  EightBit   = false;
         bool  Circumflex = false;

         BYTE Quotation = Code;

         DWORD Length = 0;

         SAY(2,"\n*** 1st Pass [1] Text constant %c",Code);
         
         if (Quotation == LEFT_ANGLE)
         {
            Quotation = RIGHT_ANGLE;
            EightBit = true;
         }
         
         while (!EndOfString && ((Code = *Str_ptr++) != Quotation))
         {
            SAY(2,"%c",Code);

            if (!Circumflex && (Code == CIRCUMFLEX))
            {
               Circumflex = true;
            }
            else
            {
               if (Circumflex)
               {
                  Code = DoCircumflex(Code);
                  Circumflex = false;
               }
               Length++;
               EightBit = (char) (EightBit || !Code || (Code > LAST_ASCII_CHARACTER));
            }
         }
         
         if (Code != Quotation)
         {
            Error(eST_MISSING_QUOTATION,Str_ptr);
         }
         
         SAY(2,"%c",Code);
         
         if (Circumflex)
         {
            Length++;
         }
         
         Pat_size += Length;
         
         if (EightBit)
         {
            AfterLiteral = false;
            if (Length > MAX_HOLERITH)
            {
               Pat_size += ((Length + MAX_HOLERITH - 1) / MAX_HOLERITH + 1) * 2;
            }
            else
            {
               Pat_size += 2;
            }
         }
         else
         {
            if (!Length || AfterLiteral)
            {
               Pat_size++;
            }
            if (!Length)
            {
               AfterLiteral = false;
            }
            else
            {
               AfterLiteral = true;
            }
         }
      }
      else if (Code == '{')
      {
          int Low = 255;
          int High = 0;
         
          bool Circumflex = false;

         SAY(2,"\n1st Pass [2] One of the character set {",Code);
         
         AfterLiteral = false;
         
         while (!EndOfString && ('}' != (Code = *Str_ptr++)))
         {
            SAY(2,"%c",Code);

            if (!Circumflex && (Code == CIRCUMFLEX))
            {
               Circumflex = true;
            }
            else
            {
               if (Circumflex)
               {
                  Code = DoCircumflex(Code);
                  Circumflex = false;
               }
               if (Low > Code)
               {
                  Low = Code;
               }
               if (High < Code)
               {
                  High = Code;
               }
            }
         }
         
         if (Code != '}')
         {
            Error(eST_MISSING_QUOTATION,Str_ptr);
         }
         
         SAY(2,"}",Code);

         if (Circumflex)
         {
            if (Low > CIRCUMFLEX)
            {
               Low = CIRCUMFLEX;
            }
            if (High < CIRCUMFLEX)
            {
               High = CIRCUMFLEX;
            }
         }

         Pat_size += 3 + (Low > High ? 0 : 1 + (High >> 3) - (Low >> 3));
      }
      else if (Code == SPACE || Code == TAB)
      {
         PassSpace;
      }
      else if (isdigit(Code))
      {
          DWORD Repetition_count = 0;

         AfterLiteral = false;
         
         SAY(2,"\n1st Pass [3] Finite repeater (at %lX) ",Str_ptr);
         
         --Str_ptr;
         
         do
         {
            Code = *Str_ptr++;
            if (Repetition_count <= MAX_INT_DIV_10)
            {
               Repetition_count *= 10;
            }
            else
            {
               Error(eST_TOO_BIG_REPEATER,Lex_ptr);
            }

            switch (Code)
            {
               case '9':
                  Repetition_count += 9; break;
               
               case '8':
                  Repetition_count += 8; break;
               
               case '7':
                  Repetition_count += 7; 
                  break;
               
               case '6':
                  Repetition_count += 6; 
                  break;
               
               case '5':
                  Repetition_count += 5;
                   break;
               
               case '4':
                  Repetition_count += 4; 
                  break;
               
               case '3':
                  Repetition_count += 3; 
                  break;
               
               case '2':
                  Repetition_count += 2;
                   break;
               
               case '1':
                  Repetition_count += 1;
                   break;
               
               case '0':
                  break;
               
               default:
                  Repetition_count /= 10;
                  --Str_ptr;

                  goto CollapseRepeater;
            }

            SAY(2,"%c",Code);
         }
         while (!EndOfString);

         CollapseRepeater :

         if (Repetition_count > MAX_REPEATER_COUNT)
         {
            Pat_size += 1 + WORD_SIZE;
         }
         else
         {
            Pat_size += 1;
         }

         SAY(2," (stop at %lX) ",Str_ptr);
      }
      else if (Code != LF)
      {
         SAY(2,"\n1st Pass [4] Keyword beginning with (%c)",Code);
         
         AfterLiteral = false;

         switch (Code)
         {
            case '(' :
            case '[' :
               if (MSS_mask == LAST_MSS_MASK || !MSS_mask)
               {
                  PushStack(MSS_ptr,MSS_used,&Zero,1);
                  MSS_mask = 1;
               }
               else
               {
                  MSS_mask <<= 1;
               }

               if (Code == '(')
               {
                  *(MSS_ptr - 1) &= ~MSS_mask;
                  
                  SAY(4,"\n ( MSS_mask = %X (Hex)",MSS_mask);
               }
               else
               {
                  *(MSS_ptr - 1) |= MSS_mask;

                  SAY(4,"\n [ MSS_mask = %X (Hex)",MSS_mask);
               }

               Pat_size++;
               
               break;
           
            case ')' :
               PopBrace(Code);
               if (Code)
               {
                  Error(eST_BRACE_ERROR,Lex_ptr);
               }
               Pat_size++;
               break;
           
            case ']' :
               PopBrace(Code);
               if (Code != 1)
               {
                  Error(eST_BRACE_ERROR,Lex_ptr);
               }

               Pat_size += 2;

               break;
            default:
            {
                BYTE KeyWordNo;

               for (KeyWordNo = 0; (KeyWordNo < KEY_LIST_SIZE && 0 == KeyWord((BYTE*) KeyList[KeyWordNo].Name)); KeyWordNo++)
               {
                  ;
               }

               if (KeyWordNo < KEY_LIST_SIZE)
               {
                  if (IsUserAlpha(*Lex_ptr))
                  {
                     PassSpace;
                     if (!EndOfString)
                     {
                        switch (*Str_ptr)
                        {
                           case RIGHT_ANGLE :
                              Error(eST_RESERVED_KEYWORD,Lex_ptr);
                           case EQUATION :
                              goto Assignment;
                        }
                     }
                  }
                  
                  SAY(2," recognized as %hX (Hex)",KeyList[KeyWordNo].Code);
                  
                  Pat_size++;
               }
               else
               {
                  if (IsUserAlpha(*Lex_ptr))
                  {
                     PassId;
                     if (EndOfString)
                     {
                        goto ReferenceToLabel;
                     }
                     switch (*Str_ptr)
                     {
                        case RIGHT_ANGLE :
                           {
                              BYTE*    New_Str_ptr = ++Str_ptr;
                              BYTE*    Old_ASS_ptr = ASS_ptr;

                              WORD  Old_ASS_used = ASS_used;

                              if (GetExternalPattern && (*GetExternalPattern)((char*)Lex_ptr,Lex_End - Lex_ptr))
                              {
                                 Error(eST_RESERVED_KEYWORD,Lex_ptr);
                              }

                              SAY(2," defines a label",Code);

                              while (!EmptyASS)
                              {
                                 PopLabel;

                                 if (KeyWord(Header.Label.Name))
                                 {
                                    Error(eST_DUPLICATE_LABEL,Lex_ptr);
                                 }
                              }

                              ASS_ptr = Old_ASS_ptr;
                              ASS_used = Old_ASS_used;
                              
                              Header.Label.Name = Lex_ptr;
                              
                              Header.Label.Destination_offset = Pat_size;                                                                      
                              PushLabel;
                              
                              Str_ptr = New_Str_ptr;
                           } 
                           
                           break;

                        case EQUATION:
                           Assignment : Str_ptr++;

                        default:
                           ReferenceToLabel : Pat_size += 1 + WORD_SIZE;
                           SAY(2," is label or equation",Code);
                           break;
                     }
                  }
                  else
                  {
                     PopBrace(Code);

                     if (Code != 2)
                     {
                        Error(eST_UNRECOGNIZED_CHARACTER,Lex_ptr);
                     }

                     *New_Str_ptr = Str_ptr;

                     return (++Pat_size);
                  }
               }

               break;
            }
         }
      }
   }

   PopBrace(Code);

   if (Code != 2)
   {
      Error(eST_MISSING_RIGHT_BRACE,Str_ptr);
   }
   
   *New_Str_ptr = Str_ptr;
   
   return (++Pat_size);

NoMoreMemory:

   Free(); // release memory allocated by myself

   Error(eST_NO_DYNAMIC_MEMORY,Str_ptr);
}

/*
CloseStatement -- Pop the MSS until a select statement

ResultStatus - EMPTY_ATOM if a subpattern matches null string,
NON_EMPTY_ATOM - otherwise

Returns :

EMPTY_ATOM if there is possibility to match null string
NON_EMPTY_ATOM if there is no possibility to match null
ERROR_STATUS if $ or * repeat a pattern which can match null
*/

BYTE  CloseStatement(BYTE ResultStatus)
{
   BYTE  EmptyStatus;

   while (GetHdr(MSS_ptr) & NON_SELECT_MASK)
   {
      PopStatement;

      switch (EmptyStatus)
      {
         case INDEFINITE:
            if (ResultStatus == EMPTY_ATOM)
            {
               return (ERROR_STATUS);
            }

            ResultStatus = EMPTY_ATOM;
        
         case SON_WILL_SAY :
            break;
        
         default :
            ResultStatus = (BYTE)(EmptyStatus >> NON_SELECT_MASK);
            break;
      }
   }

   *(MSS_ptr - 1) |= ResultStatus;
   
   return (ResultStatus);
} 

/*
Second_pass - Second pass of the pattern translation.

Str_ptr - Source string pointer
Str_End - Points to the first BYTE following source string
Pat_ptr - Points to a buffer for the pattern allocation
Error_code - Error code (not changed if OK)
Error_ptr - Points to an error location
*/

/* ******************************************************************** **
** @@ PatternMatcher::Second_pass()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

int PatternMatcher::Second_pass
(
   BYTE*             Str_ptr,
   BYTE*             Str_End,
   BYTE*             Pat_ptr,
   E_STATUS_CODE*    Error_code,
   BYTE**            Error_ptr
)
{
   BYTE     Code = 0;
   
   BYTE*    Lex_ptr = NULL;
   BYTE*    Lex_End = NULL;
   BYTE*    Pat_Beg = Pat_ptr;

   BYTE     EmptyStatus = SELECT_BRANCH;

   bool     AfterLiteral = false;

   HeaderUnion Header;                                  

   PushStatement;

   while (!EndOfString)
   {
      Lex_ptr = Str_ptr;

      if (((Code = *Str_ptr++) != SPACE) && (Code != TAB) && (Code != LF))
      {
         EmptyStatus = NON_EMPTY_ATOM;

         if ((Code == RABBIT_EARS) || (Code == APOSTROPHE) || (Code == LEFT_ANGLE))
         {
            bool     EightBit   = false;
            bool     Broken     = false;
            bool     Circumflex = false;

            BYTE     Quotation = Code;
            
            DWORD    Length  = 0;
            DWORD    Counter = 0;

            SAY(4,"\n2nd Pass [1] Text constant %c",Code);
            
            if (Quotation == LEFT_ANGLE)
            {
               Quotation = RIGHT_ANGLE;
               EightBit = true;
            }

            while (!EndOfString && ((Code = *Str_ptr++) != Quotation))
            {
               SAY(4,"%c",Code);

               if (!Circumflex && (Code == CIRCUMFLEX))
               {
                  Circumflex = true;
               }
               else
               {
                  if (Circumflex)
                  {
                     Code = DoCircumflex(Code);
                     Circumflex = false;
                  }

                  if (Quotation == RIGHT_ANGLE)
                  {
                     Code = (BYTE) tolower(Code);
                  }
                  
                  Length++;

                  EightBit = (BYTE) (EightBit || !Code || (Code > LAST_ASCII_CHARACTER));
               }
            }
            
            SAY(4,"%c",Code);
            
            if (Circumflex)
            {
               Length++;
            }
            
            if (!Length)
            {
               EmptyStatus = EMPTY_ATOM;
            }
            
            Str_ptr = ++Lex_ptr;
            
            Broken = (bool) (Length > MAX_HOLERITH);
            
            SAY(2,"\n2nd Pass [2] << ",Code);

            if (Broken)
            {
               *Pat_ptr++ = LEFT_BRACE;
               
               SAY(2,"(",Code);
            }

            if (Broken || EightBit)
            {
               AfterLiteral = false;
            }
            else
            {
               if (!Length || AfterLiteral)
               {
                  *Pat_ptr++ = NOOP;

                  SAY(6," [NOOP to separate literals]",Code);
               }

               if (!Length)
               {
                  AfterLiteral = false;
               }
               else
               {
                  AfterLiteral = true;
               }
            }
            
            Counter = Length;

            while (Counter)
            {
               Length = Counter;

               if (Broken || EightBit)
               {
                  if (Length > MAX_HOLERITH)
                  {
                     Length = MAX_HOLERITH;
                  }

                  if (Quotation == RIGHT_ANGLE)
                  {
                     *Pat_ptr++ = CASE_DEAF_HOLERITH;
                     
                     SAY(2," CASE DEAF [%d] ",Length - 1);
                  }
                  else
                  {
                     *Pat_ptr++ = HOLERITH;

                     SAY(2," HOLERITH [%d] ",Length - 1);
                  }
               
                  *Pat_ptr++ = (BYTE)(Length - 1);
               }
               
               Counter -= Length;
               Circumflex = false;

               while (Length)
               {
                  Code = *Str_ptr++;

                  if (!Circumflex && (Code == CIRCUMFLEX))
                  {
                     Circumflex = true;
                  }
                  else
                  {
                     --Length;

                     if (Circumflex)
                     {
                        *Pat_ptr++ = DoCircumflex(Code);
                        
                        SAY(2,"%c",DoCircumflex(Code));
                        
                        Circumflex = false;
                     }
                     else
                     {
                        if (Quotation == RIGHT_ANGLE)
                        {
                           Code = (BYTE)tolower(Code);
                        }

                        *Pat_ptr++ = Code;
                        
                        SAY(2,"%c",Code);
                     }
                  }
               }
            }

            if (EmptyStatus != EMPTY_ATOM && *(Str_ptr - 1) == Quotation)
            {
               *(Pat_ptr - 1) = CIRCUMFLEX; // End cicumflex means itself
               
               SAY(2,"%c",CIRCUMFLEX);
            }
            else
            {
               Str_ptr++; // Pass closing quotation marks
            }

            if (Broken)
            {
               *Pat_ptr++ = RIGHT_BRACE;
               
               SAY(2,")",Code);
            }
         }
         else if (Code == '{')
         {
             int Low  = 255;
             int High = 32;
             
             bool Circumflex = false;

            static BYTE Map[32];

            SAY(2,"\n2nd Pass [3] One of the character set {",Code);
            
            while (--High)
            {
               Map[High] = 0;
            }
            
            AfterLiteral = false;
            
            EmptyStatus = RETURN_NON_EMPTY;

            while (!EndOfString && ('}' != (Code = *Str_ptr++)))
            {
               SAY(2,"%c",Code);

               if (!Circumflex && (Code == CIRCUMFLEX))
               {
                  Circumflex = true;
               }
               else
               {
                  if (Circumflex)
                  {
                     Code = DoCircumflex(Code);
                     Circumflex = false;
                  }

                  if (Low > Code)
                  {
                     Low = Code;
                  }

                  if (High < Code)
                  {
                     High = Code;
                  }

                  Map[Code >> 3] |= PowerOf2[Code & 0x07];
               }
            }
            
            SAY(2,"} =",Code);

            if (Circumflex)
            {
               if (Low > CIRCUMFLEX)
               {
                  Low = CIRCUMFLEX;
               }

               if (High < CIRCUMFLEX)
               {
                  High = CIRCUMFLEX;
               }

               Map[CIRCUMFLEX >> 3] |= PowerOf2[CIRCUMFLEX & 0x07];
            }

            *Pat_ptr++ = ANY_OF;
            
            if (Low > High)
            {
               *Pat_ptr++ = 0;
               *Pat_ptr++ = 0;
               SAY(2," nothing",Code);
            }
            else
            {
               *Pat_ptr++ = (BYTE) (Low >>= 3);
               *Pat_ptr++ = (BYTE) (High = 1 + (High >> 3) - Low);

               while (High--)
               {
                  SAY(2," %X",Map[Low]);

                  *Pat_ptr++ = Map[Low++];
               }
            }
         }
         else if (isdigit(Code))
         {  
            DWORD Repetition_count = 0;

            AfterLiteral = false;
            
            --Str_ptr;

            do
            {
               Code = *Str_ptr++;
            
               Repetition_count *= 10;
            
               switch (Code)
               {
                  case '9' :
                  {
                     Repetition_count += 9; 
                     break;
                  }
                  case '8' :
                  {
                     Repetition_count += 8; 
                     break;
                  }
                  case '7' :
                  {
                     Repetition_count += 7; 
                     break;
                  }
                  case '6' :
                  {
                     Repetition_count += 6; 
                     break;
                  }
                  case '5' :
                  {
                     Repetition_count += 5; 
                     break;
                  }
                  case '4' :
                  {
                     Repetition_count += 4; 
                     break;
                  }
                  case '3' :
                  {
                     Repetition_count += 3; 
                     break;
                  }
                  case '2' :
                  {
                     Repetition_count += 2; 
                     break;
                  }
                  case '1' :
                  {
                     Repetition_count += 1; 
                     break;
                  }
                  case '0' :
                  {
                     break;
                  }
                  default :
                  {
                     Repetition_count /= 10;
                     --Str_ptr;
                     goto CollapseRepeater;
                  }
               }
            }
            while (!EndOfString);

CollapseRepeater:

            switch (Repetition_count)
            {
               case 0:
               {
                  EmptyStatus = RETURN_EMPTY; 
                  break;
               }
               default:
               {
                  EmptyStatus = SON_WILL_SAY;
                  break;
               }
            }

            if (Repetition_count <= MAX_REPEATER_COUNT)
            {
               *Pat_ptr++ = (BYTE) (Repetition_count + LAST_ASCII_CHARACTER + 1);
               
               SAY(2,"\n2nd Pass [4] << repeat %d (small)",Repetition_count);
            }
            else
            {
               *Pat_ptr++ = FINITE_REPEATER;
               
               Take.Int = Repetition_count;
               
               PutInt(Pat_ptr);

               SAY(2,"\n2nd Pass [5] << repeat %d (big)",Repetition_count);
            }
         }
         else
         {
            SAY(2,"\n2nd Pass [6] Keyword beginning with (%c)",Code);

            AfterLiteral = false;
            
            switch (Code)
            {
               case ']':
                  CloseBranch(EMPTY_ATOM); *Pat_ptr++ = OR;
               case ')':
                  CloseBranch(EMPTY_ATOM); *Pat_ptr++ = RIGHT_BRACE;                                      
                  PopStatement;
                  EmptyStatus = (BYTE) ((EmptyStatus & ~SELECT_MASK) >> SELECT_SHIFT);
                  break;
               case '(':
               case '[':
                  *Pat_ptr++ = LEFT_BRACE;
                  EmptyStatus = SELECT_BRANCH;
                  break;
               default:
                  {  
                     BYTE KeyWordNo;

                     for (KeyWordNo = 0; (KeyWordNo < KEY_LIST_SIZE && 0 == KeyWord((BYTE*) KeyList[KeyWordNo].Name)); KeyWordNo++)
                     {
                        ;
                     }

                     if (KeyWordNo < KEY_LIST_SIZE)
                     {
                        if (IsUserAlpha(*Lex_ptr))
                        {
                           PassSpace;

                           if (!EndOfString && *Str_ptr == EQUATION)
                           {
                              goto Assignment;
                           }
                        }

                        SAY(2," recognized as %hX (Hex)",KeyList[KeyWordNo].Code);

                        *Pat_ptr++ = KeyList[KeyWordNo].Code;

                        switch (KeyList[KeyWordNo].Code)
                        {
                           case OR:
                              CloseBranch(EMPTY_ATOM);
                              continue;

                           case ELLIPSIS:
                           case END_OF_STRING:
                           case END_OF_KEYWORD:
                              EmptyStatus = EMPTY_ATOM;
                              break;

                           case BIG_REPEATER:
                           case LITTLE_REPEATER:
                              EmptyStatus = INDEFINITE;
                              break;

                           case QUERY:
                              EmptyStatus = RETURN_NON_EMPTY;
                              break;

                           case INVERSE:
                              EmptyStatus = RETURN_EMPTY;
                              break;
                        }
                     }
                     else
                     {
                        PassId;

                        if (EndOfString)
                        {
                           goto ReferenceToLabel;
                        }

                        switch (*Str_ptr)
                        {
                           case RIGHT_ANGLE:
                              Str_ptr++;
                              
                              SAY(2," is label declaration",Code);
                              
                              continue;

                           case EQUATION:
                              Assignment:
                              {
                                 DWORD  Id;

                                 Str_ptr++;

                                 SAY(2," is immediate assignment",Code);

                                 if (GetVariableId == NULL || (Id = (*GetVariableId) ((char*) Lex_ptr,Lex_End - Lex_ptr)) == 0)
                                 {
                                    Error(eST_UNDEFINED_VARIABLE,Lex_ptr);
                                 }

                                 *Pat_ptr++ = ASSIGN;

                                 PutWord(Pat_ptr,Id);
                                 EmptyStatus = SON_WILL_SAY;
                              }

                              break;

                           default:
                              ReferenceToLabel:
                              {
                                 BYTE*    New_Str_ptr = Str_ptr;
                                 BYTE*    Old_ASS_ptr = ASS_ptr;
                                 
                                 WORD Old_ASS_used   = ASS_used;

                                 SAY(2," is reference to a label",Code);
                                 
                                 Find_for_label :
                                 
                                 if (EmptyASS)
                                 {
                                    SAY(2," EXTERNAL one!",Code);

                                    char*    Pattern = NULL;

                                    if (GetExternalPattern)
                                    {
                                       Pattern = (*GetExternalPattern)((char*)Lex_ptr,Lex_End - Lex_ptr);
                                    }

                                    if (!GetExternalPattern || !Pattern)
                                    {
                                       Error(eST_UNRECOGNIZED_KEYWORD,Lex_ptr);
                                    }

                                    *Pat_ptr++ = EXTERNAL_PATTERN;

                                    PutPtr(Pat_ptr,(BYTE *) Pattern);
                                 }
                                 else
                                 {
                                    PopLabel;

                                    if (KeyWord(Header.Label.Name))
                                    {
                                       *Pat_ptr++ = GO_TO;

                                       Take.Int = ((long) (Pat_Beg - Pat_ptr) - WORD_SIZE + Header.Label.Destination_offset);

                                       SAY(4," (rel.offs. = %d )",Take.Int);

                                       PutInt(Pat_ptr);
                                    }
                                    else
                                    {
                                       goto Find_for_label;
                                    }
                                 }

                                 ASS_ptr = Old_ASS_ptr;

                                 ASS_used = Old_ASS_used;

                                 Str_ptr = New_Str_ptr;
                              }
                        }
                     }
                  }
            }
         }
         switch (EmptyStatus)
         {
            case EMPTY_ATOM:
            case NON_EMPTY_ATOM:
               FindForSelect(EmptyStatus);
               break;

            default:
               PushStatement;
         }
      }
   }

   *Pat_ptr = RIGHT_BRACE;

   return 0;

NoMoreMemory:

   Free(); // release memory allocated by myself

   Error(eST_NO_DYNAMIC_MEMORY,Str_ptr);

   IndefiniteLoop:

   Error(eST_POSSIBLE_INDEFINITE_LOOP,--Str_ptr);
}

/*
Translate -- pattern translator

Length - of a character string containing a pattern to be translated
String - points to the string (maybe not NUL terminated)
Pointer - number of starting character (0..Length)
Pattern - address of a buffer for translated pattern
Size - its size

Returns :
eST_SUCCESS - Success
eST_BRACE_ERROR - Right brace does not match the left one
eST_DUPLICATE_LABEL - Duplicate definition of a label
eST_FAILURE - Failure. There is no any pattern
eST_MISSING_QUOTATION - Missing closing quotation marks
eST_MISSING_RIGHT_BRACE - One or more missing right braces
eST_NO_DYNAMIC_MEMORY - Dynamic memory overflow
eST_POSSIBLE_INDEFINITE_LOOP - Repetition of a pattern that matches null
eST_TOO_BIG_REPEATER - Repetition count is too big
eST_TOO_LARGE_PATTERN - Pattern is too large for provided buffer
eST_TOO_LARGE_STRING - Pattern length exceeds 1Gbyte
eST_UNDEFINED_VARIABLE - Immediate assignment to unknown variable
eST_UNRECOGNIZED_CHARACTER - Unrecognized character
eST_UNRECOGNIZED_KEYWORD - Unrecognized keyword or undefined label
eST_RESERVED_KEYWORD - Empty or conflict variable name

After successful completion String [*Pointer] will be the 1st charac-
ter following translated pattern. Size will contain actual size of
the pattern i.e. number of used out bytes of the Pattern buffer.

*/

/* ******************************************************************** **
** @@ PatternMatcher::Translate()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

int PatternMatcher::Translate
(
   const char* const       pszText,
   int                     iSize,
   int*                    piPointer,
   BYTE*                   pPattern,
   int*                    piSize
)
{
   ASSERT(iSize > 0);

   BYTE*    Str_ptr = (BYTE*)&pszText[*piPointer];
   BYTE*    Pat_ptr = pPattern;

   BYTE*    Str_Beg = (BYTE*)pszText;
   BYTE*    Str_End = (BYTE*)&pszText[iSize];

   DWORD    Pat_size = 0;

   BYTE*    Error_ptr = NULL;
   
   WORD  Old_MSS_mask = MSS_mask;                          

   StackData;                            

   CheckString;                        
   SaveStack;
   
   _MatchError = eST_SUCCESS;
   
   Pat_size = First_pass(Str_ptr,Str_End,&Str_End,&_MatchError,&Error_ptr);
   
   MSS_mask = Old_MSS_mask;

   if (_MatchError != eST_SUCCESS)
   {
      goto FatalError;
   }
   
   if (Pat_size > MAX_STRING_LENGTH)
   {
      ErrorDone(eST_TOO_LARGE_STRING);
   }
   
   *piSize    = (int)Pat_size;
   *piPointer = Str_End - Str_Beg;

   if ((int) Pat_size > *piSize)
   {
      *piSize = 0;

      ErrorDone(eST_TOO_LARGE_PATTERN);
   }

   MSS_ptr  = Old_MSS_ptr;    // Clean MSS after 1st Pass
   MSS_used = Old_MSS_used;

   Second_pass(Str_ptr,Str_End,Pat_ptr,&_MatchError,&Error_ptr);

   if (_MatchError != eST_SUCCESS)
   {
      goto FatalError;
   }

   Done(eST_SUCCESS);

   FatalError:

   *piSize = 0;

   *piPointer = Error_ptr - Str_Beg;
   
   Done(_MatchError);
}

/*
patmaker -- pattern constructor

String - points to the NUL terminated string

Returns :

Pointer to translated pattern or NULL

The String is translated and a memory block is requested to allocate
the pattern body. Any syntax or other error leads to the NULL as a
result. Note that whole String must be recognized as a pattern. Error
code is returned to the _MatchError variable. You should member that
error may be resulted from malloc's failure.
*/

/* ******************************************************************** **
** @@ PatternMatcher::Compile()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

const BYTE* const PatternMatcher::Compile(const char* const pszExpression)
{
   BYTE*    Pat_ptr = NULL;
   BYTE*    Str_ptr = (BYTE*)pszExpression;
   BYTE*    Str_End = (BYTE*)pszExpression;

   DWORD    Pat_size = 0;
   BYTE*    Error_ptr = NULL;

   WORD Old_MSS_mask = MSS_mask;                          

   StackData;                            
   SaveStack;

   _MatchError = eST_SUCCESS;
   
   Str_End += strlen(pszExpression);
   
   Pat_size = First_pass(Str_ptr,Str_End,&Str_ptr,&_MatchError,&Error_ptr);
   
   MSS_mask = Old_MSS_mask;
   
   if (_MatchError != eST_SUCCESS)
   {
      goto Stop;
   }
   
   if (Str_ptr != Str_End)
   {
      _MatchError = eST_UNRECOGNIZED_KEYWORD;
      goto Stop;
   }
   
   if (Pat_size > MAX_STRING_LENGTH)
   {
      _MatchError = eST_TOO_LARGE_PATTERN;
      goto Stop;
   }
   
   if (NULL == (Pat_ptr = (BYTE *) malloc(Pat_size)))
   {
      _MatchError = eST_NO_DYNAMIC_MEMORY;
      goto Stop;
   }
   
   MSS_ptr = Old_MSS_ptr; /* Clean MSS after 1st Pass */
   MSS_used = Old_MSS_used;
   
   Second_pass((BYTE*)pszExpression,Str_End,Pat_ptr,&_MatchError,&Error_ptr);
 
   if (_MatchError == eST_SUCCESS)
   {
      Done(Pat_ptr);
   }
   
   free(Pat_ptr);

Stop:

   Done(NULL);
}

/*
patinit -- pattern constructor (see also patmaker)

Length - of a character string containing a pattern to be translated
String - points to the string (maybe not NUL terminated)
Pointer - number of starting character (0..Length)

Returns :

The address of the translated pattern or 0

After successful completion String [*Pointer] will be the 1st
character following translated pattern. The AllocatePattern function
is called to allocate translated pattern. On success the pattern
address is returned. Otherwise the result is 0. The _MatchError
variable can be consulted for the actual error code.
*/

/* ******************************************************************** **
** @@ PatternMatcher::Init()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

const char* const PatternMatcher::Init
(
   const char* const       pszText,
   int                     iSize,
   int*                    piPointer
)
{
   ASSERT(iSize > 0);

   BYTE*    Str_ptr   = (BYTE*)&pszText[*piPointer];
   BYTE*    Pat_ptr   = NULL;
   BYTE*    Str_End   = (BYTE*)&pszText[iSize];
   BYTE*    Str_Beg   = (BYTE*)pszText;
   BYTE*    Error_ptr = NULL;

   DWORD    Pat_size = 0;

   WORD Old_MSS_mask = MSS_mask;                          
   
   StackData;                            
   CheckString;                        
   SaveStack;

   _MatchError = eST_SUCCESS;

   Pat_size = First_pass(Str_ptr,Str_End,&Str_End,&_MatchError,&Error_ptr);

   MSS_mask = Old_MSS_mask;

   if (_MatchError != eST_SUCCESS)
   {
      goto FatalError;
   }

   if (Pat_size > MAX_STRING_LENGTH || 0 == AllocatePattern || (0 == (Pat_ptr = (BYTE *) (*AllocatePattern) ((DWORD) Pat_size))))
   {
      /* No memory for the pattern */
      _MatchError = eST_TOO_LARGE_PATTERN;
      Error_ptr  = Str_End;
      goto FatalError;
   }

   MSS_ptr  = Old_MSS_ptr; /* Clean MSS after 1st Pass */
   MSS_used = Old_MSS_used;

   *piPointer = Str_End - Str_Beg;

   Second_pass(Str_ptr,Str_End,Pat_ptr,&_MatchError,&Error_ptr);

   if (_MatchError != eST_SUCCESS)
   {
      goto FatalError;
   }

   Done((char*)Pat_ptr);

   FatalError:

   *piPointer = Error_ptr - Str_Beg;

   Done(0);
}


#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024 * 128)
#endif

enum E_MATCH_ERROR
{
   eME_ERR_NONE,
   eME_ERR_SYNTAX,
   eME_ERR_FATAL
};

/*
#define MAX_PATTERN_LENGTH          (75 * 27 + 3)
#define NUMBER_OF_DISPLAYS          (1)
#define NUMBER_OF_ERRORS            (15)
#define NUMBER_OF_PATTERNS          (9)
#define NUMBER_OF_EXT_PATTERNS      (16)
#define NUMBER_OF_VARIABLES         (8)
#define LF                          ('\n')
*/

enum E_SHOW_MODE
{
   eSM_VAR_LINE         = 0,
   eSM_VAR_COLUMN       = 1,
   eSM_VAR_LENGTH       = 2,
   eSM_VAR_FILE         = 3,
   eSM_VAR_WHERE        = 4,
   eSM_VAR_PUT          = 5,
   eSM_VAR_FRAME        = 6,
   eSM_VAR_LIGHT        = 7
};

DWORD TimeStamp; // Assignment time

int   BufferBeg   = 0; // 1st line in buffer
int   BufferEnd   = 0; // Last line in buffer
int   BufferTop   = BUFFER_SIZE; // Top of buffer

int   CurLineBeg  = 0; // Current line index
int   CurLineEnd  = 0; // Its end
int   CurLineOffs = 0; // Bytes from start one

int   FileNo      = 0; // File name argv index
int   LineNo      = 0; // In the current file

int   ExtPatError = 0; // Error in ext.pattern
short RefToExtPat = 0; // Flag for errors
short Terminal    = 1; // Terminal type
char* FileName    = "<stream>"; // Current file name

struct POSTPONED
{
   DWORD Assign;
   DWORD Offset;
   DWORD Length;
};

POSTPONED Postponed[NUMBER_OF_VARIABLES * 2];

char  Buffer[BUFFER_SIZE];
char  Pattern[MAX_PATTERN_LENGTH] = "\
\n \
\n \
\n \
\n \
\nPATTERN MATCHING UTILITY version 1.1 Copyright (c) 1993 Dmitry A.Kazakov\
\nPattern matching utility comes with ABSOLUTELY NO WARRANTY. This is free\
\nsoftware, and you are welcome to redistribute it under certain conditions.\
\n \
\nUsage : match <pattern> <file1> <file2> ... - match files \
\n match <pattern> - match standard input \
\n match ^ <file1> <file2> ... - pattern from standard input\
\n \
\nThe utility tries to match the pattern with the content of given files. \
\nOn successful matching the process continues just from the next BYTE of a \
\nfile. On failure one line is skipped. Matchig algorithm allows one to use \
\nrecoursive patterns and ones matching several lines of a file. \
\n A lot of embedded patterns are provided. For example, pattern C_COM \
\nmatches a C's comment. Besides, frequently used patterns can be kept in \
\nenvironmental variables. \
\n There is a set of predefined variables. Such a variable can be assigned \
\nduring matching process. For example, LIGHT=C_COM causes output of current\
\nline with highlighting its part matched by the C_COM pattern. \
\n As a pattern you may specify the circumflex (^). In this case it is \
\nassumed that actual pattern comes from the standard input. Line margins \
\nare ignored. \
\n To obtain internal representation of a pattern you can use the following \
\n command : match <pattern> ^ \
\n";

/*>>>>>>>>>>>>>>>>>>>>> Display control <<<<<<<<<<<<<<<<<<<<<<*/
struct MATCH_DISPLAY
{
   // How to turn the inversed mode
   short          OnSize;
   char*          On;
   short          OffSize;
   char*          Off;
};

MATCH_DISPLAY  Display[NUMBER_OF_DISPLAYS + 1]                 =
{
   { 0, NULL, 0, NULL }, // An unknown terminal
   { 4, "\033[7m", 3, "\033[m" } // vt100 family

};

/*>>>>>>>>>>>>>>>>>>>>>> Embedded patterns <<<<<<<<<<<<<<<<<<<<<<

Name Matches

c_id C identifier
c_com C comment
cpp_com C++ comment
c_str C quoted string ".."
c_chr C character literal '..'
c_op Any sequence of C statements limited by an
unbalanced bracket, comma, or semicolon
cpp_op The same as c_op but recognizes C++ comments
c_blank Spaces, tabs, line feeds and C comments
cpp_blank Same for C++


C identifier :
{a..Z_} ${a..Z_0123456789}:

*/
unsigned char  c_id[27] =
{
   0xCE, 0x08, 0x08, 0xfe, 0xff, 0xff, 0x87, 0xfe,
   0xFF, 0xff, 0x07, 0xfc, 0xce, 0x06, 0x0a, 0xff,
   0x03, 0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff,
   0x07, 0xf9, 0xfd
};

/*
C comment :
'^o*'*(%|/):'*^o'
*/
unsigned char  c_com[12] =
{
   0x2F, 0x2a, 0xfb, 0xff, 0xc1, 0xfe, 0xcb, 0xfd,
   0xF9, 0x2a, 0x2f, 0xfd
};

/*
C++ comment :
'^o*'*(%|/):'*^o' | '^o^o'$%:[/]
*/
unsigned char  cpp_com[22] =
{
   0x2F, 0x2a, 0xfb, 0xff, 0xc1, 0xfe, 0xcb, 0xfd,
   0xF9, 0x2a, 0x2f, 0xfe, 0x2f, 0x2f, 0xfc, 0xc1,
   0xF9, 0xff, 0xcb, 0xfe, 0xfd, 0xfd
};

// C string literal: '"' *('\"'|'\\'|'\'end/|%): '"'
unsigned char  c_str[18] =
{
   0x22, 0xfb, 0xff, 0x5c, 0x22, 0xfe, 0x5c, 0x5c,
   0xFE, 0x5c, 0xc0, 0xcb, 0xfe, 0xc1, 0xfd, 0xf9,
   0x22, 0xfd
};

// C character literal: "'"("\''"|*%:"'")
unsigned char  c_chr[12] =
{
   0x27, 0xff, 0x5c, 0x27, 0x27, 0xfe, 0xfb, 0xc1,
   0xf9, 0x27, 0xfd, 0xfd
};

/*
C operator sequence:
item> ( * ( blank
| character $character:
| '('item')'
| '{'item'}'
| '['item']'
| '^o*'*(%|/):'*^o'
| '"' *('\"'|'\\'|'\'end/|%):'"'
| "'\''"|"'"*%:"'"
| %
| /
) :
^^{,;)]^=}
)
*/
unsigned char  c_op[98] =
{
   0xFF, 0xfb, 0xff, 0xc2, 0xfe, 0xc7, 0xfc, 0xc7,
   0xF9, 0xfe, 0x28, 0xf5, 0x21, 0,    0,    0,
   0x29, 0xfe, 0x7b, 0xf5, 0x31, 0,    0,    0,
   0x7D, 0xfe, 0x5b, 0xf5, 0x41, 0,    0,    0,
   0x5D, 0xfe, 0x2f, 0x2a, 0xfb, 0xff, 0xc1, 0xfe,
   0xCB, 0xfd, 0xf9, 0x2a, 0x2f, 0xfe, 0x22, 0xfb,
   0xFF, 0x5c, 0x22, 0xfe, 0x5c, 0x5c, 0xfe, 0x5c,
   0xC0, 0xcb, 0xfe, 0xc1, 0xfd, 0xf9, 0x22, 0xfe,
   0x27, 0x5c, 0x27, 0x27, 0xfe, 0x27, 0xfb, 0xc1,
   0xF9, 0x27, 0xfe, 0xc1, 0xfe, 0xcb, 0xfd, 0xf9,
   0xFA, 0xfa, 0xce, 0x5,  0xb,  0x12, 0,    0x8,
   0x00, 0,    0, 0x20,    0,    0,    0,    0x20,
   0xFD, 0xfd
};

/*
C++ operator sequence:
item> ( * ( blank
| character $character:
| '('item')'
| '{'item'}'
| '['item']'
| '^o*'*(%|/):'*^o'
| '^o^o'*%: end/
| '"' *('\"'|'\\'|'\'end/|%):'"'
| "'\''"|"'"*%:"'"
| %
| /
) :
^^{,;)]^=}
)
*/
unsigned char  cpp_op[106] =
{
   0xFF, 0xfb, 0xff, 0xc2, 0xfe, 0xc7, 0xfc, 0xc7,
   0xF9, 0xfe, 0x28, 0xf5, 0x21, 0,    0,    0,
   0x29, 0xfe, 0x7b, 0xf5, 0x31, 0,    0,    0,
   0x7D, 0xfe, 0x5b, 0xf5, 0x41, 0,    0,    0,
   0x5D, 0xfe, 0x2f, 0x2a, 0xfb, 0xff, 0xc1, 0xfe,
   0xCB, 0xfd, 0xf9, 0x2a, 0x2f, 0xfe, 0x2f, 0x2f,
   0xFB, 0xc1, 0xf9, 0xc0, 0xcb, 0xfe, 0x22, 0xfb,
   0xFF, 0x5c, 0x22, 0xfe, 0x5c, 0x5c, 0xfe, 0x5c,
   0xC0, 0xcb, 0xfe, 0xc1, 0xfd, 0xf9, 0x22, 0xfe,
   0x27, 0x5c, 0x27, 0x27, 0xfe, 0x27, 0xfb, 0xc1,
   0xF9, 0x27, 0xfe, 0xc1, 0xfe, 0xcb, 0xfd, 0xf9,
   0xFA, 0xfa, 0xce, 0x5,  0xb,  0x12, 0,    0x8,
   0x00, 0,    0,    0x20, 0,    0,    0,    0x20,
   0xFD, 0xfd
};

/*
C blank:
$( blank
| '^o*'*(%|/):'*^o'
| end/
) :
*/
unsigned char  c_blank[21] =
{
   0xFC, 0xff, 0xc2, 0xfe, 0x2f, 0x2a, 0xfb, 0xff,
   0xC1, 0xfe, 0xcb, 0xfd, 0xf9, 0x2a, 0x2f, 0xfe,
   0xC0, 0xcb, 0xfd, 0xf9, 0xfd
};

/*
C++ blank :
$( blank
| '^o*'*(%|/):'*^o'
| '^o^o'*%: end[/]
| end/
) :
*/
unsigned char  cpp_blank[32] =
{
   0xFC, 0xff, 0xc2, 0xfe, 0x2f, 0x2a, 0xfb, 0xff,
   0xC1, 0xfe, 0xcb, 0xfd, 0xf9, 0x2a, 0x2f, 0xfe,
   0x2F, 0x2f, 0xfb, 0xc1, 0xf9, 0xc0, 0xff, 0xcb,
   0xFE, 0xfd, 0xfe, 0xc0, 0xcb, 0xfd, 0xf9, 0xfd
};

// The list of embedded patterns
char*    EmbPatterns[NUMBER_OF_PATTERNS] =
{
   (char*) c_id, (char*) c_com, (char*) cpp_com, (char*) c_str, (char*) c_chr, (char*) c_op, (char*) cpp_op, (char*) c_blank, (char*) cpp_blank
};

// The table of embedded pattern names
char*    TableOfPatterns[NUMBER_OF_PATTERNS + 1] =
{
   // Embedded patterns
   "c_id", // C identifier
   "c_com", // C comment
   "cpp_com", // C++ comment
   "c_str", // C string literal
   "c_chr", // C character literal
   "c_op", // C operator sequence
   "cpp_o", // C++ operator sequence
   "c_blank", // C blank text
   "cpp_blank", // C++ blank text
   NULL
};

/*>>>>>>>>>>>>>>>>>>>>> External patterns <<<<<<<<<<<<<<<<<<<<*/

int      NumbOfExtPat = -1;

char*    TableOfExtPatterns[NUMBER_OF_EXT_PATTERNS + 1] =
{
   NULL
};

char*    ExtPatterns[NUMBER_OF_EXT_PATTERNS] =
{
   NULL
};

/*>>>>>>>>>>>>>>>>>>>>> Variables to be assigned <<<<<<<<<<<<<*/

char*    TableOfVarNames[NUMBER_OF_VARIABLES * 4 + 1] =
{
   "line&", // Line number
   "line", // ... then <LF>
   "~line&", // ... show immediately
   "~line", // ... <LF> and immediately
   "column&", // Column number
   "column", "~column&", "~column", "length&", // Length of a matched part of string
   "length", "~length&", "~length", "file&", // Current file name
   "file", "~file&", "~file&", "where&", // File + Line + Column + Length
   "where", "~where&", "~where", "put&", // Output matched part of string
   "put", "~put&", "~put", "frame&", // Output matched string
   "frame", "~frame&", "~frame", "light&", // Highlight matched part of string
   "light", "~light&", "~light", NULL
};

/*>>>>>>>>>>>>>>>>>>>>> Error messages <<<<<<<<<<<<<<<<<<<<<<<*/

struct ERRORS_STRUCT
{
   int   Status;
   char* Message;
};

ERRORS_STRUCT  Errors[NUMBER_OF_ERRORS] =
{
   {  eST_NO_DYNAMIC_MEMORY,        "Dynamic memory overflow"                                      },
   {  eST_WRONG_PATTERN_FORMAT,     "Internal error"                                               },
   {  eST_BRACE_ERROR,              "The right bracket is extra or has wrong type - (] or [)"      },
   {  eST_MISSING_QUOTATION,        "A text literal is not closed - missing right quotation"       },
   {  eST_TOO_BIG_REPEATER,         "Finite repeater is greater than 4,000,000,000"                },
   {  eST_DUPLICATE_LABEL,          "Duplicate definition of a pattern label"                      },
   {  eST_UNRECOGNIZED_CHARACTER,   "Unrecognized character"                                       },
   {  eST_MISSING_RIGHT_BRACE,      "Missing one or more right brackets"                           },
   {  eST_UNRECOGNIZED_KEYWORD,     "Unrecognized keyword (there is no such pattern)"              },
   {  eST_TOO_LARGE_PATTERN,        "Specified pattern is too long"                                },
   {  eST_TOO_LARGE_STRING,         "You cannot match more than 1 Gb"                              },
   {  eST_CANNOT_RETURN,            "Too large piece matched - cannot return back"                 },
   {  eST_UNDEFINED_VARIABLE,       "Unrecognized keyword before equation (=)"                     },
   {  eST_POSSIBLE_INDEFINITE_LOOP, "A pattern repeated by $ or * should not match empty string"   },
   {  eST_RESERVED_KEYWORD,         "You cannot use the atom name as a label"                      }
};

/*>>>>>>>>>>>>>>>>>>>>> Subprograms start here <<<<<<<<<<<<<<<*/

// ShowError - Show error message
void ShowError(int Status)
{
   int   ErrorNo;

   for (ErrorNo = 0; ErrorNo < NUMBER_OF_ERRORS; ErrorNo++)
   {
      if (Status == Errors[ErrorNo].Status)
      {
         fprintf(stderr,Errors[ErrorNo].Message);
         return;
      }
   }

   fprintf(stderr,"Internal error");
}

// FindName - Search for a name in the keyword table

int FindName(char* Name,int Length,char* Table[])
{
   char*    With = NULL;
   char*    Text = NULL;

   int      Count = 0;
   int      Item  = 0;

   while (With = Table[Item++])
   {
      Text = Name;

      for (Count = Length; Count > 0; --Count)
      {
         if (!*With || tolower(*Text++) != *With++)
         {
            goto Go;
         }
      }

      if (!*With)
      {
         return (Item);
      }

      Go:
      continue;
   }

   return 0;
}

// NextLine - Get the next line of input file
int NextLine(const char** Line,int* Length)
{
   int   Index = BufferEnd;
   int   Code  = LF;

   short    Crossed = 0;

   CurLineOffs += CurLineEnd - CurLineBeg + 1;

   if (CurLineEnd != BufferEnd)
   {
      if (CurLineEnd == BufferTop)
      {
         /* Go ahead buffer top */
         CurLineEnd = 0;
      }

      CurLineBeg = ++CurLineEnd;

      while ((CurLineEnd != BufferEnd) && (CurLineEnd != BufferTop) && (Buffer[CurLineEnd] != LF))
      {
         ++CurLineEnd;
      }

      *Line = &Buffer[CurLineBeg];
      *Length = CurLineEnd - CurLineBeg;

      ++LineNo;

      return 1;
   }

   if (feof(stdin))
   {
      // Restore CurLineOffs and cry ...
      CurLineOffs -= CurLineEnd - CurLineBeg + 1;
      return 0;
   }

   ++LineNo;

   CurLineBeg = BufferEnd;

   do
   {
      if (Index >= BUFFER_SIZE)
      {
         char* From  = &Buffer[BufferEnd];
         char* To    = &Buffer[0];

         if (Crossed)
         {
            goto TooLongLine;
         }

         ++Crossed;

         while (From < &Buffer[BUFFER_SIZE])
         {
            *To++ = *From++;
         }

         Index = To - &Buffer[0];
         CurLineBeg = 0;
      }

      Buffer[Index++] = (char) Code;
   }
   while (EOF != (Code = fgetc(stdin)) && LF != Code);

   // if (Code == EOF) Buffer [Index - 1] = LF;

   // Management of the buffer pointers

   if (Crossed)
   {
      /* Buffer top crossed, new line goes from Buffer [0] */
      if (Index >= BufferEnd)
      {
         goto TooLongLine;
      }

      /*
      Two cases are possible. The first one is:

      New line> damaged by new line
      [XXXXXX XXXXXXXXXXXX ]
      BufferEnd| BufferBeg| BufferTop|

      Assuming the rest of buffer is free we will have got the second
      case when BufferBeg is less than BufferEnd:

      New line>
      [XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ]
      BufferBeg| BufferEnd|BufferTop

      We must reset BufferBeg to the 1st LF in X shadowed field.
      */

      BufferBeg = Index + 1; // At least one BYTE difference

      while (BufferBeg < BufferEnd && Buffer[BufferBeg] != LF)
      {
         ++BufferBeg;
      }

      if (BufferBeg >= BufferEnd)
      {
         // Just new line in the buffer
         BufferBeg = 0;
         BufferTop = Index;
         BufferEnd = Index;
      }
      else
      {
         // Several lines in the buffer
         BufferTop = BufferEnd;
      }
   }
   else
   {
      // The New line goes from Buffer [BufferEnd]
      if (BufferBeg > BufferEnd)
      {
         /*
         New line>
         [XXXXXX XXXXXXXXXXXX ]
         BufferEnd| BufferBeg| BufferTop|
         We should know where the new line finishes.
         */
         if (Index >= BufferBeg)
         {
            /*
            New line------->
            [XXXXXX XXXXXXXXXXXX ]
            BufferEnd| BufferBeg| BufferTop|
            We must reset BufferBeg to the 1st LF in X shadowed field.
            */
            BufferBeg = Index + 1; /* At least one BYTE */

            while (BufferBeg < BufferTop && LF != Buffer[BufferBeg])
            {
               ++BufferBeg;
            }

            if (BufferBeg >= BufferTop)
            {
               BufferTop = Index;
               BufferBeg = 0;

               while (BufferBeg < BufferEnd && LF != Buffer[BufferBeg])
               {
                  ++BufferBeg;
               }

               if (BufferBeg == BufferEnd)
               {
                  BufferBeg = CurLineBeg; /* Just new line */
               }
            }
         }
      }
      else
      {
         /*
         New line------->
         [XXXXXXXXXXXXXXX ]
         BufferBeg| BufferEnd|BufferTop

         We must reset BufferTop to the new top of buffer.
         */

         BufferTop = Index;
      }
   }

   BufferEnd = Index;
   CurLineEnd = Index;
   CurLineBeg++;
   *Line = &Buffer[CurLineBeg];
   *Length = CurLineEnd - CurLineBeg;

   return 1;

   TooLongLine:

   fprintf(stderr,"\nLine %d is too large. Is it really a text file?\n",LineNo);
   exit(eME_ERR_FATAL);

   return 0; // To avoid warning message
}

// PrevLine - Return to the previous line of input file
int PrevLine(const char** Line,int* Length)
{
   if (CurLineBeg == (BufferBeg + 1))
   {
      // Cannot return to that line
      return 0;
   }

   if (!(--CurLineBeg))
   {
      CurLineBeg = BufferTop;
   }

   CurLineEnd = CurLineBeg;

   while (Buffer[--CurLineBeg] != LF)
   {
      ;}

   *Line = &Buffer[++CurLineBeg];
   *Length = CurLineEnd - CurLineBeg;

   CurLineOffs -= *Length + 1;

   --LineNo;

   return 1;
}

/* ******************************************************************** **
** @@ PatternMatcher::ExtPat()
** @  Copyrt:
** @  Author:
** @  Modify:
** @  Update:
** @  Notes : ExtPat - Gives reference to an external pattern
** ******************************************************************** */

char* PatternMatcher::ExtPat(char* Name,int Length)
{
   // Embedded pattern ?
   int   PatternIndex   = FindName(Name,Length,TableOfPatterns);

   if (PatternIndex)
   {
      ++RefToExtPat;

      return EmbPatterns[--PatternIndex];
   }

   // External pattern ?
   PatternIndex = FindName(Name,Length,TableOfExtPatterns);

   if (PatternIndex)
   {
      ++RefToExtPat;

      return ExtPatterns[--PatternIndex];
   }

   // Create new external pattern description
   TableOfExtPatterns[++NumbOfExtPat] = (char *) malloc(Length);

   if ((NumbOfExtPat >= NUMBER_OF_EXT_PATTERNS) || !TableOfExtPatterns)
   {
      return NULL;
   }

   strncpy(TableOfExtPatterns[NumbOfExtPat],Name,Length);

   ExtPatterns[NumbOfExtPat] = getenv(TableOfExtPatterns[NumbOfExtPat]);

   if (!ExtPatterns[NumbOfExtPat])
   {
      ExtPatError = eST_UNRECOGNIZED_KEYWORD;
      return NULL;
   }

   ExtPatterns[NumbOfExtPat] = (char*)Compile(ExtPatterns[NumbOfExtPat]);

   if (!ExtPatterns[NumbOfExtPat])
   {
      ExtPatError = _MatchError;
      return NULL;
   }

   return ExtPatterns[NumbOfExtPat];
}

// VariableId - Gives Id of a predefined variable
DWORD VariableId(char* Name,int Length)
{
   ++RefToExtPat;

   return (DWORD) FindName(Name,Length,TableOfVarNames);
}

/* ******************************************************************** **
** @@ OutPut()
** @ Copyrt :
** @ Author :
** @ Modify :
** @ Update :
** @ Notes : OutPut - Flushes matched part of input file
** ******************************************************************** */

void OutPut(DWORD Offset,DWORD Length,int Variable)
{
   const char* Line        = &Buffer[CurLineBeg];

   int         Counter     = 0;
   int         Error       = 0;

   int         LineLength  = CurLineEnd - CurLineBeg;

   int         StrtLineNo  = 0;
   int         StrtColNo   = 0;

   E_SHOW_MODE eMode       = (E_SHOW_MODE) (Variable >> 2);

   int         Index       = 0;
   int         From        = 0;

   while (CurLineOffs > (int) Offset)
   {
      if (!PrevLine(&Line,&LineLength))
      {
         printf("< Can't return to a line where this pattern was matched >\n");
         ++Error;
         break;
      }

      ++Counter;
   }

   if (!Error)
   {
      switch (eMode)
      {
         case eSM_VAR_LINE:
            // Show the location
         case eSM_VAR_COLUMN:
         case eSM_VAR_WHERE:
            {
               StrtLineNo = LineNo;
               StrtColNo = Offset - CurLineOffs + 1;
               break;
            }
         case eSM_VAR_FRAME:
            // Show the whole line
         case eSM_VAR_LIGHT:
            {
               for (From = CurLineBeg; From < (CurLineBeg + (int) Offset - CurLineOffs); ++From)
               {
                  putchar(Buffer[From]);
               }

               if (eMode == eSM_VAR_LIGHT)
               {
                  /* Turn the inversed mode on */
                  for (From = 0; From < Display[Terminal].OnSize; ++From)
                  {
                     putchar(Display[Terminal].On[From]);
                  }
               }
            }
         case eSM_VAR_PUT:
            // Show the matched text
            {
               From = CurLineBeg + Offset - CurLineOffs;

               for (; Length; --Length)
               {
                  // Something to output
                  if (From >= CurLineEnd)
                  {
                     // End of a line
                     putchar(LF);

                     if (NextLine(&Line,&LineLength))
                     {
                        --Counter;
                     }

                     From = CurLineBeg;
                  }
                  else
                  {
                     // Here is a character to output
                     putchar(Buffer[From++]);
                  }
               }

               if (eMode == eSM_VAR_LIGHT)
               {
                  // Turn the inversed mode off
                  for (Index = 0; Index < Display[Terminal].OffSize; ++Index)
                  {
                     putchar(Display[Terminal].Off[Index]);
                  }
               }

               if (eMode != eSM_VAR_PUT)
               {
                  // Show the rest of the line
                  while (From < CurLineEnd)
                  {
                     putchar(Buffer[From++]);
                  }
               }

               break;
            }
      }
   }

   while (Counter--)
   {
      NextLine(&Line,&LineLength);
   }

   if (Error)
   {
      return;
   }

   switch (eMode)
   {
      case eSM_VAR_WHERE:
         {
            printf("In %s:%d.%d",FileName,StrtLineNo,StrtColNo);

            if (StrtLineNo != LineNo)
            {
               printf("/%d",LineNo);
            }

            putchar(' ');
            break;
         }
      case eSM_VAR_FILE:
         {
            printf("File %s ",FileName);
            break;
         }
      case eSM_VAR_LINE:
         {
            if (StrtLineNo == LineNo)
            {
               printf("Line %d ",LineNo);
            }
            else
            {
               printf("Lines %d-%d ",StrtLineNo,LineNo);
            }

            break;
         }
      case eSM_VAR_COLUMN:
         {
            printf("Colunn %d ",StrtColNo);
            break;
         }
      case eSM_VAR_LENGTH:
         {
            printf("%d BYTE",Length);

            if (Length != 1)
            {
               putchar('s');
            }

            putchar(' ');

            break;
         }
   }

   if (Variable & 1)
   {
      putchar(LF);
   }
}

// Assign - Assigns matched part of input file to a predefined variable
void Assign(DWORD Id,DWORD Offset,DWORD Length)
{
   int   Variable = Id - 1;

   if (Variable & 2)
   {
      OutPut(Offset,Length,Variable);
   }
   else
   {
      Variable = (Variable >> 1) + (Variable & 1);
      Postponed[Variable].Assign = TimeStamp++;
      Postponed[Variable].Offset = Offset;
      Postponed[Variable].Length = Length;
   }
}

// DeAssign - Discards effect of the Assign
void DeAssign(DWORD Id)
{
   int   Var   = Id - 1;

   if (!(Var & 2))
   {
      Postponed[(Var >> 1) + (Var & 1)].Assign = 0;
   }
}

/* ******************************************************************** **
** @@ main()
** @ Copyrt :
** @ Author :
** @ Modify :
** @ Update :
** @ Notes :
** ******************************************************************** */

 #if 0
int main(int argc,char** argv)
{
   long        When     = 0;
   long        WhenThis = 0;
   long        WhenLast = 0;
   int         Index    = 0;
   int         Status   = 0;
   int         Size     = 0;
   int         Length   = 0;
   int         Old      = 0;
   int         Variable = 0;
   int         Pointer  = 0;
   short       ShowNow  = 0;

   const char* Line     = NULL;

   if (argc < 2)
   {
      goto ShowUsage;
   }

   // Translate user pattern
   GetNextLine        = NextLine;
   GetPreviousLine    = PrevLine;
   GetVariableId      = VariableId;
   GetExternalPattern = ExtPat;
   AssignVariable     = Assign;
   DeAssignVariable   = DeAssign;
   UserAlphas         = "_~&";   // Assume _~& are letters

   Size = MAX_PATTERN_LENGTH;

   Index = 0;

   if (argv[1][0] == '^' && !argv[1][1])
   {
      // Get the pattern from the stdin
      Length = 0;

      while ((Status = fgetc(stdin)) != EOF)
      {
         if (Status != LF)
         {
            // Ignore LF's
            if (Length == BUFFER_SIZE)
            {
               // Error will appear later
               break;
            }
            else
            {
               Buffer[Length++] = (char) Status;
            }
         }
      }

      Line = &Buffer[0];
   }
   else
   {
      // Get the pattern from the command line
      Length = strlen(argv[1]);
      Line = argv[1];
   }

   Status = Translate(Length,Line,&Index,Pattern,&Size);

   if (Status != eST_SUCCESS)
   {
      goto TranslationError;
   }

   if (Index != Length)
   {
      Status = eST_UNRECOGNIZED_CHARACTER;
      goto TranslationError;
   }

   if ((argc == 3) && (argv[2][0] == '^') && !argv[2][1])
   {
      if (RefToExtPat)
      {
         // Just flush the internal representation of the pattern
         fprintf(stderr,"\nThis pattern cannot be outlined because it refers to an external one\n");
         return eME_ERR_SYNTAX;
      }

      Length = 0;

      printf("/*\n\tInternal representation for pattern:\n\t%s\n*/\n{",Line);

      for (Index = 0; Index < Size; ++Index)
      {
         if (!Length)
         {
            printf("\n ");
            Length = 10;
         }

         Length--;

         printf("%#4x%c ",(unsigned char)Pattern[Index],Index == (Size - 1) ? ' ' : ',');
      }

      printf("\n}\n");

      return eME_ERR_NONE;
   }

   if (argc >= 3)
   {
      // Source files from the command line
      FileNo = 2;
   }

   GetInputFile:

   if (FileNo)
   {
      FileName = argv[FileNo];

      if (!freopen(FileName,"rt",stdin))
      {
         fprintf(stderr,"\nCannot open input file (%s)\n",FileName);
         return eME_ERR_FATAL;
      }
   }

   // Match the input file

   if (!NextLine(&Line,&Length))
   {
      goto Done;
   }

   Index = 0;

   while (true)
   {
      CurLineOffs = 0;

      TimeStamp = 1;

      for (Variable = 0; Variable < (NUMBER_OF_VARIABLES * 2); ++Variable)
      {
         Postponed[Variable].Assign = 0;
      }

      Old = Pointer;

      Status = match(Length,Line,&Pointer,Pattern);

      switch (Status)
      {
         case eST_SUCCESS:
            {
               Pointer -= CurLineOffs; // From the next character
               Line = &Buffer[CurLineBeg];
               Length = CurLineEnd - CurLineBeg;
               WhenLast = 0;

               while (true)
               {
                  // Sort variables according to assignment order
                  WhenThis = 0;

                  for (Variable = 0; Variable < NUMBER_OF_VARIABLES * 2; ++Variable)
                  {
                     When = Postponed[Variable].Assign;

                     if ((When > WhenLast) && (!WhenThis || (When < WhenThis)))
                     {
                        WhenThis = When;
                        ShowNow = (short) Variable;
                     }
                  }

                  if (!WhenThis)
                  {
                     break;
                  }

                  OutPut(Postponed[ShowNow].Offset,Postponed[ShowNow].Length,((ShowNow & 1) ? ((ShowNow - 1) << 1) + 1 : (ShowNow << 1)));

                  WhenLast = WhenThis;
               }

               if ((Old == Pointer) && (Pointer < Length))
               {
                  // Null string was successfully matched
                  printf("< Null >\n");
                  ++Pointer;
               }

               if (Pointer >= Length)
               {
                  // To avoid a loop
                  if (!NextLine(&Line,&Length))
                  {
                     goto Done;
                  }

                  Pointer = 0; // From the beginning
               }

               break;
            }
         case eST_FAILURE:
            {
               while (CurLineOffs)
               {
                  if (!PrevLine(&Line,&Length))
                  {
                     Status = eST_CANNOT_RETURN;
                     goto MatchingError;
                  }
               }

               if (!NextLine(&Line,&Length))
               {
                  goto Done;
               }

               Pointer = 0; // From the beginning
               break;
            }
         default:
            {
               goto MatchingError;
            }
      }
   }

   Done:

   if (FileNo++)
   {
      // Reinitialize buffer for a new file
      BufferBeg = 0;
      BufferEnd = 0;
      BufferTop = BUFFER_SIZE;
      CurLineBeg = 0;
      CurLineEnd = 0;
      LineNo = 0;

      if (FileNo < argc)
      {
         goto GetInputFile;
      }
   }

   return eME_ERR_NONE;

   MatchingError:

   fprintf(stderr,"\nFailure: ");
   ShowError(Status);
   fprintf(stderr,"\n");

   return eME_ERR_FATAL;

   ShowUsage:

   fprintf(stderr,Pattern);

   return eME_ERR_SYNTAX;

   TranslationError:

   fprintf(stderr,"\n%s",Line);

   for (Size = 0; Size < Index; ++Size)
   {
      switch (*Line++)
      {
         case '\t':
            Pattern[Size] = '\t';
            break;

         default:
            Pattern[Size] = ' ';
            break;
      }
   }

   Pattern[Size++] = '^';
   Pattern[Size]   = 0;

   if (ExtPatError)
   {
      fprintf(stderr,"\n%s\nSyntax in pattern specified by environmental variable:\n",Pattern);
      ShowError(ExtPatError);
   }
   else
   {
      fprintf(stderr,"\n%s\nSyntax: ",Pattern);
      ShowError(Status);
   }

   fprintf(stderr,"\n");

   return eME_ERR_SYNTAX;
}
#endif

/* ******************************************************************** **
** @@ PatternMatcher::GetStatus()
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

E_STATUS_CODE PatternMatcher::GetStatus()
{
   return _MatchError;   
}

/* ******************************************************************** **
** End of File
** ******************************************************************** */
