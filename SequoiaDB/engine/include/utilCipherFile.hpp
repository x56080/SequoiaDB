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

   Source File Name = utilCipherFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHERFILE_H_
#define UTILCIPHERFILE_H_

#include "ossFile.hpp"

namespace engine
{

   class _utilCipherAbstractFile : public SDBObject
   {
   public:
      enum cipherRole
      {
         RRole,
         WRole
      } ;

      _utilCipherAbstractFile() {}
      virtual ~_utilCipherAbstractFile() {}

      virtual INT32 initFile( const std::string &fileName, 
                              cipherRole role ) = 0 ;
      virtual INT32 readFromFile( const CHAR **fileContent,
                                  INT64 *contentLen ) = 0 ;
      virtual INT32 writeToFile( const std::string& fileContent ) = 0 ;
   } ;
   typedef _utilCipherAbstractFile utilCipherAbstractFile ;


   class _utilCipherFile : public _utilCipherAbstractFile
   {
   public:
      _utilCipherFile() : _fileContent( NULL ) {}
      ~_utilCipherFile() ;

      INT32 initFile( const std::string &fileName, 
                      cipherRole role) ;
      INT32 readFromFile( const CHAR **fileContent,
                          INT64 *contentLen ) ;
      INT32 writeToFile( const std::string& fileContent ) ;
   private:
      ossFile  _file ;
      CHAR    *_fileContent ;
   } ;
   typedef _utilCipherFile utilCipherFile ;

}

#endif