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

#ifndef DMS_WT_DATA_CURSOR_HPP_
#define DMS_WT_DATA_CURSOR_HPP_

#include "dmsDataCursor.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTItem.hpp"


namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTDataCursor define
    */
   class _dmsWTDataCursor : public _dmsDataCursor
   {
   public:
      _dmsWTDataCursor() ;
      virtual ~_dmsWTDataCursor() = default ;
      _dmsWTDataCursor( const _dmsWTDataCursor & ) = delete ;
      _dmsWTDataCursor &operator =( const _dmsWTDataCursor & ) = delete ;

      virtual INT32 open( ICollection *collection,
                          const dmsRecordID &startRID,
                          BOOLEAN afterStartRID,
                          BOOLEAN isForward,
                          IExecutor *executor ) ;
      virtual INT32 close() ;

      virtual INT32 moveNext( IExecutor *executor ) ;
      virtual INT32 movePrev( IExecutor *executor ) ;

   protected:
      INT32 _extractRecordID() ;
      INT32 _extractRecordData() ;

   protected:
      dmsWTSession _session ;
      dmsWTCursor _cursor ;
   } ;

   typedef class _dmsWTDataCursor dmsWTDataCursor ;

}
}

#endif // DMS_WT_DATA_CURSOR_HPP_
