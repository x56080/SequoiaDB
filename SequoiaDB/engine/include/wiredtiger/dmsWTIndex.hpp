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

   Source File Name = dmsWTIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_INDEX_HPP_
#define DMS_WT_INDEX_HPP_

#include "interface/IIndex.hpp"
#include "dmsMetadata.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTIndex define
    */
   class _dmsWTIndex : public IIndex,
                       public _dmsWTStoreHolder
   {
   public:
      _dmsWTIndex( const dmsIdxMetadata &metadata,
                   dmsWTStorageEngine &engine,
                   const dmsWTStore &indexStore )
      : _dmsWTStoreHolder( engine, indexStore ),
        _metadata( metadata, TRUE )
      {
      }

      virtual ~_dmsWTIndex() = default ;

      virtual const dmsIdxMetadata &getMetadata() const
      {
         return _metadata ;
      }

      virtual INT32 index( const keystring::keyString &keyString,
                           const dmsRecordID &rid,
                           BOOLEAN checkDuplicated,
                           IExecutor *executor ) ;
      virtual INT32 unindex( const keystring::keyString &keyString,
                             const dmsRecordID &rid,
                             IExecutor *executor ) ;

      virtual INT32 createIndexCursor( std::unique_ptr<IIndexCursor> &cursor,
                                       const keystring::keyString &startKey,
                                       BOOLEAN isAfterStartKey,
                                       BOOLEAN isForward,
                                       IExecutor *executor ) ;

      static INT32 buildIdxConfigString( const dmsWTEngineOptions &options,
                                         const dmsCreateIdxOptions &createIdxOptions,
                                         ossPoolString &configString ) ;

      static INT32 buildIdxURI( utilCSUniqueID csUID,
                                utilCLInnerID clInnerID,
                                UINT32 clLID,
                                utilIdxInnerID idxInnerID,
                                ossPoolString &idxURI ) ;

   protected:
      BOOLEAN _isUnique = FALSE ;
      BOOLEAN _isStrictUnique = FALSE ;
      dmsIdxMetadata _metadata ;
   } ;

   typedef class _dmsWTIndex dmsWTIndex ;

}
}

#endif // DMS_WT_INDEX_HPP_
