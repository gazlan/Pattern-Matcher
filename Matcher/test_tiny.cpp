// xt - eXTractor | Tiny Test

/*
TESTING pattern type Copyright (c) Dmitry A. Kazakov St.Petersburg
(C++) Spring 1993
Very little test for pattern type
*/

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "pattern_matcher.h"

int main()
{
   char*    pszText = "This string contains several words";

   PatternMatcher    PM;

   int   Index  = 0;
   int   Length = 0;

   int      iTextSize = strlen(pszText);
   
   BYTE*    pWord  = (BYTE*)PM.Compile("$Letter:");
   BYTE*    pBlank = (BYTE*)PM.Compile("Blank");

   Index  = 0;
   Length = strlen("abcd");

   if (!(PM.Match("abcd",Length,&Index,pWord) && (Index == Length)))
   {
      printf("\n\a[E]: Error on [abcd]\n");
      return 0;
   }

   Index  = 0;
   Length = strlen("abc+");

   if (PM.Match("abc+",strlen("abc+"),&Index,pWord) && (Index == Length))
   {
      printf("\n\a[E]: Error on [abc+]\n");
      return 0;
   }

   int   Count = 0;

   Index = 0;

   do
   {
      PM.Match(pszText,iTextSize,&Index,pWord);
      PM.Match(pszText,iTextSize,&Index,pBlank);

      ++Count;
   }
   while (PM.GetStatus() == eST_SUCCESS);

   free(pWord);
   free(pBlank);

   const Pattern     ptWord (PM,"$Letter:"); // Matches a word
   const Pattern     ptBlank(PM,"Blank");    // Match spaces

   if (!("abcd" |= ptWord) || ("abc+" |= ptWord))
   {
      printf("\n\a[E]: Error in |=\n");
      return 0;
   }
   
   Index = 0;
   Count = 0;

   do
   {
      Index += &pszText[Index] >> ptWord;  // Matches a word
      Index += &pszText[Index] >> ptBlank; // Match spaces  

      ++Count; 
   }  
   while (PM.GetStatus() == eST_SUCCESS);

   if (Count != 5)
   {
      printf("\n\a[E]: Error in >>\n");
      return 0;
   }

   if (Count != 5)
   {
      printf("\n\a[E]: Error in >>\n");
      return 0;
   }

   printf("\n[I]: ALL OK\n");

   return 0;
}
