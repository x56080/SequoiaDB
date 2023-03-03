/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
