/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossStackDump.hpp

   Descriptive Name = Operating System Services Stack Dump Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions to rewind windows
   and linux stacks.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_STACK_DUMP_HPP
#define OSS_STACK_DUMP_HPP

#include "ossFeat.hpp"
#include "ossTypes.hpp"

#define OSS_UNKNOWN_STACKFRAME_NAME "?unknown"
#define OSS_FUNC_NAME_LEN_MAX 1024

typedef char OSS_INSTRUCTION ;
typedef const OSS_INSTRUCTION * OSS_INSTRUCTION_PTR ;

class ossPrimitiveFileOp ;

#if defined (_LINUX)
   #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 128
#elif defined (_WINDOWS)
   #if defined (_WINDOWS32)
       #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 62
   #elif defined(_WINDOWS64)
       #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 128
   #endif
#endif

#if defined (_LINUX)

   #include "ossSignal.hpp"

   void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile ) ;

   void ossWalkStack ( UINT32 framesToSkip,
                       OSS_INSTRUCTION_PTR * ppInstruction,
                       UINT32 framesRequested ) ;

   void ossGetSymbolNameFromAddress ( OSS_INSTRUCTION_PTR pInstruction,
                                      CHAR * pName,
                                      size_t nameSize,
                                      UINT32_64 *pOffset ) ;

   void ossRestoreSystemSignal( const INT32 sigNum,
                                const BOOLEAN isCoreNeeded,
                                const CHAR *dumpDir ) ;

   void ossSignalHandlerAbort( OSS_HANDPARMS, const CHAR *dumpDir ) ;
#elif defined (_WINDOWS)
   #include <windows.h>
   #include <dbgHelp.h>

   UINT32 ossWalkStack ( UINT32 framesToSkip,
                         UINT32 framesRequested,
                         void ** ppInstruction ) ;

   UINT32 ossWalkStackEx( LPEXCEPTION_POINTERS lpEP,
                          UINT32 framesToSkip,
                          UINT32 framesRequested ,
                          void ** ppInstruction ) ;

   void ossGetSymbolNameFromAddress( HANDLE hProcess,
                                     UINT64 pInstruction,
                                     SYMBOL_INFO * pSymbol,
                                     CHAR * pName,
                                     UINT32 nameSize ) ;
   UINT32 ossSymInitialize( HANDLE  phProcess,
                            PSTR    pUserSearchPath,
                            BOOLEAN bInvadeProcess ) ;
#endif  // _LINUX

#endif  // OSS_STACK_DUMP_HPP

