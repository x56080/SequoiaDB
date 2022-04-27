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

   Source File Name = dpsJournalEntry.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_JOURNAL_ENTRY_HPP_
#define DPS_JOURNAL_ENTRY_HPP_

#include "dpsLogRecord.hpp"
#include "ossLikely.hpp"
#include <array> //c++11

namespace engine
{
   class dpsJournalEntry : public SDBObject
   {
      public:
         dpsJournalEntry() = default;

         ///rawdata begins from log record header.
         dpsJournalEntry(const CHAR *rawdata, UINT32 bufferSize);
         ~dpsJournalEntry() = default;
         dpsJournalEntry(const dpsJournalEntry &);
         dpsJournalEntry &operator=(const dpsJournalEntry &);

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _header._length;}
         OSS_INLINE const dpsLogRecordHeader &getHeader()const {return _header;}
         OSS_INLINE BOOLEAN hasElements()const {return 0 < _elementCount;}
         OSS_INLINE const CHAR *getElementRawData()const {return _elementBuffer;}
         OSS_INLINE UINT32 getElementRawDataSize()const
         {
            return hasElements() ?
                   _header._length - DPS_LOG_HEAD_SIZE : 0;
         }
         OSS_INLINE UINT32 getEntrySize()const
         {
            return _header._length;
         }
         OSS_INLINE UINT32 getElementCount()const {return _elementCount;}

      public:
         void reset();

         ///rawdata begins from log record header.
         INT32 load(const CHAR *rawdata, UINT32 bufferSize);

         /// type can not be dummy.
         /// raw data can not be invalid.
         INT32 init(UINT16 type,
                    UINT16 flags,
                    UINT32 elementRawSize,
                    const CHAR *elementRawData);

      public:
         class element : public SDBObject
         {
            friend class dpsJournalEntry;
            public:
               element() = default;
               ~element() = default;
               element(const dpsRecordEle &meta,
                       const CHAR *value):
               _meta(meta),
               _value(value){}
               element(const element &) = default;
               element &operator=(const element &) = default;

            public:
               OSS_INLINE BOOLEAN isValid()const {return DPS_INVALID_TAG != _meta.tag;}
               OSS_INLINE const dpsRecordEle &getMeta()const {return _meta;}
               OSS_INLINE DPS_TAG getTag()const {return _meta.tag;}
               OSS_INLINE UINT32 getSize()const {return _meta.len;}
               OSS_INLINE const CHAR *getValue()const {return _value;}
               
               template<class T>
               const T *getObjectValue()const
               {
                  SDB_ASSERT(isValid() && _meta.len == sizeof(T), "can not be invalid");
                  return reinterpret_cast<const T *>(_value);
               }

               OSS_INLINE INT32 getInt32Value()const
               {
                  if (OSS_LIKELY(isValid() && _meta.len == sizeof(INT32)))
                  {
                     return *(reinterpret_cast<const INT32 *>(_value));
                  }
                  else
                  {
                     SDB_ASSERT(FALSE, "invalid value size");
                     return 0;
                  }
               }
               OSS_INLINE INT64 getInt64Value()const
               {
                  if (OSS_LIKELY(isValid() && _meta.len == sizeof(INT64)))
                  {
                     return *(reinterpret_cast<const INT64 *>(_value));
                  }
                  else
                  {
                     SDB_ASSERT(FALSE, "invalid value size");
                     return 0;
                  }
               }

            private:
               dpsRecordEle _meta;
               const CHAR *_value = nullptr;
         };//class element

         element getElement(UINT32 pos)const;

      private:
         INT32 _loadElements(const CHAR *elementRawData, UINT32 size);

      private:
         dpsLogRecordHeader _header;
         UINT32 _elementCount = 0;
         std::array<dpsRecordEle, DPS_MERGE_BLOCK_MAX_DATA> _elements;
         std::array<UINT32, DPS_MERGE_BLOCK_MAX_DATA> _values;
         const CHAR *_elementBuffer = nullptr;
   };//class dpsJournalEntry
   

} // namespace engine


#endif//DPS_JOURNAL_ENTRY_HPP_