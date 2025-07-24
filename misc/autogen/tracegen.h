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

   Source File Name = tracegen.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef TRACEGEN_H
#define TRACEGEN_H

#include <string>
#include "core.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include "filenamegen.h"

// output file path
#define TRACEFILENAMEPATH SOURCEPATH "include/pdTrace.h"
#define TRACEFILENAMEPATH1 SOURCEPATH "pd/pdFunctionList.cpp"
#define TRACEINCLUDEPATH SOURCEPATH "include/"
#define TRACEINCLUDESUFFIX "Trace.h"
#define TRACEEYECATCHER "PD_TRACE_DECLARE_FUNCTION"
#define TRACEEYECATCHERLEN 25
class TraceGen
{
public :
   static void genList ();
private :
   static void _extractFromFile ( std::ofstream *fout,
                                  std::ofstream &fout1,
                                  const CHAR *pFileName,
                                  INT32 compid ) ;
   static void _genList ( const CHAR *pPath,
                          std::ofstream *fout,
                          std::ofstream &fout1,
                          INT32 compid ) ;
} ;

const INT32 _pdTraceComponentNum = 28 ;
const CHAR *pdGetTraceComponent ( UINT32 id ) ;

#endif
