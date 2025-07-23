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

   Source File Name = dmsInternalSchemaHandler.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/11/2023  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INTSCHEMA_HANDLER_HPP__
#define DMS_INTSCHEMA_HANDLER_HPP__

#include "dmsEventHandler.hpp"
#include "dmsSysSUMgr.hpp"

namespace engine
{
   /*
      _dmsInternalSchemaHandler define
   */
   class _dmsInternalSchemaHandler : public _IDmsEventHandler
   {
         struct cmp_column_name
         {
            bool operator()( const CHAR *l, const CHAR *r)
            {
               return ossStrcmp( l, r ) < 0 ;
            }
         } ;
      public:
         _dmsInternalSchemaHandler() ;
         virtual ~_dmsInternalSchemaHandler() {}

         virtual INT32 onCreateIndex ( IDmsEventHolder *pEventHolder,
                                       IDmsSUCacheHolder *pCacheHolder,
                                       const dmsEventCLItem &clItem,
                                       const dmsEventIdxItem &idxItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

         OSS_INLINE virtual UINT32 getMask () const
         {
            return DMS_EVENT_MASK_SCHEMA ;
         }

         OSS_INLINE virtual const CHAR *getName() const
         {
            return "InnerSchemaHandler" ;
         }

   } ;
   typedef _dmsInternalSchemaHandler dmsInternalSchemaHandler ;

}

#endif /* DMS_INTSCHEMA_HANDLER_HPP__ */
