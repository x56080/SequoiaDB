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

   Source File Name = sptUsrOmaAssit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USROMA_ASSIT_HPP__
#define SPT_USROMA_ASSIT_HPP__

#include "oss.hpp"
#include "sptUsrRemoteAssit.hpp"
#include <string>
using std::string ;

namespace engine
{

   /*
      sptUsrOmaAssit define
   */
   class _sptUsrOmaAssit : public _sptUsrRemoteAssit
   {
      public:
         _sptUsrOmaAssit() ;
         ~_sptUsrOmaAssit() ;

      public:
         INT32       connect( const CHAR *pHostName,
                              const CHAR *pServiceName ) ;
         INT32       disconnect() ;

         INT32       createNode( const CHAR *pSvcName,
                                 const CHAR *pDBPath,
                                 const CHAR *pConfig ) ;

         INT32       removeNode( const CHAR *pSvcName,
                                 const CHAR * pConfig ) ;

         INT32       startNode( const CHAR *pSvcName ) ;

         INT32       stopNode( const CHAR *pSvcName ) ;

      protected:
         INT32       _getNodeHandle( const CHAR *pSvcName,
                                     ossValuePtr &handle ) ;
         void        _releaseNodeHandle( ossValuePtr handle ) ;

         INT32       _getCoordGroupHandle( ossValuePtr &handle ) ;

         INT32       _regSocket( ossValuePtr pSock ) ;

      private:
         ossValuePtr          _groupHandle ;
   } ;
   typedef _sptUsrOmaAssit sptUsrOmaAssit ;

}

#endif // SPT_USROMA_ASSIT_HPP__
