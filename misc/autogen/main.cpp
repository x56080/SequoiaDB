/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = main.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "rcgen.h"
#include "filenamegen.h"
#include "optgen.h"
#include "tracegen.h"
#include "buildgen.h"
#include "dbConfForWeb.h"
#include "vergen.h"
#include <iostream>
using std::cout;
using std::endl;

void displayUsage (const char* argv)
{
    cout<<"Usage: "<<argv<<" <language>"<<endl
        <<"    <language>   desciption language as below ( default is english ):"<<endl
        <<"           en    english descriptions"<<endl
        <<"           cn    chinese desciptions"<<endl;
}

static void genRC ( const char *lang )
{
   RCGen xml ( lang ) ;
   xml.run () ;
}

static void genOpt ( const char *lang )
{
   OptGen xml ( lang ) ;
   xml.run () ;
}

static void genTrace ()
{
   TraceGen::genList () ;
}

static void genVer()
{
   VerGen verGen ;
   verGen.run(false) ;
}

enum supportedLangs
{
   LANG_CN = 0,
   LANG_EN,
   LANG_MAX
} ;

const CHAR *pLang[] = {
   "cn",
   "en"
} ;

static void genDoc ( const char *lang )
{
   RCGen rcGen ( lang ) ;
   rcGen.genDoc() ;

   OptGenForWeb optGen ( lang ) ;
   optGen.run () ;

   VerGen verGen ;
   verGen.run(true) ;

   for ( int i = 0; i < LANG_MAX; ++i )
   {
      RCGen xml ( pLang[i] ) ;
      // generate web console for english and chinese
      xml.genWeb() ;  
   }
}

static void genBuild ()
{
   BuildGen gen ;
   gen.run () ;
}

static void genFileName ()
{
   FileNameGen::genList() ;
}

int main (int argc, char** argv)
{
   if ( ( 2 == argc && argv[1][0] == '?' ) ||
        ( 2 < argc ))
   {
      if ( argv[1][0] == '?' )
         displayUsage ( argv[0] ) ;
      return 0 ;
   }
   const char * lang = "en" ;
   if ( 2 == argc )
   {
      lang = argv[1] ;
   }
   genRC ( lang ) ;
   genFileName () ;
   genOpt ( lang ) ;
   genTrace () ;
   genDoc ( "cn" ) ;
   genBuild () ;
   genVer() ;
   return 0;
}
