/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rtnResultSetFilter.hpp

   Descriptive Name = Resultset filter

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/20/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_RSFILTER_HPP__
#define RTN_RSFILTER_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "pd.hpp"

namespace engine
{
   class _rtnResultSetFilter : public SDBObject
   {
      typedef ossPoolSet<dmsRecordID>  RID_SET ;
      typedef RID_SET::const_iterator  RID_SET_ITR ;

   public:
      _rtnResultSetFilter() {}
      virtual ~_rtnResultSetFilter() {}

      OSS_INLINE INT32 push( const dmsRecordID &rid, BOOLEAN &hasPushed ) ;
      OSS_INLINE BOOLEAN isFiltered( const dmsRecordID &item ) ;

   private:
      RID_SET _filterSet ;
   } ;
   typedef _rtnResultSetFilter rtnResultSetFilter ;

   OSS_INLINE INT32 _rtnResultSetFilter::push( const dmsRecordID &rid,
                                               BOOLEAN &hasPushed )
   {
      INT32 rc = SDB_OK ;
      std::pair<ossPoolSet<dmsRecordID>::iterator, bool> result ;
      try
      {
         result = _filterSet.insert( rid ) ;
      }
      catch( const std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      hasPushed = result.second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE BOOLEAN _rtnResultSetFilter::isFiltered( const dmsRecordID &item )
   {
      return ( _filterSet.find( item ) != _filterSet.end() ) ;
   }
}

#endif /* RTN_RSFILTER_HPP__ */
