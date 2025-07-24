/*******************************************************************************

<<<<<<< HEAD
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
=======

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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   Source File Name = dmsEngineDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#ifndef SDB_DMS_ENGINE_DEF_HPP_
#define SDB_DMS_ENGINE_DEF_HPP_

#include "dms.hpp"
<<<<<<< HEAD
#include "dmsDef.hpp"
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

namespace engine
{
   enum class DMS_SCAN_FOR : INT32
   {
      NONE = 0,
      SHARE = 1,
      UPDATE = 2,
<<<<<<< HEAD
   } ;

   class dmsEngineDescriptor : public SDBObject
   {
   public:
      dmsEngineDescriptor() = default ;
      ~dmsEngineDescriptor() = default ;
      dmsEngineDescriptor( const dmsEngineDescriptor &o ) = default ;
      dmsEngineDescriptor &operator =( const dmsEngineDescriptor &o ) = default ;

   public:
      static constexpr UINT64 FLAG_DATA_SNAPSHOT_ENABLED = 0x01 ;

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return DMS_STORAGE_ENGINE_UNKNOWN != _type ;
      }

      OSS_INLINE void setName( const CHAR *name )
      {
         _name = name;
      }

      OSS_INLINE const CHAR *getName() const
      {
         return _name;
      }

      OSS_INLINE void setType( DMS_STORAGE_ENGINE_TYPE type )
      {
         _type = type ;
      }

      OSS_INLINE DMS_STORAGE_ENGINE_TYPE getType() const
      {
         return _type ;
      }

      OSS_INLINE void setFlags( UINT64 flags )
      {
         _flags = flags;
      }
      OSS_INLINE BOOLEAN isDataSnapshotEnabled() const
      {
         return 0 != OSS_BIT_TEST( _flags, FLAG_DATA_SNAPSHOT_ENABLED ) ;
      }
   protected:
      const CHAR *_name = NULL ;
      DMS_STORAGE_ENGINE_TYPE _type = DMS_STORAGE_ENGINE_UNKNOWN ;
      UINT64 _flags = 0 ;
   } ;

} // namespace engine

#endif // SDB_DMS_ENGINE_HPP_
=======
   };//enum DMS_SCAN_FOR

   class dmsEngineDescriptor : public SDBObject
   {
      public:
         dmsEngineDescriptor(){}
         ~dmsEngineDescriptor(){}
         dmsEngineDescriptor(const dmsEngineDescriptor &o):
         _name(o._name),
         _type(o._type),
         _flags(o._flags){}
         dmsEngineDescriptor &operator=(const dmsEngineDescriptor &o)
         {
            _name = o._name;
            _type = o._type;
            _flags = o._flags;
            return *this;
         }

      public:
         static constexpr UINT64 FLAG_DATA_SNAPSHOT_ENABLED = 0x01;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _name &&
                   0 != ossStrlen(_name) &&
                   DMS_ENGINE_INVALID != _type;
         }
         OSS_INLINE void setName(const CHAR *name)
         {
            _name = name;
         }
         OSS_INLINE const CHAR *getName()const
         {
            return _name;
         }
         OSS_INLINE void setType(DMS_ENGINE_TYPE type)
         {
            _type = type;
         }
         OSS_INLINE DMS_ENGINE_TYPE getType()const
         {
            return _type;
         }

         OSS_INLINE void setFlags(UINT64 flags)
         {
            _flags = flags;
         }
         OSS_INLINE BOOLEAN isDataSnapshotEnabled()const
         {
            return 0 != OSS_BIT_TEST(_flags, FLAG_DATA_SNAPSHOT_ENABLED);
         }
      private:
         const CHAR *_name = NULL;
         DMS_ENGINE_TYPE _type = DMS_ENGINE_INVALID;
         UINT64 _flags = 0;
   };//class dmsEngineDescriptor
} // namespace engine


#endif//SDB_DMS_ENGINE_HPP_
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
