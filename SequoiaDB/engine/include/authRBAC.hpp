/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = authRBAC.hpp

   Descriptive Name = Role based access control header file.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/09/2022  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_RBAC_HPP__
#define AUTH_RBAC_HPP__

#include "core.hpp"
namespace engine
{
   UINT32 authGetBuiltinRoleID( const CHAR *roleName ) ;
   BOOLEAN authIsMonCmd( const CHAR *cmdName ) ;
}

#endif /* AUTH_RBAC_HPP__ */
