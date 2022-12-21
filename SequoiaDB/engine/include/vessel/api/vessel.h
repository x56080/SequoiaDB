/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = vessel.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_H_
#define VESSEL_VESSEL_H_

#include "interface/IDataStorageEngine.h"
#include "vessel/vesselOptions.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   class IVessel : public IDataStorageEngine
   {
      public:
         IVessel(){}
         virtual ~IVessel(){}
         IVessel(const IVessel &) = delete;
         IVessel &operator=(const IVessel &) = delete;

      public:
         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options) = 0;

         
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options) = 0;

         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct,
                                 DMS_SU_DESCRIPTOR &desc ) override
         {
            return SDB_OK;
         };

         virtual INT32 listCS( IExecutor *executor, DATA_CURSOR_PTR &cursor ) override
         {
            return SDB_OK;
         };

         virtual INT32 removeCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &options ) override
         {
            return SDB_OK;
         };

         virtual INT32 removeEmptyCS( IExecutor *executor,
                                      const CHAR *name,
                                      const dmsRemoveCSOptions &options ) override
         {
            return SDB_OK;
         };

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *name,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite,
                                 DMS_SU_DESCRIPTOR &desc ) override
         {
            return SDB_OK;
         };

         virtual INT32 unloadCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &delOptions ) override
         {
            return SDB_OK;
         };

         virtual INT32 restoreCS( IExecutor *executor, const CHAR *name ) override
         {
            return SDB_OK;
         };

         virtual INT32 returnCS( IExecutor *executor,
                                 dmsReturnOptions &options,
                                 BOOLEAN blockWrite ) override
         {
            return SDB_OK;
         };

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) override
         {
            return SDB_OK;
         };

   }; /// end of class IVessel
} /// end of namespace vessel
} /// end of namespace engine

#endif