//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jun 29 14:25:53 PDT 2009
// Last Modified: Mon Jun 29 14:26:01 PDT 2009
// Filename:      ...sig/src/sig/PerlRegularExpression.h
// Web Address:   http://sig.sapp.org/src/sig/PerlRegularExpression.h
// Syntax:        C++; Perl Compatible Regular Expressions (http://www.pcre.org)
//
// See also:      http://www.adp-gmbh.ch/cpp/regexp/pcre/functions.html
//                http://regexkit.sourceforge.net/Documentation/pcre
//                http://www.gammon.com.au/pcre/pcreapi.html
//
// Syntax:        http://regexkit.sourceforge.net/Documentation/pcre/pcresyntax.html
//

#ifndef _PERLREGULAREXPRESSION_H_INCLUDED
#define _PERLREGULAREXPRESSION_H_INCLUDED

#include <iostream>
#include "Array.h"
#include "SigString.h"
#include "pcre.h"
#include <vector>


class PerlRegularExpression {
   public:
           PerlRegularExpression    (void);
          ~PerlRegularExpression    ();

      int  search                   (Array<char>& input,
                                     const char* searchstring,
                                     const char* optionstring = NULL);
      int  search                   (const vector<char>& input,
                                     const string& searchstring,
                                     const string& optionstring = "");
      int  search                   (const string& input,
                                     const string& searchstring,
                                     const string& optionstring = "");
      int  search                   (const char* input,
                                     const char* searchstring,
                                     const char* optionstring = NULL);
      int  search                   (const Array<char>& input);
      int  search                   (const string& input);
      int  search                   (const char* input);

      int  searchAndReplace         (Array<char>& output, const char* input,
                                     const char* searchstring,
                                     const char* replacestring);
      int  searchAndReplace         (string& output, const string& input,
                                     const string& searchstring,
                                     const string& replacestring);
      int  searchAndReplace         (Array<char>& output, const char* input);
      int  searchAndReplace         (string& output, const string& input);

      int  sar                      (Array<char>& inout,
                                     const char* searchstring,
                                     const char* replacestring,
                                     const char* optionstring = NULL);
      int  sar                      (string& inout,
                                     const string& searchstring,
                                     const string& replacestring,
                                     const string& optionstring = "");
      int  sar                      (vector<char>& inout,
                                     const string& searchstring,
                                     const string& replacestring,
                                     const string& optionstring = "");

      void tr                       (Array<char>& inout,
                                     const char* inputlist,
                                     const char* outputlist);

      void tr                       (string& inout,
                                     const string& inputlist,
                                     const string& outputlist);

      static int getTokens          (Array<SigString>& output,
                                     const char* separator,
                                     const char* input);

      static int getTokens          (Array<Array<char>>& output,
                                     const char* separator,
                                     const char* input);

      static int getTokens          (vector<string>& output,
                                     const string& separator,
                                     const string& input);

      static int getTokensWithEmpties(Array<Array<char>>& output,
                                      const char* separator, const char* input);

      static int getTokensWithEmpties(vector<string>& output,
                                      const string& separator, const string& input);

      void initializeSearch         (void);
      void initializeSearch         (const char* searchstring);

      void studySearch              (void);
      void initializeSearchAndStudy (void);
      void initializeSearchAndStudy (const char* searchstring);

      const char* getSubmatch       (int index);
      const char* getSubmatch       (void);
      int  getSubmatchStart         (int index);
      int  getSubmatchEnd           (int index);

      // set regex compile options:
      void setExtendedSyntax        (void);
      void setBasicSyntax           (void);
      void setIgnoreCase            (void);
      void setNoIgnoreCase          (void);
      void setSingle                (void);
      void setGlobal                (void);
      void setAnchor                (void);
      void setNoAnchor              (void);

      void setSearchString          (const char* searchstring);
      void setReplaceString         (const char* replacestring);

   protected:
      char  ignorecaseQ;
      char  extendedQ;
      char  globalQ;
      char  anchorQ;                // true if anchored search
      int   valid;
      int   studyQ;

      pcre* pre;                    // Perl-Compatible RegEx compile structure
      pcre_extra* pe;               // Extra data structure for analyzing
                                    // pcre* information for faster searches.
      const char* compile_error;
      int   error_offset;

      Array<int> output_substrings; // list of substring matches
      Array<char> substring;        // storage for a substring match

      Array<char> input_string;     // storage for input string
      Array<char> search_string;    // regular expression
      Array<char> replace_string;

   private:
      void expandList               (Array<char>& expandlist,
                                     const string& input);
      void expandList               (vector<char>& expandlist,
                                     const string& input);

};

#endif  /* _PERLREGULAREXPRESSION_H_INCLUDED */



// md5sum: fff68b3c92d5d5d55f2491aa14e0b15c PerlRegularExpression.h [20050403]

