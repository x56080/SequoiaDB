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

   Source File Name = dmsTransContext.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_TRANS_CONTEXT_HPP__
#define DMS_TRANS_CONTEXT_HPP__

#include "sdbInterface.hpp"
#include "dms.hpp"

using namespace bson ;

namespace engine
{

   class _dmsMBContext ;
   class _rtnScanner ;

   /*
      _dmsScanTransContext define
   */
   class _dmsScanTransContext : public _IContext
   {
      public:
         _dmsScanTransContext( _dmsMBContext *pMBContext,
                               _rtnScanner *pScanner,
                               DMS_ACCESS_TYPE accessType ) ;
         virtual ~_dmsScanTransContext() ;

      protected:
         INT32       _checkAccess() ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

         virtual void reset()
         {
            _isCursorSame = TRUE ;
         }

         virtual BOOLEAN isCursorSame() const
         {
            return _isCursorSame ;
         }

      protected:
         _dmsMBContext           *_pMBContext ;
         _rtnScanner             *_pScanner ;
         DMS_ACCESS_TYPE         _accessType ;
         BOOLEAN                 _isCursorSame ;

   } ;
   typedef _dmsScanTransContext dmsScanTransContext ;
   typedef _dmsScanTransContext dmsTBTransContext ;
   typedef _dmsScanTransContext dmsIXTransContext ;

}

#endif /* DMS_TRANS_CONTEXT_HPP__ */

