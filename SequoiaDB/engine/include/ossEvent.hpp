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

   Source File Name = ossEvent.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSS_EVENT_HPP_
#define OSS_EVENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <boost/thread.hpp>
#include <boost/thread/cv_status.hpp>

namespace engine
{

   class _ossEvent : public SDBObject
   {
      public:
         _ossEvent () ;
         virtual ~_ossEvent () ;

      public:
         INT32 wait ( INT64 millisec = -1, INT32 *pData = NULL ) ;
         INT32 signal ( INT32 data = 0 ) ;
         INT32 signalAll ( INT32 data = 0 ) ;
         INT32 reset () ;
         UINT32 waitNum () ;

      protected:
         virtual void _onWait () ;

      protected:
         mutable boost::mutex       _mutex ;
         boost::condition_variable  _cond ;
         UINT32                     _signal ;
         UINT32                     _waitNum ;
         INT32                      _useData ;

   };

   typedef _ossEvent ossEvent ;

   class _ossAutoEvent : public _ossEvent
   {
      public:
         _ossAutoEvent () ;
         virtual ~_ossAutoEvent () ;

      protected:
         virtual void _onWait () ;

   };

   typedef _ossAutoEvent ossAutoEvent ;

}

#endif //OSS_EVENT_HPP_
