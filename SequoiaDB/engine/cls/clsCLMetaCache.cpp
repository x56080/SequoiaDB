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

   Source File Name = clsCLMetaCache.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/30/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsCLMetaCache.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   _clsCLMetaCache::_clsCLMetaCache( ossPoolString &&clFullName,
                                     utilCLUniqueID clUID,
                                     const CLS_CL_STAT_PTR &clStatPtr,
                                     CLS_INDEX_INFO_SET_PTR &&infoSetPtr )
   : _name( std::move( clFullName ) )
   , _clUID( clUID )
   , _clStatPtr( clStatPtr )
   , _indexInfoSetPtr( std::move( infoSetPtr ) )
   {
   }

   INT32 _clsCLMetaCache::init( const CHAR *clFullName,
                                utilCLUniqueID clUID,
                                const CLS_CL_STAT_PTR &clStatPtr,
                                CLS_INDEX_INFO_SET_PTR &&infoSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( clFullName, SDB_INVALIDARG, error, PDERROR, "can not be nullptr" );
      PD_CHECK( UTIL_IS_VALID_CLUNIQUEID( clUID ), SDB_INVALIDARG, error, PDERROR,
                "collection unique id[%d] must be valid", clUID );
      try
      {
         _name = ossPoolString( clFullName );
         _clUID = clUID;
         _clStatPtr = clStatPtr;
         _indexInfoSetPtr = std::move( infoSetPtr );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void _clsCLMetaCache::reset()
   {
      _name.clear();
      _clUID = UTIL_UNIQUEID_NULL;
      _clStatPtr = CLS_DEFAULT_CL_STAT;
      _indexInfoSetPtr = nullptr;
   }

   void _clsCLMetaCache::resetCLStat()
   {
      _clStatPtr = CLS_DEFAULT_CL_STAT;
   }

   void _clsCLMetaCache::setCLStat( const CLS_CL_STAT_PTR &clStatPtr )
   {
      _clStatPtr = clStatPtr;
   }

   void _clsCLMetaCache::setIndexInfoSetPtr( const CLS_INDEX_INFO_SET_PTR &infoSetPtr )
   {
      _indexInfoSetPtr = infoSetPtr;
   }

   CLS_INDEX_INFO_SET_PTR &_clsCLMetaCache::_getIndexInfoSet()
   {
      return _indexInfoSetPtr;
   }

   const ossPoolString &_clsCLMetaCache::_getCLFullName() const
   {
      return _name;
   }

   CLS_CL_STAT_PTR &_clsCLMetaCache::_getCLStat()
   {
      return _clStatPtr;
   }
} // namespace engine