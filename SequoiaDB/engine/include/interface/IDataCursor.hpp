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

   Source File Name = IDataCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_DATA_CURSOR_HPP_
#define SDB_I_DATA_CURSOR_HPP_

#include "interface/ICursor.hpp"

namespace engine
{

   // forward declaration
   class ICollection ;

   /*
      IDataCursor define
    */
   class IDataCursor : public ICursor
   {
   public:
      IDataCursor() = default ;
      virtual ~IDataCursor() = default ;
      IDataCursor( const IDataCursor & ) = delete ;
      IDataCursor &operator =( const IDataCursor & ) = delete ;

   public:
      virtual INT32 open( ICollection *collection,
                          const dmsRecordID &startRID,
                          BOOLEAN afterStartRID,
                          BOOLEAN isForward,
                          IExecutor *executor ) = 0 ;
   } ;

}


#endif // SDB_I_DATA_CURSOR_HPP_