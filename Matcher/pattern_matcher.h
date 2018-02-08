/* ******************************************************************** **
** @@ PatternMatcher
** @  Copyrt : 
** @  Author : 
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

#ifndef __PATTERN_MATCHER_HPP__
#define __PATTERN_MATCHER_HPP__

#if _MSC_VER > 1000
#pragma once
#endif

/* ******************************************************************** **
** @@ internal defines
** ******************************************************************** */

enum E_STATUS_CODE
{
   eST_FAILURE                  = 0,
   eST_SUCCESS                  = 1,
   eST_WRONG_PATTERN_FORMAT     = 2,
   eST_NO_DYNAMIC_MEMORY        = 3,
   eST_BRACE_ERROR              = 4,
   eST_MISSING_QUOTATION        = 5,
   eST_TOO_BIG_REPEATER         = 6,
   eST_DUPLICATE_LABEL          = 7,
   eST_UNRECOGNIZED_CHARACTER   = 8,
   eST_MISSING_RIGHT_BRACE      = 9,
   eST_UNRECOGNIZED_KEYWORD     = 10,
   eST_TOO_LARGE_PATTERN        = 11,
   eST_TOO_LARGE_STRING         = 12,
   eST_CANNOT_RETURN            = 13,
   eST_UNDEFINED_VARIABLE       = 14,
   eST_POSSIBLE_INDEFINITE_LOOP = 15,
   eST_RESERVED_KEYWORD         = 16,
   eST_INTERNAL_ERROR           = 17
};

/* ******************************************************************** **
** @@ internal prototypes
** ******************************************************************** */

/* ******************************************************************** **
** @@ external global variables
** ******************************************************************** */

/* ******************************************************************** **
** @@ static global variables
** ******************************************************************** */

/* ******************************************************************** **
** @@ Global Function Prototypes
** ******************************************************************** */

class PatternMatcher
{
   private:

      char*             _pszUserAlphas;
//      E_STATUS_CODE     _MatchError;

   public:
            // !!! TEMPORARY !!!
      int               _iUserBase;

   public:

       PatternMatcher();
      ~PatternMatcher();

      int   Translate(const char* const pszText,int iSize,int* piPointer,BYTE* pPattern,int* piSize);
      int   Match    (const char* const pszText,int iSize,int* piPointer,BYTE* pPattern);
      
      const char* const    Init   (const char* const pszText,int iSize,int* piPointer);
      const BYTE* const    Compile(const char* const pszExpression);

      E_STATUS_CODE     GetStatus();

      // Callback
/*
      int     (*GetNextLine)       (const char** Line,int* Length);
      int     (*GetPreviousLine)   (const char** Line,int* Length);
      char*   (*AllocatePattern)   (DWORD Size);
      char*   (*GetExternalPattern)(char* Name,int Length);
      DWORD   (*GetVariableId)     (char* Name,int Length);
      void    (*AssignVariable)    (DWORD VariableId,DWORD Offset,DWORD Length);
      void    (*DeAssignVariable)  (DWORD VariableId);
*/
      private:

         void     Free();
         BYTE*    MatchKeyWord(BYTE* String,BYTE* Str_End,BYTE* Text);
         DWORD    First_pass (BYTE* Str_ptr,BYTE* Str_End,BYTE** New_Str_ptr,E_STATUS_CODE* Error_code,BYTE** Error_ptr);
         int      Second_pass(BYTE* Str_ptr,BYTE* Str_End,BYTE* Pat_ptr,E_STATUS_CODE* Error_code,BYTE** Error_ptr);
         char*    ExtPat(char* Name,int Length);
};

/* ******************************************************************** **
** @@ class Pattern
** @  Copyrt: Copyright (c) Dmitry A. Kazakov, 2001
** @  Author: Dmitry A. Kazakov * mailbox@dmitry-kazakov.de
** @  Modify:
** @  Update:
** @  Notes : http://www.dmitry-kazakov.de/match/match_1_1.tgz
** @  Notes : The only member is protected field Body. It contains the pointer  
** @  Notes : to the internal pattern representation.                          
** @  Notes : The pattern body is allocated when an object of the pattern class
** @  Notes : is created and released when the object is removed.              
** ******************************************************************** */

class Pattern
{
   private:

      const BYTE*             _pBody; // Pointer to the internal representation
      const PatternMatcher&   _rPM;

   public:

   // Constructor - From string
   // String - To be translated
   Pattern(PatternMatcher& rPM,const char* const pszPattern) 
   :  _rPM(rPM),_pBody(rPM.Compile(pszPattern))
   {
   }

   // Destructor
   ~Pattern()
   {
      if (_pBody)
      {
         free((void*)_pBody);
         _pBody = NULL;
      }
   }

   // operator |= - Match the whole string against the pattern
   // pszText - Pointer to the string to be matched
   // Pattern - Pattern
   // Returns:
   // [0] The pattern does not match the whole string
   // [1] The pattern matches the whole string
   friend int operator |= (const char* const pszText,const Pattern& rPattern);

   // operator >> - Match a part of the string against the pattern
   // pszText - Pointer to the string to be matched
   // Pattern - Pattern
   // Returns:
   // Number of matched string bytes
   //
   friend int operator >> (const char* const pszText,const Pattern& rPattern);

   // Binary - The internal pattern representation
   // Returns:
   // Pointer to the string representing the pattern
   const BYTE* const Binary()
   {
      return _pBody;
   }

   private:

   // Preventing automatic copying of a pattern ...
   // Declared but not defined
   Pattern(const Pattern&); 
   // Declared but not defined
   void operator = (const Pattern&); 
};

//////////////////////////////////////////////////////////////////////////
inline int operator |= (const char* pszText,const Pattern& rPattern)
{
   int   Index  = 0;
   int   Length = strlen(pszText);

   Pattern&          rPat = (Pattern&)rPattern; 
   PatternMatcher    rPm  = (PatternMatcher&)rPat._rPM;

   bool  bExist  = rPat.Binary() != NULL;
   bool  bResult = rPm.Match(pszText,Length,&Index,(BYTE*)rPat.Binary()) == eST_SUCCESS;

   return bExist && bResult && (Length == Index);
}
//////////////////////////////////////////////////////////////////////////
inline int operator >> (const char* pszText,const Pattern& rPattern)
{
   // Matching of string prefix returns number of matched characters
   int   Index = 0;

   Pattern&          rPat = (Pattern&)rPattern; 
   PatternMatcher    rPm  = (PatternMatcher&)rPat._rPM;

   if (rPat.Binary())
   {
      // Here is a body
      rPm.Match(pszText,strlen(pszText),&Index,(BYTE*)rPat.Binary());
   }

   return Index;
}
//////////////////////////////////////////////////////////////////////////

#endif

/* ******************************************************************** **
** End of File
** ******************************************************************** */
