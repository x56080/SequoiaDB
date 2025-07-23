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

   /*
      _dmsTBTransContext define
   */
   class _dmsTBTransContext : public _IContext
   {
      public:
         _dmsTBTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType ) ;
         virtual ~_dmsTBTransContext() ;

      protected:
         INT32       _checkAccess() ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

      protected:
         _dmsMBContext           *_pMBContext ;
         DMS_ACCESS_TYPE         _accessType ;

   } ;
   typedef _dmsTBTransContext dmsTBTransContext ;

   class _rtnIXScanner ;
   /*
      _dmsIXTransContext define
   */
   class _dmsIXTransContext : public _dmsTBTransContext
   {
      public:
         _dmsIXTransContext( _dmsMBContext *pMBContext,
                             DMS_ACCESS_TYPE accessType,
                             _rtnIXScanner *pScanner ) ;
         virtual ~_dmsIXTransContext() ;

         BOOLEAN  isCursorSame() const ;

      public:
         virtual INT32 pause() ;
         virtual INT32 resume() ;

      protected:
         _rtnIXScanner           *_pScanner ;
         BOOLEAN                 _isReadonly ;
         BOOLEAN                 _isSame ;

   } ;
   typedef _dmsIXTransContext dmsIXTransContext ;

}

#endif /* DMS_TRANS_CONTEXT_HPP__ */

