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

   Source File Name = dmsRBSGCJob.hpp

   Descriptive Name = Rollback Segment Garbage Collection Job Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to clear
   cached access plans.

   Dependencies: N/A

   Restrictions: N/A
  
   Change Activity:
   defect Date        Who Description 
   ====== =========== === ==============================================
          02/05/2020  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSRBSGCJOB_HPP__
#define DMSRBSGCJOB_HPP__

#include "utilLightJobBase.hpp"
    
namespace engine
{

   // forward class declaration
   class _dmsRBSMgr ;

   /*
      _dmsRBSGCJob define
      Class to trigger RBS GC work in the background
   */
   class _dmsRBSGCJob : public _utilLightJob
   {
      public:
         _dmsRBSGCJob( _dmsRBSMgr  *rbsSUMgr ) ;
         virtual ~_dmsRBSGCJob() ;
         virtual const CHAR*     name() const ;
         virtual INT32   doit( IExecutor *pExe,
                               UTIL_LJOB_DO_RESULT &result,
                               UINT64 &sleepTime ) ;
     
      private:
         _dmsRBSMgr * _rbsSUMgr ;
   } ;
   typedef _dmsRBSGCJob dmsRBSGCJob ;

   INT32  dmsStartAsyncRBSGC( ) ;

}
#endif //DMSRBSGCJOB_HPP__

