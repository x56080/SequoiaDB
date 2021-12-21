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

   Source File Name = rtnCSHandlers.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_RTN_CS_HANDLERS_HPP_
#define SDB_RTN_CS_HANDLERS_HPP_

#include "rtnHandler.hpp"
#include "utilUniqueID.hpp"
#include "dmsEngineOptions.hpp"

namespace engine
{
   class rtnCreateCSHandler : public rtnHandler
   {
      public:
         rtnCreateCSHandler(){}
         virtual ~rtnCreateCSHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb);

      public:
         void init(const CHAR *name,
                   const utilCSUniqueID &uniqueId,
                   const dmsCreateCSOptions &o);
         OSS_INLINE void setSysCall(BOOLEAN v) {_syscall = v;}

      private:
         const CHAR *_name = NULL;
         utilCSUniqueID _uniqueId = UTIL_UNIQUEID_NULL;
         dmsCreateCSOptions _o;
         BOOLEAN _syscall = FALSE;

   };//class rtnCreateCSHandler

   class rtnTestCSHandler : public rtnHandler
   {
      public:
         rtnTestCSHandler(){}
         virtual ~rtnTestCSHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb);

      public:
         OSS_INLINE void init(const CHAR *name,
                              const utilCSUniqueID &uniqueId)
         {
            _name = name;
            _input = uniqueId;
            _output = UTIL_UNIQUEID_NULL;
         }

         OSS_INLINE const utilCSUniqueID &getUniqueId()const {return _output;}

      private:
         const CHAR *_name = NULL;
         utilCSUniqueID _input = UTIL_UNIQUEID_NULL;
         utilCSUniqueID _output = UTIL_UNIQUEID_NULL;
   };//class rtnTestCSHandler
} // namespace engine


#endif//SDB_RTN_CS_HANDLERS_HPP_