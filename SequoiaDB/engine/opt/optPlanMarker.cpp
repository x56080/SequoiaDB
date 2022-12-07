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

   Source File Name = optPlanMarker.cpp

   Descriptive Name = Optimizer Access Plan Marker

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "optPlanMarker.hpp"

namespace engine
{
   void optPlanMarker::incMainCLInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         INT32 &count = found->second.first;
         if ( count <= OPT_MAIN_CL_INVALID_THRESHOLD )
         {
            ++count;
         }
      }
      else
      {
         _counter.emplace( clFullName, make_pair< INT32, INT32 >( 1, 0 ) );
      }
   }
   void optPlanMarker::incParamInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         INT32 &count = found->second.second;
         if ( count <= OPT_PARAM_INVALID_THRESHOLD )
         {
            ++count;
         }
      }
      else
      {
         _counter.emplace( clFullName, make_pair< INT32, INT32 >( 0, 1 ) );
      }
   }

   void optPlanMarker::setMainCLInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      _counter[ clFullName ].first = OPT_MAIN_CL_INVALID_THRESHOLD + 1;
   }
   void optPlanMarker::setParamInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      _counter[ clFullName ].second = OPT_PARAM_INVALID_THRESHOLD + 1;
   }

   void optPlanMarker::erase( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      _counter.erase( clFullName );
   }

   void optPlanMarker::clearMainCLInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         found->second.first = 0;
      }
   }

   void optPlanMarker::clearParamInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         found->second.second = 0;
      }
   }

   void _optPlanMarker::clear()
   {
      ossScopedRWLock lock( &_latch, EXCLUSIVE );
      _counter.clear();
   }

   BOOLEAN optPlanMarker::testMainCLInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, SHARED );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         INT32 &count = found->second.first;
         if ( count > OPT_MAIN_CL_INVALID_THRESHOLD )
         {
            return TRUE;
         }
      }
      return FALSE;
   }
   BOOLEAN optPlanMarker::testParamInvalid( const CHAR *clFullName )
   {
      ossScopedRWLock lock( &_latch, SHARED );
      ossPoolMap< ossPoolString, std::pair< INT32, INT32 > >::iterator found =
         _counter.find( clFullName );
      if ( found != _counter.end() )
      {
         INT32 &count = found->second.first;
         if ( count > OPT_PARAM_INVALID_THRESHOLD )
         {
            return TRUE;
         }
      }
      return FALSE;
   }
} // namespace engine