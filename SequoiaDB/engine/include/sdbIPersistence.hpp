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

   Source File Name = sdbIPersistence.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/01/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_PERSISTENCE_HPP__
#define SDB_I_PERSISTENCE_HPP__

#include "sdbInterface.hpp"

namespace engine
{

   /*
      _IDataSyncBase define
   */
   class _IDataSyncBase
   {
      public:
         _IDataSyncBase() {}
         virtual ~_IDataSyncBase() {}

      public:
         virtual BOOLEAN      isClosed() const = 0 ;
         virtual BOOLEAN      canSync( BOOLEAN &force ) const = 0 ;

         virtual INT32        sync( BOOLEAN force,
                                    BOOLEAN sync,
                                    IExecutor* cb ) = 0 ;

         virtual void         lock() = 0 ;
         virtual void         unlock() = 0 ;
   } ;
   typedef _IDataSyncBase IDataSyncBase ;

   /*
      _IDataSyncManager define
   */
   class _IDataSyncManager
   {
      public:
         _IDataSyncManager() {}
         virtual ~_IDataSyncManager() {}

      public:
         virtual void         registerSync( IDataSyncBase *pSyncUnit ) = 0 ;
         virtual void         unregSync( IDataSyncBase *pSyncUnit ) = 0 ;
         virtual void         notifyChange() = 0 ;
   } ;
   typedef _IDataSyncManager IDataSyncManager ;

}

#endif // SDB_I_PERSISTENCE_HPP__

