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

   Source File Name = sptFuncMap.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_FUNCMAP_HPP_
#define SPT_FUNCMAP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptInvokeDef.hpp"


namespace engine
{
   using namespace JS_INVOKER ;

   class _sptFuncMap : public SDBObject
   {
   public:
      _sptFuncMap()
      :_construct(NULL),
       _destruct(NULL),
       _resolve(NULL)
      {

      }

      virtual ~_sptFuncMap()
      {
         _construct = NULL ;
         _destruct = NULL ;
         _resolve = NULL ;
         _normal.clear() ;
      }
   public:
      typedef std::map<std::string, JS_INVOKER::MEMBER_FUNC>
              NORMAL_FUNCS ;
   public:
      BOOLEAN isMemberFunc( const CHAR *funcName ) const
      {
         return NULL == funcName ?
                FALSE : 0 < _normal.count( funcName ) ;
      }

      JS_INVOKER::MEMBER_FUNC
      getMemberFunc( const CHAR *funcName ) const
      {
         JS_INVOKER::MEMBER_FUNC func = NULL ;
         if ( NULL != funcName )
         {
            NORMAL_FUNCS::const_iterator itr =
                        _normal.find( funcName ) ;
            if ( _normal.end() != itr )
            {
               func = itr->second ;
            }
         }

         return func ;
      }

      const NORMAL_FUNCS &getMemberFuncs()const
      {
         return _normal ;
      }

      const NORMAL_FUNCS &getStaticFuncs() const
      {
         return _static ;
      }

      const NORMAL_FUNCS &getGlobalFuncs() const
      {
         return _global ;
      }

      BOOLEAN addMemberFunc( const CHAR *name,
                             JS_INVOKER::MEMBER_FUNC f )
      {
         return ( NULL != name && NULL != f ) ?
                _normal.insert( std::make_pair( name, f ) ).second :
                FALSE ;
      }

      BOOLEAN addStaticFunc( const CHAR *name,
                             JS_INVOKER::MEMBER_FUNC f )
      {
         return ( NULL != name && NULL != f ) ?
                _static.insert( std::make_pair( name, f ) ).second :
                FALSE ;
      }

      BOOLEAN addGlobalFunc( const CHAR *name,
                             JS_INVOKER::MEMBER_FUNC f )
      {
         return ( NULL != name && NULL != f ) ?
                _global.insert( std::make_pair( name, f ) ).second :
                FALSE ;
      }

      void setConstructor( JS_INVOKER::MEMBER_FUNC f )
      {
         _construct = f ;
      }

      void setDestructor( JS_INVOKER::DESTRUCT_FUNC f )
      {
         _destruct = f ;
      }

      void setResolver( JS_INVOKER::RESLOVE_FUNC f )
      {
         _resolve = f ;
      }

      JS_INVOKER::MEMBER_FUNC getConstructor() const
      {
         return _construct ;
      }

      JS_INVOKER::DESTRUCT_FUNC getDestructor() const
      {
         return _destruct ;
      }

      JS_INVOKER::RESLOVE_FUNC getResolver() const
      {
         return _resolve ;
      }
   private:

      NORMAL_FUNCS _normal ;
      NORMAL_FUNCS _static ;
      NORMAL_FUNCS _global ;
      JS_INVOKER::MEMBER_FUNC _construct ;
      JS_INVOKER::DESTRUCT_FUNC _destruct ;
      JS_INVOKER::RESLOVE_FUNC _resolve ;
   } ;
   typedef class _sptFuncMap sptFuncMap ;
}

#endif

