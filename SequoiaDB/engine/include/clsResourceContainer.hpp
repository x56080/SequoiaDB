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

   Source File Name = clsResourceContainer.hpp

   Descriptive Name = Resource Container

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/10/2020   LYB Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_RESOURCE_CONTAINER_HPP_
#define CLS_RESOURCE_CONTAINER_HPP_

#include "oss.hpp"
#include "ossTypes.hpp"

namespace engine
{
   class _coordResource ;

   class _clsResourceContainer : public SDBObject
   {
   public:
      _clsResourceContainer() : _pResource( NULL )
      {
      }

      ~_clsResourceContainer()
      {
         _pResource = NULL ;
      }

   public:
      void setResource( _coordResource *pResource )
      {
         _pResource = pResource ;
      }

      _coordResource* getResource()
      {
         return _pResource ;
      }

   private:
      _coordResource *_pResource ;
   };

   typedef _clsResourceContainer clsResourceContainer ;

   /*
      get global pointer
   */
   _clsResourceContainer* sdbGetResourceContainer() ;
}

#endif //CLS_RESOURCE_CONTAINER_HPP_


