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

   Source File Name = ICursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_CURSOR_HPP_
#define SDB_I_CURSOR_HPP_

#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"

namespace engine
{

   /*
      ICursor define
    */
   class ICursor : public _utilPooledObject
   {
   public:
      ICursor() = default ;
      virtual ~ICursor() = default ;
      ICursor( const ICursor & ) = delete ;
      ICursor &operator =( const ICursor & ) = delete ;

   public:
      virtual BOOLEAN isOpened() const = 0 ;
      virtual BOOLEAN isClosed() const = 0 ;
      virtual BOOLEAN isForward() const = 0 ;
      virtual BOOLEAN isBackward() const = 0 ;
      virtual BOOLEAN isEOF() const = 0 ;

      virtual INT32 close() = 0 ;

      virtual INT32 moveNext( IExecutor *executor ) = 0 ;
      virtual INT32 movePrev( IExecutor *executor ) = 0 ;

      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) = 0 ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) = 0 ;
   } ;

}


#endif // SDB_I_CURSOR_HPP_