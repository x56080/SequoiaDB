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

   Source File Name = dmsSysSUMgr.hpp

   Descriptive Name = Data Management Service SYS Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS System Storage Unit management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMSSYSSUMGR_HPP__
#define DMSSYSSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "dms.hpp"

using namespace std ;

namespace engine
{

   class _SDB_DMSCB ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;

   /*
      _dmsSysSUMgr define
   */
   class _dmsSysSUMgr : public SDBObject
   {
      public :
         _dmsSysSUMgr ( _SDB_DMSCB *dmsCB )
         : _dmsCB( dmsCB )
         {
            _su = NULL ;
         }

         virtual ~_dmsSysSUMgr () {}

         _dmsStorageUnit *getSU ()
         {
            return _su ;
         }

      protected :
         #define DMSSYSSUMGR_XLOCK() ossScopedLock _lock(&_mutex, EXCLUSIVE)
         #define DMSSYSSUMGR_SLOCK() ossScopedLock _lock(&_mutex, SHARED)

         ossSpinSLatch        _mutex ;
         _dmsStorageUnit      *_su ;
         _SDB_DMSCB           *_dmsCB ;
   } ;


}

#endif //DMSSYSSUMGR_HPP__

