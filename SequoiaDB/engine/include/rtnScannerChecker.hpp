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

   Source File Name = rtnScannerChecker.hpp

   Descriptive Name = RunTime Scanner Checker

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2022  HGM first init

   Last Changed =

*******************************************************************************/
#ifndef RTN_SCANNER_CHECKER_HPP_
#define RTN_SCANNER_CHECKER_HPP_

#include "pmdEDU.hpp"
#include "dmsScanner.hpp"

namespace engine
{

   /*
      _rtnScannerChecker define
    */
   // scanner checker to check if scanner is interrupted
   // use the place holder context to hold position for scanner
   class _rtnScannerChecker : public utilPooledObject,
                              public IDmsScannerChecker
   {
   public:
      _rtnScannerChecker( pmdEDUCB *cb ) ;
      virtual ~_rtnScannerChecker() ;

   public:
      virtual INT32 open( UINT32 suLID,
                          UINT32 mbLID,
                          const CHAR *csName,
                          const CHAR *clShortName,
                          const CHAR *optrDesc ) ;
      virtual void  release() ;
      virtual BOOLEAN needInterrupt() ;

   protected:
      INT32 _open( UINT32 suLID,
                   UINT32 mbLID,
                   const CHAR *csName,
                   const CHAR *clShortName,
                   const CHAR *optrDesc ) ;
      void _release() ;

   protected:
      pmdEDUCB * _eduCB ;
      INT64      _contextID ;
   } ;

   typedef class _rtnScannerChecker rtnScannerChecker ;

   /*
      _rtnScannerCheckerCreator define
    */
   class _rtnScannerCheckerCreator : public _IDmsScannerCheckerCreator
   {
   public:
      _rtnScannerCheckerCreator() {}
      virtual ~_rtnScannerCheckerCreator() {}

      INT32 createChecker( UINT32 suLID,
                           UINT32 mbLID,
                           const CHAR *csName,
                           const CHAR *clShortName,
                           const CHAR *optrDesc,
                           pmdEDUCB *cb,
                           IDmsScannerChecker **ppChecker ) ;
      void releaseChecker( IDmsScannerChecker *pChecker ) ;
   } ;

   typedef class _rtnScannerCheckerCreator rtnScannerCheckerCreator ;

}

#endif // RTN_SCANNER_CHECKER_HPP_
