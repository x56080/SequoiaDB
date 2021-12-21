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

   Source File Name = dmsEngineCB.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_ENGINE_CB_HPP_
#define SDB_DMS_ENGINE_CB_HPP_

#include "sdbInterface.hpp"
#include "interface/IDataStorageEngine.h"

namespace engine
{
   class _dmsEngineCB : public _IControlBlock
   { 
      public:
         _dmsEngineCB();
         virtual ~_dmsEngineCB();
         _dmsEngineCB(const _dmsEngineCB &o) = delete;
         _dmsEngineCB &operator=(const _dmsEngineCB &) = delete;

      public:
         virtual SDB_CB_TYPE cbType() const {return SDB_CB_DMS_ENGINE;}
         virtual const CHAR* cbName() const {return "DMS_ENGINE_CB";}

         virtual INT32 init();
         virtual INT32 active();
         virtual INT32 deactive();
         virtual INT32 fini();

      public:
         OSS_INLINE IDataStorageEngine *getEngine(){return _engine;}

      private:
         IDataStorageEngine *_engine = NULL;

   };//class _dmsEngineCB

   typedef class _dmsEngineCB SDB_DMS_ENGINE_CB;

   SDB_DMS_ENGINE_CB *sdbGetDMSEngineCB();
} // namespace engine


#endif//SDB_DMS_ENGINE_CB_HPP_