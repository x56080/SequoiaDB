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

   Source File Name = func.js

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2022/11/15  Tan Zhaobo Initial Draft

   Last Changed =

*******************************************************************************/

function getTSString( ts ) {
   if ( undefined != ts && ts instanceof Timestamp ) {
      return ts.toString().split("\"")[1] ;
   } else {
      throw new Error( "ts is invalid: " + ts ) ;
   }
}