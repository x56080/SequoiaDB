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

   Source File Name = utilPooledAutoPtr.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_POOLED_AUTO_PTR_HPP__
#define UTIL_POOLED_AUTO_PTR_HPP__

#include "ossTypes.hpp"

namespace engine
{

   /*
      _utilPooledAutoPtr define
   */
   class _utilPooledAutoPtr
   {
      public:
         _utilPooledAutoPtr() ;
         _utilPooledAutoPtr( const _utilPooledAutoPtr &rhs ) ;
         ~_utilPooledAutoPtr() ;

         _utilPooledAutoPtr& operator= ( const _utilPooledAutoPtr &rhs ) ;
         bool operator! () const { return get() ? false : true ; }

         operator bool () { return get() ? true : false ; }
         operator CHAR* () { return get () ; }
         operator BOOLEAN () { return get() ? TRUE : FALSE ; }
         operator const CHAR* () { return get() ; }

         static _utilPooledAutoPtr alloc( UINT32 size ) ;

      public:
         CHAR*       get() ;
         const CHAR* get() const ;
         INT32       refCount() const ;
         void        release() ;

      private:
         _utilPooledAutoPtr( CHAR *ptr ) ;

      protected:
         INT32*      _refPtr() ;

      private:
         CHAR        *_ptr ;
   } ;
   typedef _utilPooledAutoPtr utilPooledAutoPtr ;

}

#endif // UTIL_POOLED_AUTO_PTR_HPP__

