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

   Source File Name = mthMatchOpNode.hpp

   Descriptive Name = Method Match Operation Node Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCH_OPNODE_HPP_
#define MTH_MATCH_OPNODE_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilList.hpp"
#include <set>
#include <boost/shared_ptr.hpp>
#include "../bson/bson.hpp"
#include "mthMatchNode.hpp"
#include "mthCommon.hpp"
#include <vector>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "pcrecpp.h"

using namespace bson ;
using namespace pcrecpp ;
using namespace std ;

namespace engine
{
   class _mthMatchFunc
   {
      public:
         _mthMatchFunc( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFunc() ;

      public:
         INT32 init( const CHAR *fieldName, const BSONElement &ele ) ;
         BSONObj toBson() ;
         string toString() ;

         // func *p = new ( _mthNodeAllocator *allocator ) func( _mthNodeAllocator *allocator )
         // use p->release() to release p. and do not use p anymore.
         void* operator new ( size_t size, _mthNodeAllocator *allocator ) ;
         // do not call delete p directly
         void operator delete ( void *p ) ;
         // just make the compiler shut up (windows's warning)
         void operator delete ( void *p, _mthNodeAllocator *allocator ) ;


         virtual void release() = 0 ;

      public:
         virtual void clear() ;
         virtual INT32 getType() = 0 ;
         virtual const CHAR* getName() = 0 ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) = 0 ;

         virtual INT32 adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                  _utilArray< INT32 > &out ) ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) = 0 ;

      protected:
         _mthMatchFieldName<> _fieldName ;
         BSONElement _funcEle ;
         _mthNodeAllocator *_allocator ;
   } ;

   class _mthMatchFuncABS : public _mthMatchFunc
   {
      public:
         _mthMatchFuncABS( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncABS() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual void clear() ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;
   } ;

   class _mthMatchFuncCEILING : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncCEILING( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncCEILING() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncFLOOR : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncFLOOR( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncFLOOR() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncLOWER : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncLOWER( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncLOWER() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncUPPER : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncUPPER( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncUPPER() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncLTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncLTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncLTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncRTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncRTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncRTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncSTRLEN : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncSTRLEN( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSTRLEN() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncSUBSTR : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSUBSTR( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSUBSTR() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _begin ;
         INT32 _limit ;
   } ;

   class _mthMatchFuncMOD : public _mthMatchFunc
   {
      public:
         _mthMatchFuncMOD( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncMOD() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncADD : public _mthMatchFunc
   {
      public:
         _mthMatchFuncADD( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncADD() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncSUBTRACT : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSUBTRACT( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSUBTRACT() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncMULTIPLY : public _mthMatchFunc
   {
      public:
         _mthMatchFuncMULTIPLY( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncMULTIPLY() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncDIVIDE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncDIVIDE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncDIVIDE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncCAST : public _mthMatchFunc
   {
      public:
         _mthMatchFuncCAST( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncCAST() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         BSONType _castType ;
   } ;

   class _mthMatchFuncSLICE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSLICE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSLICE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;
         virtual INT32 adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                  _utilArray< INT32 > &out ) ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _begin ;
         INT32 _limit ;
   } ;

   class _mthMatchFuncSIZE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSIZE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSIZE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncTYPE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncTYPE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncTYPE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _resultType ;
   } ;

   class _mthMatchFuncRETURNMATCH : public _mthMatchFunc
   {
      public:
         _mthMatchFuncRETURNMATCH( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncRETURNMATCH() ;

      public:
         INT32 getOffset() ;
         INT32 getLen() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _offset ;
         INT32 _len ;
   } ;

   class _mthMatchFuncEXPAND : public _mthMatchFunc
   {
      public:
         _mthMatchFuncEXPAND( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncEXPAND() ;

      public:
         const CHAR* getFieldName() ;
         void getElement( BSONElement &ele ) ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   typedef _utilList< _mthMatchFunc* > MTH_FUNC_LIST ;
   class _mthMatchOpNode : public _mthMatchNode
   {
      public:
         _mthMatchOpNode( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNode() ;

      public: /* from parent */
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;

         virtual INT32 addChild( _mthMatchNode *child ) ;
         virtual void delChild( _mthMatchNode *child ) ;

         virtual void clear() ;

         virtual void setWeight( UINT32 weight ) ;

         virtual INT32 calcPredicate( _rtnPredicateSet &predicateSet ) ;

         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;

         virtual BOOLEAN isTotalConverted() ;

         virtual BOOLEAN hasDollarFieldName() ;

         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;

         virtual BSONObj toBson() ;

      public:
         INT32 addFunc( _mthMatchFunc *func ) ;
         INT32 addFuncList( MTH_FUNC_LIST &funcList ) ;
         void getFuncList( MTH_FUNC_LIST &funcList ) ;
         BOOLEAN hasReturnMatch() ;

      protected: /* from itself */
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) = 0 ;

         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element) ;
         virtual void _clear() ;

      protected:
         INT32 _execute( const CHAR *pFieldName, const BSONObj &obj,
                         BOOLEAN isArrayObj,
                         _mthMatchTreeContext &context,
                         BOOLEAN &result ) ;

         INT32 _dollarMatches( const CHAR *pFieldName,
                               const BSONElement &element,
                               _mthMatchTreeContext &context,
                               BOOLEAN &result ) ;

         INT32 _calculateFuncs( const BSONElement &in, BSONObj &out ) ;

         INT32 _doFuncMatch( const BSONElement &original,
                             const BSONElement &matchTarget,
                             _mthMatchTreeContext &context,
                             BOOLEAN &matchResult ) ;

         INT32 _saveElement( _mthMatchTreeContext &context,  BOOLEAN isMatch,
                             INT32 index ) ;

         BOOLEAN _isNot() ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            return FALSE ;
         }

      protected:
         MTH_FUNC_LIST _funcList ;
         BSONElement _toMatch ;
         BOOLEAN _isCompareField ;
         const CHAR *_cmpFieldName ;
         BOOLEAN _hasDollarFieldName ;

         BOOLEAN _hasReturnMatch ;
         BOOLEAN _hasExpand ;
         INT32 _offset ;
         INT32 _len ;
   } ;

   class _mthMatchOpNodeET : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeET(  _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeET() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeNE : public _mthMatchOpNodeET
   {
      public:
         _mthMatchOpNodeNE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeNE() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;
         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual void release() ;
   } ;

   class _mthMatchOpNodeLT : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeLT( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeLT() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeLTE : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeLTE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeLTE() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeGT : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeGT( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeGT() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeGTE : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeGTE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeGTE() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeRegex ;

   class _mthMatchOpNodeIN : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeIN( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeIN() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual void _clear() ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
      protected:
         BOOLEAN _isMatch( const BSONElement &ele ) ;


      protected:
         typedef set<BSONElement, element_cmp_lt> VALUE_SET ;
         set<BSONElement, element_cmp_lt> _valueSet ;

         typedef vector<_mthMatchOpNodeRegex *> REGEX_VECTOR ;
         REGEX_VECTOR _regexVector ;
   } ;

   class _mthMatchOpNodeNIN : public _mthMatchOpNodeIN
   {
      public:
         _mthMatchOpNodeNIN( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeNIN() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeALL : public _mthMatchOpNodeIN
   {
      public:
         _mthMatchOpNodeALL( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeALL() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         INT32 _valueMatchWithReturnMatch(  const BSONElement &left,
                                            const BSONElement &right,
                                            _mthMatchTreeContext &context,
                                            BOOLEAN &result )  ;

         BOOLEAN _valueMatchNoReturnMatch( const BSONElement &left,
                                           const BSONElement &right ) ;

         BOOLEAN _isMatchSingle( const BSONElement &ele ) ;
   } ;

   class _mthMatchOpNodeEXISTS : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeEXISTS( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeEXISTS() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         UINT32 getWeight() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            // For { $exists : 1 } return FALSE if undefined
            // For { $exists : 0 } return TRUE if undefined
            return _toMatch.trueValue() ? FALSE : TRUE ;
         }
   } ;

   class _mthMatchOpNodeMOD : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeMOD( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeMOD() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      protected:
         BOOLEAN _isModValid( const BSONElement &modmEle ) ;

      protected:
         BSONElement _mod ;
         BSONElement _modResult ;
   } ;

   class _mthMatchOpNodeTYPE : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeTYPE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeTYPE() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      protected:
         INT32 _type ;
   } ;

   class _mthMatchOpNodeISNULL : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeISNULL( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeISNULL() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            // For { $isnull : 1 } return TRUE if undefined
            // For { $isnull : 0 } return FALSE if undefined
           return _toMatch.trueValue() ? TRUE : FALSE ;
         }
   } ;

   class _mthMatchOpNodeEXPAND : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeEXPAND( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeEXPAND() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

         virtual INT32 calcPredicate( _rtnPredicateSet &predicateSet ) ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchTree ;
   class _mthMatchOpNodeELEMMATCH : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeELEMMATCH( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeELEMMATCH() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual void _clear() ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      protected:
         _mthMatchTree *_subTree ;
   } ;

   class _mthMatchOpNodeRegex : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeRegex( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchOpNodeRegex() ;

      public:
         //forbiddened
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;

         //use this to init _mthMatchOpNodeRegex.(not graceful here)
         INT32 init( const CHAR *fieldName, const CHAR *regex,
                     const CHAR *options ) ;

         virtual INT32 getType() ;
         virtual const CHAR *getOperatorStr() ;
         virtual BSONObj toBson() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual UINT32 getWeight() ;
         virtual void release() ;

      public:
         BOOLEAN matches( const BSONElement &ele ) ;

      protected:
         virtual void _clear() ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      private:
         pcrecpp::RE_Options _flags2options( const char* options ) ;
         BOOLEAN _isPureWords( const char* regex, const char* options ) ;

      private:
         const CHAR *_regex ;
         const CHAR *_options ;
         boost::shared_ptr<RE> _re ;
         BOOLEAN _isSimpleMatch ;
         BSONObj _matchObj ;
   } ;
}

#endif

