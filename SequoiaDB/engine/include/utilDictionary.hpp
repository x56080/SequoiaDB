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

   Source File Name = utilDictionary.hpp

   Descriptive Name = Uniform dictionary definitions.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/23/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_DICTIONARY
#define UTIL_DICTIONARY

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
   /* Dictionary type. Currently only lzw is supported. */
   enum UTIL_DICT_TYPE
   {
      UTIL_DICT_LZW = 1,
      UTIL_DICT_INVALID
   } ;

   #define UTIL_INVALID_DICT_VERSION      0xFF
   #define UTIL_LZW_DICT_VERSION          1
   /*
    * The uniform header structure of dictionaries. Any dictionary should put
    * this structure at the head.
    */
   struct _utilDictHead
   {
      UTIL_DICT_TYPE _type ;
      UINT8 _version ;
      CHAR _reserve[20] ;
   } ;
   typedef _utilDictHead utilDictHead ;

   struct _utilDictionaryDetail
   {
      UTIL_DICT_TYPE _type ;
      UINT32 _maxCode ;
      UINT8 _version ;
      UINT8 _codeSize ;
      UINT8 _varLenCompEnable ;
   } ;
   typedef _utilDictionaryDetail utilDictionaryDetail ;

   class _utilDictCreator : public SDBObject
   {
   public:
      virtual ~_utilDictCreator() {}
      virtual INT32 prepare() = 0 ;
      virtual void reset() = 0 ;
      virtual void build( const CHAR *src, UINT32 srcLen, BOOLEAN &full ) = 0 ;
      virtual INT32 finalize( CHAR *buff, UINT32 &size ) = 0 ;
   } ;
   typedef _utilDictCreator utilDictCreator ;

   void getDictionaryDetail( void *dictionary, utilDictionaryDetail &detail ) ;
}

#endif /* UTIL_DICTIONARY */

