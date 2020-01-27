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

   Source File Name = coordGTSAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   global transaction control in COORD.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_GTS_AGENT_HPP__
#define COORD_GTS_AGENT_HPP__

#include "oss.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "dpsTransDef.hpp"
#include "sdbInterface.hpp"
#include "dpsGTSAgent.hpp"

namespace engine
{

   class _coordResource ;

   /*
      _coordGTSAgent define
    */
   // _coordGTSAgent handles GTS agent in COORD
   class _coordGTSAgent : public SDBObject,
                          public dpsGTSAgent
   {
   public:
      // constructor and destructor
      _coordGTSAgent() ;
      virtual ~_coordGTSAgent() ;

   public:
      // initialize GTS agent with COORD resource
      INT32 init( _coordResource *resource ) ;
      // finalize GTS agent to release COORD resource
      void  fini() ;

   public:
      // update global lowTran
      virtual INT32  updateGlobLowTran() ;
      // on attach event
      virtual void   onAttach( pmdEDUCB *eduCB ) ;
      // on detach event
      virtual void   onDetach( pmdEDUCB *eduCB ) ;

   protected:
      // COORD resource
      _coordResource * _resource ;
   } ;

   typedef class _coordGTSAgent coordGTSAgent ;

}

#endif // COORD_GTS_AGENT_HPP__
