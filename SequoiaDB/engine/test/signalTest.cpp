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
*******************************************************************************/

#include "core.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossSignal.hpp"
#include "ossStackDump.hpp"
#include "ossUtil.hpp"
#include <stdio.h>
#if defined (_LINUX)
void dummyCore()
{
  OSS_INSTRUCTION_PTR ppAddress[100] ;
  char funcName[256] ;
  UINT32_64 offset = 0 ;
  CHAR *p = NULL ;

  ossWalkStack( 0, ppAddress, 20 ) ;
  for ( int i = 0 ; i < 20 ; i++ )
  {
      ossGetSymbolNameFromAddress( ppAddress[i],
                                   funcName,
                                   sizeof( funcName ),
                                   &offset ) ;
      printf("[%3d] 0x%16p  %s  + 0x%llx\n",
             i, (void*)ppAddress[i], funcName, (UINT64)offset ) ;
  }
  getchar();
  // generate core file
  *p = 10 ;
}


void dummyf5()
{
  dummyCore() ;
}

int dummyf4()
{
   int y=1 ;
   dummyf5() ;
   return  y ;
}

void dummyf3()
{
   dummyf4() ;
}

void dummyf2()
{
   dummyf3() ;
}

void dummyf1()
{
   dummyf2() ;
}

inline void baz()
{
  dummyf1() ;
}

inline void bar()
{
  baz() ;
}

void foo( )
{
  bar() ;
}

void myHdl( OSS_HANDPARMS )
{
   ossPrimitiveFileOp trapFile ;

   trapFile.Open( "/home/taoewang/repos/trap.txt" ) ;
   ossDumpStackTrace( OSS_HANDARGS, &trapFile ) ;
   trapFile.Close() ;

   ossRestoreSystemSignal( signum, false, "/home/taoewang/repos" ) ;
   return ;
}
#endif
int main()
{
   int rc = 0 ;
#if defined (_LINUX)
   struct sigaction newact ;

   sigemptyset (&newact.sa_mask) ;
   newact.sa_sigaction = ( OSS_SIGFUNCPTR ) myHdl ;
   newact.sa_flags |= SA_SIGINFO ;
   newact.sa_flags |= SA_ONSTACK ; ;

   sigaction (SIGSEGV, &newact, NULL) ;

   foo() ;
#endif
   return rc ;

}
