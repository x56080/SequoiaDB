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

   Source File Name = seAdptPipeHandlers.hpp

   Descriptive Name = seadapter pipe handlers

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for pipe
   manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          25/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SEADPT_PIPE_HANDLES_HPP__
#define SEADPT_PIPE_HANDLES_HPP__

#include "pmdPipeManager.hpp"

using  namespace engine ;

namespace seadapter
{
   /*
      _seAdapterPipeHandler define
    */
   // _seAdapterPipeHandler handles messages to acquire seAdapter info
   class _seAdapterPipeHandler : public _IPmdPipeHandler,
                                 public utilPooledObject
   {
   public:
      _seAdapterPipeHandler() ;
      virtual ~_seAdapterPipeHandler() ;

   public:
      virtual INT32 processMessage( CHAR *message,
                                    utilNodePipe &nodePipe ) ;
   } ;
   typedef class _seAdapterPipeHandler seAdapterPipeHandler ;
}

#endif // SEADPT_PIPE_HANDLES_HPP__

