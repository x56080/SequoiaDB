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

   Source File Name = dpsRecordElements.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_RECORD_ELEMENTS_HPP__
#define DPS_RECORD_ELEMENTS_HPP__

#include "utilUniqueBuffer.hpp"
#include "dpsDef.hpp"

namespace engine
{
   class _dpsRecordEle ;

   class _dpsRecordElements : public SDBObject
   {
      public:
         _dpsRecordElements() = default ;
         ~_dpsRecordElements() = default ;
         explicit _dpsRecordElements( utilUniqueBuffer &&buf,
                                      UINT32 size,
                                      INT32 num=-1 ) noexcept ;
         explicit _dpsRecordElements( const CHAR *buf,
                                      UINT32 size,
                                      INT32 num=-1) noexcept ;

         _dpsRecordElements( const _dpsRecordElements & ) = delete ;
         _dpsRecordElements &operator=( const _dpsRecordElements & ) = delete ;
         _dpsRecordElements( _dpsRecordElements && ) noexcept ;
         _dpsRecordElements &operator=( _dpsRecordElements && ) noexcept ;

      public:
         OSS_INLINE BOOLEAN isValid() const { return nullptr != _buf ; }
         OSS_INLINE BOOLEAN isOwned() const { return _owner.isValid() ; }
         OSS_INLINE void reset()
         {
            _size = 0 ;
            _elementNum = -1 ;
            _buf = nullptr ;
            _owner.reset() ;
         }
         OSS_INLINE UINT32 getSize() const { return _size ; }
         OSS_INLINE const CHAR *getBuf() const { return _buf ; }
         OSS_INLINE utilSlice getSlice() const { return utilSlice( _size, _buf ) ; }

      public:
         INT32 getOwned() ;

         UINT32 getElementNum() const ;

      public:
         class iterator : public SDBObject
         {
            friend class _dpsRecordElements ;
            public:
               iterator() = default ;
               ~iterator() = default ;
            private:
               explicit iterator( const CHAR *data, UINT32 size ) noexcept ;

            public:
               OSS_INLINE void reset()
               {
                  _data = nullptr ;
                  _size = 0 ;
               }
               BOOLEAN isValid() const ;
               DPS_TAG getTag() const ;
               utilSlice getValue() const ;
               BOOLEAN next() ;

            private:
               const _dpsRecordEle *_getElementHeader() const ;

            private:
               const CHAR *_data = nullptr ;
               UINT32 _size = 0 ;
         };

         iterator seek( DPS_TAG tag ) const ;
         iterator begin() const ;
         BOOLEAN contains( DPS_TAG tag ) const ;

      private:
         UINT32 _size = 0 ;
         mutable INT32 _elementNum = -1 ;
         const CHAR *_buf = nullptr ;
         utilUniqueBuffer _owner ;
   };

   using dpsRecordElements = class _dpsRecordElements ;
} // namespace engine


#endif//DPS_RECORD_ELEMENTS_HPP__