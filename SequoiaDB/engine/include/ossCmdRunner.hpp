/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossCmdRunner.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_OSSRUNNER_HPP_
#define SPT_OSSRUNNER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossNPipe.hpp"
#include "ossProc.hpp"
#include "ossEvent.hpp"

#include <vector>
#include <map>
#include <string>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options/parsers.hpp>

using namespace std ;

namespace engine
{
   class _ossCmdRunner : public SDBObject, public ossIExecHandle
   {
   public:
      _ossCmdRunner() ;
      virtual ~_ossCmdRunner() ;

   public:
      /*
         timeout is ms, ignored when isBackground = TRUE
         dupOut: whether duplicate the exe's stdout
      */
      INT32 exec( const CHAR *cmd, UINT32 &exit,
                  BOOLEAN isBackground = FALSE,
                  INT64 timeout = -1,
                  BOOLEAN needResize = FALSE,
                  OSSHANDLE *pHandle = NULL,
                  BOOLEAN addShellPrefix = FALSE,
                  BOOLEAN dupOut = TRUE ) ;

      INT32 done() ;

      INT32 read( string &out, BOOLEAN readEOF = TRUE ) ;

      OSSPID getPID() const { return _id ; }

   protected:
      virtual void  handleInOutPipe( OSSPID pid,
                                     OSSNPIPE * const npHandleStdin,
                                     OSSNPIPE * const npHandleStdout ) ;

      void  asyncRead() ;
      void  monitor() ;

      INT32 _readOut( string &out, BOOLEAN readEOF = TRUE ) ;

   private:
      OSSNPIPE       _out ;
      OSSPID         _id ;
      BOOLEAN        _stop ;
      ossEvent       _event ;
      ossEvent       _monitorEvent ;
      BOOLEAN        _hasRead ;
      string         _outStr ;
      INT32          _readResult ;
      INT64          _timeout ;

      boost::thread  *_pThread ;

   } ;
   typedef class _ossCmdRunner ossCmdRunner ;
}

#endif // SPT_OSSRUNNER_HPP_

