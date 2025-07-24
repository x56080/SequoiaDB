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

   Source File Name = mongolob.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __MONGLOB_HPP__
#define __MONGLOB_HPP__

#include "iclient.h"
#include "mongo/client/dbclient.h"

#include <stdlib.h>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <string.h>

using namespace mongo ;
using namespace std ;

class CMongoLob:public IClient
{
   public:
         CMongoLob ( const std::string& strSrvAddr ,
                             const std::string& strDbName ,
                             int startID ) ;
         ~CMongoLob ( ) ;

         virtual int Connect ( ) ;
         virtual int Putfile ( int id ) ;
         virtual int Getfile ( int id ) ;
         virtual int Mix ( int id ) ;
         virtual void Release ( ) ;

   private:
         string ChooseFile ( int id ) ;
   
   private:
         std::string m_strSrvAddr ;
         std::string m_dbName ;
         int m_startID ;

         mongo::DBClientConnection m_conn ;
         mongo::GridFS* gridfs ;

         vector<string> fileslist ;
         string sourceDir ;
         string outDir ;
         
} ;

#endif

