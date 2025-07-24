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

   Source File Name = sequoiadblob.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIALOB_HPP__
#define __SEQUOIALOB_HPP__

#include "iclient.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

using namespace std ;

class CSdbLob:public IClient
{
   public:
         CSdbLob ( const std::string& strSrvAddr,
                               const std::string& strDbName, 
                               int startID ) ;
         ~CSdbLob ( ) ;

         virtual int Connect ( ) ;
         virtual int Putfile ( int id ) ;
         virtual int Getfile ( int id ) ;
         virtual int Mix ( int id ) ;
         virtual void Release ( ) ;

   private:
         string ChooseFile ( int id ) ;
         char* getCharPtr ( string &str ) ;
         int init ( ) ;
         
   private:
         std::string m_strSrvAddr ;
         std::string m_dbName ;
         std::string m_fullClName ;
         int m_startID ;


   private:
         const static UINT64 lobSize = 1024*16 ;
      
         char* pSrvAddr ;
         char* pCSName ;
         char* pCLName ;
         int rc ;

         sdbConnectionHandle m_conn ;
         sdbCSHandle m_cs ;
         sdbCollectionHandle m_cl ;
         sdbLobHandle m_lob ;
         char* readBuf ;
         char* lobWriteBuf ;
         char* lobReadBuf ;

         vector<string> fileslist ;
         string sourceDir ;
         string outDir ;
         
  
} ;

#endif

