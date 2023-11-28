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

   Source File Name = dmsDataCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_DATA_CURSOR_HPP_
#define SDB_DMS_DATA_CURSOR_HPP_

#include "interface/IDataCursor.hpp"
#include "dms.hpp"

namespace engine
{

   /*
      _dmsDataCursor defined
    */
   class _dmsDataCursor : public IDataCursor
   {
   public:
      _dmsDataCursor() = default ;
      virtual ~_dmsDataCursor() = default ;
      _dmsDataCursor( const _dmsDataCursor & ) = delete ;
      _dmsDataCursor &operator =( const _dmsDataCursor & ) = delete ;

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

      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

   protected:
      void _setEOF()
      {
         _isEOF = TRUE ;
      }

      void _setCurrentRecordID( const dmsRecordID &recordID )
      {
         _curentRecordID = recordID ;
      }

      void _setCurrentRecord( const dmsRecordData &data )
      {
         _currentRecordData = data ;
      }

   protected:
      std::shared_ptr< ICollection > _collPtr ;
      dmsRecordID _curentRecordID ;
      dmsRecordData _currentRecordData ;
      BOOLEAN _isOpened = FALSE ;
      BOOLEAN _isClosed = FALSE ;
      BOOLEAN _isForward = TRUE ;
      BOOLEAN _isEOF = FALSE ;
   } ;

   typedef class _dmsDataCursor dmsDataCursor ;

}

#endif // SDB_DMS_DATA_CURSOR_HPP_