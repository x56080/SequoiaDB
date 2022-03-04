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

   Source File Name = catRecycleBinProcessor.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_RECYCLE_BIN_PROCESSOR_HPP__
#define CAT_RECYCLE_BIN_PROCESSOR_HPP__

#include "oss.hpp"
#include "catDef.hpp"
#include "utilRecycleItem.hpp"
#include "clsCatalogAgent.hpp"
#include "pmdEDU.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   // pre-define
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _catRecycleBinManager ;

   /*
      _catRecycleBinProcessor define
    */
   // catRecycleBinPRocessor: processor for each row of recycle objects
   // - query objects by matchers
   // - processor each objects ( check, or recycle, or return, etc )
   class _catRecycleBinProcessor
   {
   public:
      _catRecycleBinProcessor( _catRecycleBinManager *recyBinMgr,
                               utilRecycleItem &item ) ;
      virtual ~_catRecycleBinProcessor() ;

      utilRecycleItem &getRecycleItem()
      {
         return _item ;
      }

      void increaseMatchedCount()
      {
         ++ _matchedCount ;
      }

      UINT32 getMatchedCount()
      {
         return _matchedCount ;
      }

      void increaseProcessedCount()
      {
         ++ _processedCount ;
      }

      UINT32 getProcessedCount()
      {
         return _processedCount ;
      }

      virtual const CHAR *getCollection() const = 0 ;
      virtual const CHAR *getName() const = 0 ;

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) = 0 ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) = 0 ;

      virtual INT32 getExpectedCount() const
      {
         // -1 means we don't know numbers of objects should be processed
         return -1 ;
      }

   protected:
      _catRecycleBinManager * _recyBinMgr ;
      utilRecycleItem & _item ;
      UINT32            _matchedCount ;
      UINT32            _processedCount ;
   } ;

   typedef class _catRecycleBinProcessor catRecycleBinProcessor ;

   /*
      _catDropItemSubCLChecker define
    */
   // check if sub-collection is in a recycled collection space
   class _catDropItemSubCLChecker : public _catRecycleBinProcessor
   {
   public:
      _catDropItemSubCLChecker( _catRecycleBinManager *recyBinMgr,
                                utilRecycleItem &item ) ;
      virtual ~_catDropItemSubCLChecker() ;

      virtual const CHAR *getCollection() const ;

      virtual const CHAR *getName() const
      {
         return "DropItemSubCLChecker" ;
      }

      virtual INT32 getMatcher( ossPoolList< bson::BSONObj > &matcherList ) ;
      virtual INT32 processObject( const bson::BSONObj &object,
                                   pmdEDUCB *cb,
                                   INT16 w ) ;

   protected:
      BOOLEAN _isChecked( utilCSUniqueID csUniqueID ) const ;
      INT32 _saveChecked( utilCSUniqueID csUniqueID ) ;

   protected:
      typedef ossPoolSet< utilCSUniqueID > _CAT_CS_UID_SET ;
      _CAT_CS_UID_SET _checkedSet ;
   } ;

   typedef class _catDropItemSubCLChecker catDropItemSubCLChecker ;

}

#endif // CAT_RECYCLE_BIN_PROCESSOR_HPP__
