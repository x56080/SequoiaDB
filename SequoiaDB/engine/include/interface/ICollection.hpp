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

   Source File Name = ICollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_COLLECTION_HPP_
#define SDB_I_COLLECTION_HPP_

#include "sdbInterface.hpp"
#include "interface/IDataCursor.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"

namespace engine
{

   /*
      ICollection define
    */
   class ICollection : public SDBObject
   {
   public:
      ICollection() = default ;
      virtual ~ICollection() = default ;
      ICollection( const ICollection & ) = delete ;
      ICollection &operator =( const ICollection & ) = delete ;

      virtual INT32 allocRecordID( UINT32 length, dmsRecordID &rid ) = 0 ;
      virtual INT32 insertRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 updateRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 removeRecord( const dmsRecordID &rid,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 extractRecord( const dmsRecordID &rid,
                                   dmsRecordData &recordData,
                                   IExecutor *executor ) = 0 ;
      virtual INT32 createDataCursor( std::unique_ptr< IDataCursor > &cursor,
                                      const dmsRecordID &startRID,
                                      BOOLEAN afterStartRID,
                                      BOOLEAN isForward,
                                      IExecutor *executor ) = 0 ;
   } ;

}


#endif // SDB_I_COLLECTION_HPP_
