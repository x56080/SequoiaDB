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

