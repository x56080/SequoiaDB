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

   Source File Name = seAdptOprMon.hpp

   Descriptive Name = Search engine adapter operation monitor.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/21/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_OPRMON_HPP__
#define SEADPT_OPRMON_HPP__

#include "oss.hpp"
#include "rtnExtOprDef.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptOprMon : public SDBObject
   {
   public:
      _seAdptOprMon()
      {
         reset( TRUE ) ;
      }

      ~_seAdptOprMon() {}

      void reset( BOOLEAN resetTotal = FALSE )
      {
         _insertCount = 0 ;
         _updateCount = 0 ;
         _deleteCount = 0 ;
         _ignoreCount = 0 ;
         if ( resetTotal )
         {
            _totalInsertCount = 0 ;
            _totalUpdateCount = 0 ;
            _totalDeleteCount = 0 ;
            _totalIgnoreCount = 0 ;
         }
      }

      OSS_INLINE void monOprCountInc( _rtnExtOprType type, UINT64 delta = 1 )
      {
         switch ( type )
         {
         case RTN_EXT_INSERT:
            monInsertCountInc( delta ) ;
            break ;
         case RTN_EXT_UPDATE:
         case RTN_EXT_UPDATE_WITH_ID:
            monUpdateCountInc( delta ) ;
            break ;
         case RTN_EXT_DELETE:
            monDeleteCountInc( delta ) ;
            break ;
         default:
            break ;
         }
      }

      OSS_INLINE void monInsertCountInc( UINT64 delta = 1 )
      {
         _insertCount += delta ;
         _totalInsertCount += delta ;
      }

      OSS_INLINE void monUpdateCountInc( UINT64 delta = 1 )
      {
         _updateCount += delta ;
         _totalUpdateCount += delta ;
      }

      OSS_INLINE void monDeleteCountInc( UINT64 delta = 1 )
      {
         _deleteCount += delta ;
         _totalDeleteCount += delta ;
      }

      OSS_INLINE void monIgnoreCountInc( UINT64 delta = 1 )
      {
         _ignoreCount += delta ;
         _totalIgnoreCount += delta ;
      }

   public:
      UINT32 _insertCount ;
      UINT32 _updateCount ;
      UINT32 _deleteCount ;
      UINT32 _ignoreCount ;
      UINT64 _totalInsertCount ;
      UINT64 _totalUpdateCount ;
      UINT64 _totalDeleteCount ;
      UINT64 _totalIgnoreCount ;
   } ;
   typedef _seAdptOprMon seAdptOprMon ;
}

#endif /* SEADPT_OPRMON_HPP__ */

