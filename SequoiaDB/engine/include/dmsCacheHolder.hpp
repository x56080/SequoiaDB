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

   Source File Name = dmsCacheHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_CACHE_HOLDER_HPP_
#define SDB_DMS_CACHE_HOLDER_HPP_

#include "dmsSUCache.hpp"

namespace engine
{
   class _dmsCollectionStat;
   typedef class _dmsCollectionStat dmsCollectionStat;

   class _dmsStorageUnit;
   typedef class _dmsStorageUnit dmsStorageUnit;

   class _dmsMBContext;
   typedef class _dmsMBContext dmsMBContext;

   class _dmsIndexStat;
   typedef _dmsIndexStat dmsIndexStat;

   class _dmsCacheHolder : public IDmsSUCacheHolder
   {
      public :
         _dmsCacheHolder ( _dmsStorageUnit *su ) ;

         virtual ~_dmsCacheHolder () ;

         virtual const CHAR *getCSName () const ;

         virtual UINT32 getSUID () const ;

         virtual UINT32 getSULID () const ;

         virtual BOOLEAN isSysSU () const ;

         virtual BOOLEAN checkCacheUnit ( utilSUCacheUnit *pCacheUnit ) ;

         virtual BOOLEAN createSUCache ( UINT8 type ) ;

         virtual BOOLEAN deleteSUCache ( UINT8 type ) ;

         virtual void deleteAllSUCaches () ;

         OSS_INLINE virtual dmsSUCache *getSUCache ( UINT8 type )
         {
            if ( type < DMS_CACHE_TYPE_NUM )
            {
               return _pSUCaches[ type ] ;
            }
            return NULL ;
         }

         //dmsStorageUnit *getSU ()
         //{
           // return _su ;
        // }

      protected :
         INT32 _checkCollectionStat ( dmsCollectionStat *pCollectionStat ) ;
         INT32 _checkIndexStat ( dmsIndexStat *pIndexStat,
                                 dmsMBContext *ctx ) ;

      protected :
         dmsStorageUnit*      _su ;
         dmsSUCache *         _pSUCaches [ DMS_CACHE_TYPE_NUM ] ;
   } ;

   typedef class _dmsCacheHolder dmsCacheHolder;
} // namespace engine


#endif//SDB_DMS_CACHE_HOLDER_HPP_