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

   Source File Name = coordGroupHandle.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_GROUP_HANDLE_HPP__
#define COORD_GROUP_HANDLE_HPP__

#include "coordRemoteSession.hpp"

namespace engine
{

   /*
      _coordGroupHandler define
   */
   class _coordGroupHandler : public _IGroupSessionHandler
   {
      public:
         _coordGroupHandler() ;
         virtual ~_coordGroupHandler() ;

      public:
         virtual void   prepareForSend( UINT32 groupID,
                                        pmdSubSession *pSub,
                                        _coordGroupSel *pSel,
                                        _coordGroupSessionCtrl *pCtrl ) ;

   } ;
   typedef _coordGroupHandler coordGroupHandler ;

}

#endif // COORD_GROUP_HANDLE_HPP__
