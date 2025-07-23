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

   Source File Name = sequoiaFSLRUList.cpp

   Descriptive Name = sequoiafs fuse file lru cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj   Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSLRULIST_HPP__
#define __SEQUOIAFSLRULIST_HPP__

#include "sequoiaFSCommon.hpp"

#define  REDUCE_LEVEL 2
#define  HOT_LEVEL 5

namespace sequoiafs
{
   class dirMetaNode 
   {
      public:
         dirMetaNode(int k, dirMeta* v)
            : key(k), 
              isHot(FALSE),
              count(0), 
              linkPre(NULL), 
              linkNext(NULL), 
              bucketPre(NULL), 
              bucketNext(NULL)
         {
            meta.setName(v->name());
            meta.setMode(v->mode());
            meta.setUid(v->uid());
            meta.setGid(v->gid());
            meta.setPid(v->pid());
            meta.setNLink(v->nLink());
            meta.setId(v->id());
            meta.setSize(v->size());
            meta.setAtime(v->atime());
            meta.setCtime(v->ctime());
            meta.setMtime(v->mtime());
            meta.setSymLink(v->symLink());
         }

         INT32 getKey(){return key;}
         
      public:
         INT32 key;
         bool isHot;
         INT32 count;
         dirMeta meta;
         dirMetaNode *linkPre;
         dirMetaNode *linkNext;
         dirMetaNode *bucketPre;
         dirMetaNode *bucketNext;
   };

   class LRUList : public SDBObject
   {
      public:
         LRUList();
         ~LRUList();
         void init(INT32 capa){_capacity = capa;}
         INT32 addCold(dirMetaNode* node);
         dirMetaNode* findReduceCold();
         INT32 del(dirMetaNode* node);

         INT32 getColdSize(){return _coldSize;}
         INT32 getHotSize(){return _hotSize;}
         
      private:   
         INT32 _coldToHot(dirMetaNode* node);
         INT32 _coldToCold(dirMetaNode* node);
         void _reduceHot();

      private:
         INT32 _capacity;
         INT32 _hotSize;
         INT32 _coldSize;
         dirMetaNode* _hotHead;
         dirMetaNode* _hotTail;
         dirMetaNode* _coldHead;
         dirMetaNode* _coldTail;
         ossSpinXLatch _mutex;  

         friend class dirMetaCache;
   };

}

#endif
