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

   Source File Name = omagentNodeMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2015  LZ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_NODEPATHGUARD_HPP_
#define OMAGENT_NODEPATHGUARD_HPP_

#include "oss.hpp"
#include "ossSocket.hpp"
#include "pmdOptionsMgr.hpp"
#include <vector>

namespace engine {

   class _omaNodePathGuard : public SDBObject
   {
   public:
      _omaNodePathGuard() ;
      ~_omaNodePathGuard() ;

      void  init( const CHAR *nodeName, pmdOptionsCB *options ) ;

      const CHAR* name() const { return _nodeName ; }
      std::vector< std::string > *getPaths() { return &_nodePaths ; }

      BOOLEAN muteXOn( _omaNodePathGuard *pOther ) ;
      INT32   checkValid( pmdOptionsCB *options ) ;

   protected:
      INT32    _checkExistedFiles( const CHAR* path,
                                   const CHAR *filter = NULL,
                                   UINT32 deep = 1 ) ;

   private:
      CHAR _nodeName[ OSS_MAX_SERVICENAME + 1 ] ;
      std::vector<std::string> _nodePaths ;

   };

   typedef _omaNodePathGuard omaNodePathGuard ;
}

#endif // OMAGENT_NODEPATHGUARD_HPP_

