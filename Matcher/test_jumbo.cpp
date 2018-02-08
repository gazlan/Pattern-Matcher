/*
TESTING pattern matching Copyright (c) Dmitry A. Kazakov
St.Petersburg
(C, ANSI C) Spring, 1993
*/

/*
This file contains some tests for match and patran functions. For
such kind of sophisticated code like pattern matching functions are
one needs many tests to be sure that all works well. It's hard to say
how many tests are necessary to cover all possible errors, hundreds
or maybe thousands?

Usage :

matest SILENCE
or
matest [<start with test no.> [<repeat on error>]]
*/

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
#include <stdio.h>
#include <string.h>
*/
// #include "match.h"
#include "pattern_matcher.h"

#define STARTING_TEST   1
#define REPEAT_ON_ERROR 0

struct TEST_STRUCT
{
   char*       TestId;
   char*       Pattern;
   int         Patran_ptr;
   int         Patran_status;
   char*       String;
   int         Match_ptr;
   int         Match_status;
};

#define TOTAL_TESTS           (85)

TEST_STRUCT  Test[TOTAL_TESTS] =
{
   /* 01 */ { "Error in literal",                           "'abcd",                                        5,    eST_MISSING_QUOTATION,         "",               0,    0            },
   /* 02 */ { "A lot of braces",                            "([]())",                                       6,    eST_SUCCESS,                   "",               0,    eST_SUCCESS  },
   /* 03 */ { "One more braces",                            "('a'|'b')('c'|'d')",                           18,   eST_SUCCESS,                   "bc",             2,    eST_SUCCESS  },
   /* 04 */ { "Any of",                                     "{abcdef}",                                     8,    eST_SUCCESS,                   "e",              1,    eST_SUCCESS  },
   /* 05 */ { "Any of",                                     "{abcdef}",                                     8,    eST_SUCCESS,                   "x",              0,    eST_FAILURE  },
   /* 06 */ { "Any of",                                     "{}",                                           2,    eST_SUCCESS,                   "e",              0,    eST_FAILURE  },
   /* 07 */ { "Any of",                                     "${a}:",                                        5,    eST_SUCCESS,                   "ax",             1,    eST_SUCCESS  },
   /* 08 */ { "Any of",                                     "${.;,,,}:",                                    9,    eST_SUCCESS,                   ";.,,x",          4,    eST_SUCCESS  },
   /* 09 */ { "Any of",                                     "{abcd()",                                      7,    eST_MISSING_QUOTATION,         "",               0,    0            },
   /* 10 */ { "Skipping alternatives",                      "'a'|'b'|'c'",                                  11,   eST_SUCCESS,                   "a",              1,    eST_SUCCESS  },
   /* 11 */ { "Something new",                              "('a'|'b')^^('c'|'d')",                         20,   eST_SUCCESS,                   "bc",             1,    eST_SUCCESS  },
   /* 12 */ { "Error in braces",                            "(['a')|('b'|))",                               5,    eST_BRACE_ERROR,               "",               0,    0            },
   /* 13 */ { "Error in braces",                            "(('a')|('b'|)",                                13,   eST_MISSING_RIGHT_BRACE,       "",               0,    0            },
   /* 14 */ { "Braces sequence",                            "('a'|'b')('c'|'d')",                           18,   eST_SUCCESS,                   "bc",             2,    eST_SUCCESS  },
   /* 15 */ { "Separated literals",                         "5'a''b'",                                      7,    eST_SUCCESS,                   "aaaaab",         6,    eST_SUCCESS  },
   /* 16 */ { "Noop",                                       "5'' 'b'",                                      7,    eST_SUCCESS,                   "b",              1,    eST_SUCCESS  },
   /* 17 */ { "Giant repeater",                             "(('a'|999999999999900'b'))",                   6,    eST_TOO_BIG_REPEATER,          "",               0,    0            },
   /* 18 */ { "Duplicate label",                            "L1 # L1> LETTER L2 > # L1> ['a']",             23,   eST_DUPLICATE_LABEL,           "",               0,    0            },
   /* 19 */ { "Unrecognized character",                     "L1 (& # L1> LETTER L2 > #)",                   4,    eST_UNRECOGNIZED_CHARACTER,    "",               0,    0            },
   /* 20 */ { "Unrecognized character outside ()",          "'a' & # L1> LETTER L2 > #)",                   4,    eST_SUCCESS,                   "a",              1,    eST_SUCCESS  },
   /* 21 */ { "Unrecognized keyword",                       "L1 (L2 L1>'a')",                               4,    eST_UNRECOGNIZED_KEYWORD,      "",               0,    0            },
   /* 22 */ { "Unrecognized keyword outside ()",            "'a' L1",                                       4,    eST_UNRECOGNIZED_KEYWORD,      "a",              0,    0            },
   /* 23 */ { "Name conflict",                              " L >'a'",                                      1,    eST_RESERVED_KEYWORD,          "",               0,    0            },
   /* 24 */ { "Assignment error",                           "L='a'",                                        0,    eST_UNDEFINED_VARIABLE,        "a",              0,    0            },
   /* 25 */ { "Indefinite loop",                            "*['a']",                                       5,    eST_POSSIBLE_INDEFINITE_LOOP,  "",               0,    0            },
   /* 26 */ { "Indefinite loop",                            "*(Not'a')",                                    8,    eST_POSSIBLE_INDEFINITE_LOOP,  "",               0,    0            },
   /* 27 */ { "Indefinite loop",                            "$(*'a')",                                      6,    eST_POSSIBLE_INDEFINITE_LOOP,  "",               0,    0            },
   /* 28 */ { "Circumflex",                                 "'^A - 1 then ^b - Quote ^o- Circumflex -^'",   42,   eST_SUCCESS,                   "\001 - 1 then \" - Quote /- Circumflex -^", 37, eST_SUCCESS  },
   /* 29 */ { "Select statement",                           "\"abc\"|\"def\"",                              11,   eST_SUCCESS,                   "abc",            3,    eST_SUCCESS  },
   /* 30 */ { "Select statement",                           "\"abc\"|\"def\"",                              11,   eST_SUCCESS,                   "def",            3,    eST_SUCCESS  },
   /* 31 */ { "Select statement",                           "\"abc\"|\"def\"",                              11,   eST_SUCCESS,                   "c",              0,    eST_FAILURE  },
   /* 32 */ { "Square brackets",                            "'a'['b']",                                     8,    eST_SUCCESS,                   "a",              1,    eST_SUCCESS  },
   /* 33 */ { "Square brackets",                            "'a'['b']",                                     8,    eST_SUCCESS,                   "ab",             2,    eST_SUCCESS  },
   /* 34 */ { "Square brackets",                            "'a'['b']",                                     8,    eST_SUCCESS,                   "c",              0,    eST_FAILURE  },
   /* 35 */ { "Case deaf literal",                          "<AbCdEf>",                                     8,    eST_SUCCESS,                   "aBCdef",         6,    eST_SUCCESS  },
   /* 36 */ { "Atom SUCCESS",                               "'a'SUCCESS'b'",                                13,   eST_SUCCESS,                   "a",              1,    eST_SUCCESS  },
   /* 37 */ { "Atom FAILURE",                               "'a'FAILURE LETTER",                            17,   eST_SUCCESS,                   "aaa",            0,    eST_FAILURE  },
   /* 38 */ { "Finite repeater",                            "6 'a'",                                        5,    eST_SUCCESS,                   "aaaaaa",         6,    eST_SUCCESS  },
   /* 39 */ { "Finite repeater",                            "6 (<a>!<b>) <a>",                              15,   eST_SUCCESS,                   "aAaAbBA",        7,    eST_SUCCESS  },
   /* 40 */ { "Finite repeater and select",                 "(3)",                                          3,    eST_SUCCESS,                   "",               0,    eST_SUCCESS  },
   /* 41 */ { "0 as repeatition count",                     "0('a'|'b')'c'",                                13,   eST_SUCCESS,                   "c",              1,    eST_SUCCESS  },
   /* 42 */ { "Finite repeater",                            "6 (<a>!<b>) <a>",                              15,   eST_SUCCESS,                   "aAaABA",         0,    eST_FAILURE  },
   /* 43 */ { "Large finite repeater",                      "71#'end'",                                     8,    eST_SUCCESS,                   "12345678901234567890123456789012345678901234567890123456789012345678901end", 74, eST_SUCCESS  },
   /* 44 */ { "As little as possible and END atom",         "*LETTER END",                                  11,   eST_SUCCESS,                   "word",           4,    eST_SUCCESS  },
   /* 45 */ { "As little as possible with alternatives",    "* ( 'aa' | 'a') 'aa' end",                     24,   eST_SUCCESS,                   "aaa",            3,    eST_SUCCESS  },
   /* 46 */ { "As little as possible failed",               "* UCL LCL LCL",                                13,   eST_SUCCESS,                   "AA1",            0,    eST_FAILURE  },
   /* 47 */ { "As little as possible matches big data",     "*%:'a'",                                       6,    eST_SUCCESS,                   "1234567890123456789012345678901234567890123456789012345678901234567890123456789a", 80, eST_SUCCESS  },
   /* 48 */ { "Ellipsis",                                   "...'a'...'b'",                                 12,   eST_SUCCESS,                   "FIRSTaSECONDb",  13,   eST_SUCCESS  },
   /* 49 */ { "Ellipsis",                                   "...'a'...'b'",                                 12,   eST_SUCCESS,                   "ab",             2,    eST_SUCCESS  },
   /* 50 */ { "Ellipsis",                                   "..'a'..'b'",                                   10,   eST_SUCCESS,                   "FIRSTaSECOND",   0,    eST_FAILURE  },
   /* 51 */ { "Ellipsis under repeater",                    "*('a'...'b'):'c'",                             16,   eST_SUCCESS,                   "aSOMEbaANDbc",   12,   eST_SUCCESS  },
   /* 52 */ { "As much as possible",                        "$CHARACTER DIGIT",                             16,   eST_SUCCESS,                   "a1U2",           4,    eST_SUCCESS  },
   /* 53 */ { "As little as possible optimized with FENCE", "*('a'|'b'):'c'",                               14,   eST_SUCCESS,                   "aababc",         6,    eST_SUCCESS  },
   /* 54 */ { "As little as possible optimized with FENCE", "*('a'|'ab'):'a'end",                           18,   eST_SUCCESS,                   "aba",            0,    eST_FAILURE  },
   /* 55 */ { "As little as possible optimized with FENCE", "(*('a'|'b'):'x'|'ab')",                        21,   eST_SUCCESS,                   "abaaa",          2,    eST_SUCCESS  },
   /* 56 */ { "As much as possible optimized with FENCE",   "$#:'b'",                                       6,    eST_SUCCESS,                   "12345b",         6,    eST_SUCCESS  },
   /* 57 */ { "As much as possible optimized with FENCE",   "$%:'a'",                                       6,    eST_SUCCESS,                   "12345a",         0,    eST_FAILURE  },
   /* 58 */ { "As much as possible optimized with FENCE",   "($#:'x'|'123')",                               14,   eST_SUCCESS,                   "12345b",         3,    eST_SUCCESS  },
   /* 59 */ { "As much as possible optimized with FENCE",   "'12'($#:'x'|'3')",                             16,   eST_SUCCESS,                   "12345b",         3,    eST_SUCCESS  },
   /* 60 */ { "As much as possible optimized with FENCE",   "'/'$(' '):",                                   10,   eST_SUCCESS,                   "/  s",           3,    eST_SUCCESS  },
   /* 61 */ { "Fence",                                      "*%'a':*%'b'",                                  11,   eST_SUCCESS,                   "12a34b",         6,    eST_SUCCESS  },
   /* 62 */ { "Fence",                                      "*%'a':*%'b'",                                  11,   eST_SUCCESS,                   "aaaaa",          0,    eST_FAILURE  },
   /* 63 */ { "Not",                                        "Not ('a'|'b') 'c'",                            17,   eST_SUCCESS,                   "c",              1,    eST_SUCCESS  },
   /* 64 */ { "Not",                                        "^('a'|'b')'c'",                                13,   eST_SUCCESS,                   "bc",             0,    eST_FAILURE  },
   /* 65 */ { "Not",                                        "*DIGIT^DIGIT",                                 12,   eST_SUCCESS,                   "123ddd",         3,    eST_SUCCESS  },
   /* 66 */ { "Not",                                        "*%^^'b'",                                      7,    eST_SUCCESS,                   "aaab",           3,    eST_SUCCESS  },
   /* 67 */ { "NoEmpty",                                    "NoEmpty ['something']",                        21,   eST_SUCCESS,                   "something",      9,    eST_SUCCESS  },
   /* 68 */ { "NoEmpty",                                    "?['something']",                               14,   eST_SUCCESS,                   "other",          0,    eST_FAILURE  },
   /* 69 */ { "Blank atom",                                 "BLANK",                                        5,    eST_SUCCESS,                   "  a",            2,    eST_SUCCESS  },
   /* 70 */ { "Blank atom",                                 "BLANK",                                        5,    eST_SUCCESS,                   "",               0,    eST_FAILURE  },
   /* 71 */ { "End of keyword",                             "<a>_",                                         4,    eST_SUCCESS,                   "a",              1,    eST_SUCCESS  },
   /* 72 */ { "End of keyword",                             "<a>_",                                         4,    eST_SUCCESS,                   "a  ",            3,    eST_SUCCESS  },
   /* 73 */ { "End of keyword",                             "<a>_",                                         4,    eST_SUCCESS,                   "a  a",           3,    eST_SUCCESS  },
   /* 74 */ { "End of keyword",                             "<a>_",                                         4,    eST_SUCCESS,                   "a(",             1,    eST_SUCCESS  },
   /* 75 */ { "End of keyword",                             "<a>_",                                         4,    eST_SUCCESS,                   "a1",             0,    eST_FAILURE  },
   /* 76 */ { "End of keyword",                             "_<a>",                                         4,    eST_SUCCESS,                   "a1",             1,    eST_SUCCESS  },
   /* 77 */ { "End of keyword",                             "<b>_<a>",                                      7,    eST_SUCCESS,                   "b  a",           4,    eST_SUCCESS  },
   /* 78 */ { "End of keyword",                             "<b>_<a>",                                      7,    eST_SUCCESS,                   "ba",             0,    eST_FAILURE  },
   /* 79 */ { "Recoursive Call",                            "Balanced>('{'[Balanced]'}')",                  27,   eST_SUCCESS,                   "{{}}",           4,    eST_SUCCESS  },
   /* 80 */ { "Recoursive Call",                            "Balanced>('{'[Balanced]'}')",                  27,   eST_SUCCESS,                   "{{}{",           0,    eST_FAILURE  },
   /* 81 */ { "Recoursive Call",                            "*%:Balanced>('{'*(Balanced|%):'}')",           34,   eST_SUCCESS,                   "ab{c{de}f{}g}",  13,   eST_SUCCESS  },
   /* 82 */ { "Recoursive Call",                            "item>(*('['item']'|%):^^']')",                 28,   eST_SUCCESS,                   "a[b_c] d] e",    8,    eST_SUCCESS  },
   /* 83 */ { "User Base = 2",                              "$#:",                                          3,    eST_SUCCESS,                   "010112",         5,    eST_SUCCESS  },
   /* 84 */ { "User Base = 16",                             "$#:",                                          3,    eST_SUCCESS,                   "01f19Abcdfg",    10,   eST_SUCCESS  },
   /* 85 */ { "User Base = 11",                             "$#:",                                          3,    eST_SUCCESS,                   "0A/",            2,    eST_SUCCESS  }
};

int main(int argc,char** argv)
{
   PatternMatcher    PM;

   BYTE  pPattern[MAX_PATH];

   int   StartingTest  = STARTING_TEST;
   int   RepeatOnError = REPEAT_ON_ERROR;

   int   Silence = 0;

   if (argc > 1)
   {
      if (argv[1][0] == 'S')
      {
         Silence = 1;
      }

      if (argc > (1 + Silence))
      {
         sscanf(argv[1 + Silence],"%d",&StartingTest);
      }

      if (argc > (2 + Silence))
      {
         sscanf(argv[2 + Silence],"%d",&RepeatOnError);
      }
   }

   memset(pPattern,0,MAX_PATH);

   int   TestNo = 0;

   for (TestNo = StartingTest; TestNo <= TOTAL_TESTS; ++TestNo)
   {
      if (!Silence)
      {
         printf("\n\nTest no %d\t-- %s\n\t%s\n\t%s\n",TestNo,Test[TestNo - 1].TestId,Test[TestNo - 1].Pattern,Test[TestNo - 1].String);
      }

      switch (TestNo)
      {
         case TOTAL_TESTS - 2:
         {
            PM._iUserBase = 2;
            break;
         }
         case TOTAL_TESTS - 1:
         {
            PM._iUserBase = 16;
            break;
         }
         case TOTAL_TESTS - 0:
         {
            PM._iUserBase = 11;
            break;
         }
         default:
         {
            PM._iUserBase = 10;
            break;
         }
      }

      GetPattern:

      int   ptr  = 0;
      int   size = MAX_PATH;

      int   iStatus = PM.Translate(Test[TestNo - 1].Pattern,strlen(Test[TestNo - 1].Pattern),&ptr,pPattern,&size);

      if ((Test[TestNo - 1].Patran_ptr != ptr) || (Test[TestNo - 1].Patran_status != iStatus))
      {
         printf("\nTRANSLATION FAILED!");

         if (RepeatOnError)
         {
            goto GetPattern;
         }
         
         return 1;
      }

      if (iStatus != eST_SUCCESS)
      {
         continue;
      }

      ptr = 0;

      iStatus = PM.Match(Test[TestNo - 1].String,strlen(Test[TestNo - 1].String),&ptr,pPattern);
      
      if ((Test[TestNo - 1].Match_ptr != ptr) || (Test[TestNo - 1].Match_status != iStatus))
      {
         printf("\nMATCHING FAILED for test no.%d!",TestNo);

         if (RepeatOnError)
         {
            goto GetPattern;
         }

         return 1;
      }
   }

//   PM.Free();
   
   if (!Silence)
   {
      printf("\n\nALL TESTS ARE OK !\n\n");
   }

   return 0;
}
