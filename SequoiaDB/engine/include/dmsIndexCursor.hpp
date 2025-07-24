/*******************************************************************************

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

   Source File Name = dmsIndexCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_INDEX_CURSOR_HPP_
#define SDB_DMS_INDEX_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "dms.hpp"

namespace engine
{

   /*
      _dmsIndexCursor defined
    */
   class _dmsIndexCursor : public IIndexCursor
   {
   public:
      _dmsIndexCursor() = default ;
      virtual ~_dmsIndexCursor() = default ;
      _dmsIndexCursor( const _dmsIndexCursor & ) = delete ;
      _dmsIndexCursor &operator =( const _dmsIndexCursor & ) = delete ;

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

      virtual INT32 getCurrentKeyString( keystring::keyString &key ) ;
      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

   protected:
      keystring::keyString _currentKey ;
      dmsRecordID _currentRecordID ;
      dmsRecordData _currentRecordData ;
      BOOLEAN _isOpened = FALSE ;
      BOOLEAN _isClosed = FALSE ;
      BOOLEAN _isForward = TRUE ;
      BOOLEAN _isEOF = FALSE ;
   } ;

   typedef class _dmsIndexCursor dmsIndexCursor ;

}

#endif // SDB_DMS_INDEX_CURSOR_HPP_