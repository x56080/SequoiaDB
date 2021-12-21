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

   Source File Name = rtnCLHandlers.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_RTN_CL_HANDLERS_HPP_
#define SDB_RTN_CL_HANDLERS_HPP_

#include "rtnHandler.hpp"
#include "utilUniqueID.hpp"
#include "dmsEngineOptions.hpp"

namespace engine
{
   class rtnCreateCLHandler : public rtnHandler
   {
      public:
         rtnCreateCLHandler(){}
         virtual ~rtnCreateCLHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb);

      public:
         void init(const CHAR *name,
                   const utilCLUniqueID &uniqueId,
                   const dmsCreateCLOptions &o)
         {
            _fullName = name;
            _uniqueId = uniqueId;
            _o = o;
         }
         OSS_INLINE void setSysCall(BOOLEAN v) {_syscall = v;}

      private:
         const CHAR *_fullName = NULL;
         utilCLUniqueID _uniqueId = UTIL_UNIQUEID_NULL;
         dmsCreateCLOptions _o;
         BOOLEAN _syscall = FALSE;

   };//class rtnCreateCLHandler

   class rtnTestCLHandler : public rtnHandler
   {
      public:
         rtnTestCLHandler(){}
         virtual ~rtnTestCLHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb);

      public:
         OSS_INLINE void init(const CHAR *fullName,
                              const utilCLUniqueID &uniqueId)
         {
            _fullName = fullName;
            _input = uniqueId;
            _output = UTIL_UNIQUEID_NULL;
   
         }

         OSS_INLINE const utilCLUniqueID &getUniqueId()const {return _output;}

      private:
         const CHAR *_fullName = NULL;
         utilCLUniqueID _input = UTIL_UNIQUEID_NULL;
         utilCLUniqueID _output = UTIL_UNIQUEID_NULL;
   };//class rtnTestCLHandler
} // namespace engine


#endif//SDB_RTN_CL_HANDLERS_HPP_