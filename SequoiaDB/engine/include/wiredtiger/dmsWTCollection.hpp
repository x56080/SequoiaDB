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

   Source File Name = dmsWTCollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_COLLECTION_HPP_
#define DMS_WT_COLLECTION_HPP_

#include "interface/ICollection.hpp"
#include "dmsMetadata.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCollection define
    */
   class _dmsWTCollection : public ICollection, public _dmsWTStoreHolder
   {
   public:
      _dmsWTCollection( const dmsCLMetadata &metadata,
                        dmsWTStorageEngine *engine,
                        const dmsWTStore &dataStore )
      : dmsWTStoreHolder( engine, dataStore),
        _metadata( metadata )
      {
      }

      virtual ~_dmsWTCollection() = default ;

      virtual INT32 allocRecordID( UINT32 length, dmsRecordID &rid ) ;
      virtual INT32 insertRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) ;
      virtual INT32 updateRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) ;
      virtual INT32 removeRecord( const dmsRecordID &rid,
                                  IExecutor *executor ) ;
      virtual INT32 extractRecord( const dmsRecordID &rid,
                                   dmsRecordData &recordData,
                                   IExecutor *executor ) ;

      virtual INT32 createDataCursor( std::unique_ptr< IDataCursor > &cursor,
                                      const dmsRecordID &startRID,
                                      BOOLEAN afterStartRID,
                                      BOOLEAN isForward,
                                      IExecutor *executor ) ;

   protected:
      dmsCLMetadata _metadata ;
   } ;

   typedef class _dmsWTCollection dmsWTCollection ;

}
}

#endif // DMS_WT_COLLECTION_HPP_
