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

   Source File Name = optQgmOptimizer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_OPTIMIZER_HPP_
#define OPT_QGM_OPTIMIZER_HPP_

#include "qgmOptiTree.hpp"
#include "optQgmStrategy.hpp"

namespace engine
{
   typedef std::multimap< UINT32, UINT32 >         DEL_NODES ;

   class _optQgmOptimizer : public SDBObject
   {
      public:
         _optQgmOptimizer () ;
         ~_optQgmOptimizer () ;

      public:
         INT32    adjust( qgmOptTree &orgTree ) ;
         INT32    optimize( qgmOptTree &orgTree ) ;

      protected:
         INT32    _downAdjust( qgmOptTree &orgTree, BOOLEAN &adjust,
                               DEL_NODES  &delNodes ) ;
         INT32    _upAdjust( qgmOptTree &orgTree, BOOLEAN &adjust,
                             DEL_NODES  &delNodes ) ;

         INT32    _adjustByOprUnit( qgmOprUnitPtrVec * oprUnitVec,
                                    qgmOptTree &orgTree,
                                    qgmOptiTreeNode *curNode,
                                    BOOLEAN &adjust ) ;

         INT32    _onStrategy( qgmOprUnit *oprUnit,
                               qgmOptiTreeNode *curNode,
                               qgmOptiTreeNode *subNode,
                               OPT_QGM_SS_RESULT &result ) ;

         INT32    _processSSResult( qgmOptTree &orgTree,
                                    qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT result,
                                    BOOLEAN &adjust ) ;

         INT32    _upbackOprUnit( qgmOprUnit * oprUnit,
                                  qgmOptTree &orgTree,
                                  qgmOptiTreeNode *curNode,
                                  BOOLEAN &adjust,
                                  DEL_NODES  &delNodes ) ;

      private:
         INT32    _formNewNode( qgmOptTree &orgTree,
                                qgmOprUnit *oprUnit,
                                qgmOptiTreeNode *curNode,
                                qgmOptiTreeNode *subNode,
                                qgmOptiTreeNode::PUSH_FROM from ) ;
         INT32    _prepareAdjust( qgmOptTree &orgTree ) ;
         INT32    _endAdjust( qgmOptTree &orgTree ) ;

   } ;
   typedef _optQgmOptimizer optQgmOptimizer ;

   optQgmOptimizer* getQgmOptimizer() ;

}

#endif //OPT_QGM_OPTIMIZER_HPP_
