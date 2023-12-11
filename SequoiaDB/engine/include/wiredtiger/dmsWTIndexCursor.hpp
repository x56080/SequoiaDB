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

   Source File Name = dmsWTCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_INDEX_CURSOR_HPP_
#define DMS_WT_INDEX_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "wiredtiger/dmsWTCursorHolder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTIndexCursor define
    */
   class _dmsWTIndexCursor : public IIndexCursor, public _dmsWTCursorHolder
   {
   public:
      _dmsWTIndexCursor() = default ;
      virtual ~_dmsWTIndexCursor() = default ;
      _dmsWTIndexCursor( const _dmsWTIndexCursor & ) = delete ;
      _dmsWTIndexCursor &operator =( const _dmsWTIndexCursor & ) = delete ;

      virtual BOOLEAN isOpened() const
      {
         return _isOpened ;
      }

      virtual BOOLEAN isClosed() const
      {
         return _isClosed ;
      }

      virtual BOOLEAN isForward() const
      {
         return _isForward ;
      }

      virtual BOOLEAN isBackward() const
      {
         return !_isForward ;
      }

      virtual BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          const keystring::keyString &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          IExecutor *executor ) ;
      virtual INT32 advance( IExecutor *executor ) ;
      virtual INT32 locate( const bson::BSONObj &key,
                            const bson::Ordering &ordering,
                            const dmsRecordID &recordID,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) ;
      virtual INT32 locate( const keystring::keyString &key,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) ;

      virtual INT32 close()
      {
         _resetCache() ;
         return _close() ;
      }

      virtual INT32 getCurrentKeyString( keystring::keyString &key ) ;
      virtual INT32 getCurrentKey( bson::BSONObj &key ) ;
      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

   protected:
      void _resetCache()
      {
         _keyStringCache.reset() ;
         _keyObjCache = BSONObj() ;
      }

   protected:
      std::shared_ptr<IIndex> _idxPtr ;
      keystring::keyString _keyStringCache ;
      bson::BSONObj _keyObjCache ;
   } ;

   typedef class _dmsWTIndexCursor dmsWTIndexCursor ;

}
}

#endif // DMS_WT_INDEX_CURSOR_HPP_
