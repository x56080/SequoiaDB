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
#include <array> //c++11

namespace engine
{
   class dpsJournalEntry : public SDBObject
   {
      public:
         dpsJournalEntry() = default;
         dpsJournalEntry(const CHAR *rawdata);
         ~dpsJournalEntry();
         dpsJournalEntry(const dpsJournalEntry &) = delete;
         dpsJournalEntry &operator=(const dpsJournalEntry &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _header;}
         OSS_INLINE BOOLEAN isOwned()const {return nullptr != _bufferOwned;}
         OSS_INLINE const dpsLogRecordHeader *getHeader()const {return _header;}
         OSS_INLINE const CHAR *getRawData()const {return (const CHAR *)_header;}
         OSS_INLINE UINT32 getEntrySize()const
         {
            return isValid() ? _header->_length : 0;
         }
         OSS_INLINE UINT32 getElementCount()const {return _elementCount;}

         void reset();
         INT32 getOwned();
         

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
               _val(value){}
               element(const element &) = default;
               element &operator=(const element &) = default;

            public:
               OSS_INLINE BOOLEAN isValid()const {return DPS_INVALID_TAG != _meta.tag;}
               OSS_INLINE const dpsRecordEle &getMeta()const {return _meta;}
               OSS_INLINE const CHAR *getVal()const {return _val;}
            private:
               dpsRecordEle _meta;
               const CHAR *_val = nullptr;
         };//class element

         element getElement(UINT32 pos)const;

      private:
         INT32 _loadData(const CHAR *rawdata);
         void _clearDataLoaded();

      private:
         const dpsLogRecordHeader *_header = nullptr;
         UINT32 _elementCount = 0;
         std::array<dpsRecordEle, DPS_MERGE_BLOCK_MAX_DATA> _elements;
         std::array<UINT32, DPS_MERGE_BLOCK_MAX_DATA> _values = {};

         CHAR *_bufferOwned = nullptr;
   };//class dpsJournalEntry

} // namespace engine


#endif//DPS_JOURNAL_ENTRY_HPP_