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

   Source File Name = dmsIxmKeySorter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_IXM_KEY_SORTER_HPP_
#define DMS_IXM_KEY_SORTER_HPP_

#include "ixmKey.hpp"
#include "dms.hpp"
#include "../bson/ordering.h"

namespace engine
{
   class _dmsIxmKeyComparer
   {
   public:
      _dmsIxmKeyComparer( bson::Ordering& order )
      : _order( order )
      {
      }

      bool operator()( const ixmKey& key1, const ixmKey& key2 )
      {
         return ( key1.woCompare( key2, _order ) < 0 ) ;
      }

   private:
      bson::Ordering _order ;
   } ;
   typedef class _dmsIxmKeyComparer dmsIxmKeyComparer ;
   
   class _dmsIxmKeySorter: public SDBObject
   {
   private:
      // disallow copy and assign
      _dmsIxmKeySorter( const _dmsIxmKeySorter& ) ;
      void operator=( const _dmsIxmKeySorter& ) ;

   protected:
      _dmsIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer )
      : _bufSize( bufSize ), _comparer( comparer ) {}

   public:
      virtual ~_dmsIxmKeySorter() {}
      virtual INT32 push( const ixmKey& key, const dmsRecordID& recordID ) = 0 ;
      virtual INT32 sort() = 0 ;
      virtual INT32 fetch( ixmKey& key, dmsRecordID& recordID ) = 0 ;
      virtual INT32 reset() = 0 ;
      virtual INT64 bufferSize() const { return _bufSize ; }
      virtual INT64 usedBufferSize() const = 0 ;

   protected:
      INT64                _bufSize ;
      _dmsIxmKeyComparer   _comparer ;
   } ;
   typedef class _dmsIxmKeySorter dmsIxmKeySorter ;

   class _dmsIxmKeySorterCreator: public SDBObject
   {
   private:
      // disallow copy and assign
      _dmsIxmKeySorterCreator( const _dmsIxmKeySorterCreator& ) ;
      void operator=( const _dmsIxmKeySorterCreator& ) ;

   protected:
      _dmsIxmKeySorterCreator() {}

   public:
      virtual ~_dmsIxmKeySorterCreator() {}
      virtual _dmsIxmKeySorter* createSorter(  INT64 bufSize, const _dmsIxmKeyComparer& comparer  ) = 0 ;
      virtual void releaseSorter( _dmsIxmKeySorter* sorter ) = 0 ;
   } ;
   typedef class _dmsIxmKeySorterCreator dmsIxmKeySorterCreator ;
}

#endif /* DMS_IXM_KEY_SORTER_HPP_ */

