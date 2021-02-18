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

   Source File Name = clNameOrID.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CL_NAME_OR_ID_H_
#define VESSEL_CL_NAME_OR_ID_H_

#include "dms.hpp"
#include "slice.h"

namespace engine
{
namespace vessel
{
   class clNameOrID : public SDBObject
   {
      public:
         OSS_INLINE clNameOrID()
                    :_cs(DMS_INVALID_LOGICCSID),
                     _cl(DMS_INVALID_LOGICCLID){}

         OSS_INLINE ~clNameOrID(){}

         OSS_INLINE clNameOrID(UINT32 len, const CHAR *name)
                    :_cs(DMS_INVALID_LOGICCSID),
                     _cl(DMS_INVALID_LOGICCLID),
                     _name(len, name){}

         OSS_INLINE clNameOrID(const clNameOrID &o)
                    :_cs(DMS_INVALID_LOGICCSID),
                     _cl(DMS_INVALID_LOGICCLID),
                      _name(o._name){}

         OSS_INLINE clNameOrID &operator=(const clNameOrID &o)
         {
            _cs = o._cs;
            _cl = o._cl;
            _name = o._name;
            return *this;
         }

         OSS_INLINE void set(UINT32 cs, UINT32 cl)
         {
            _cs = cs;
            _cl = cl;
         }

         OSS_INLINE void set(UINT32 len, const CHAR *name)
         {
            _name.reset(len, name);
         }

         OSS_INLINE void reset()
         {
            _cs = DMS_INVALID_LOGICCSID;
            _cl = DMS_INVALID_LOGICCLID;
            _name.reset(0, NULL);
         }

         OSS_INLINE UINT32 getCS()const
         {
            return _cs;
         }

         OSS_INLINE UINT32 getCL()const
         {
            return _cl;
         }

         OSS_INLINE const slice &getName()const
         {
            return _name;
         }
      private:
         UINT32 _cs;
         UINT32 _cl;
         slice _name;
   }; // class clNameOrID
} // namespace vessel
} // namespace engine

#endif // VESSEL_COLLECTION_NAME_OR_ID_H_