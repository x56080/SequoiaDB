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

   Source File Name = iclient.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __ICLIENT_H__
#define __ICLIENT_H__

#include <string>

using namespace std ;

struct IClient
{
   public:
         virtual int Connect ( ) = 0 ;
         virtual int Putfile ( int id ) = 0 ;
         virtual int Getfile ( int id ) = 0 ;
         virtual int Mix ( int id ) = 0 ;
	 virtual void Release ( ) = 0 ;
} ;

extern "C" IClient* CreateClient ( const char* pSrvAddr, const char* pDbName, int startID ) ;

#endif

