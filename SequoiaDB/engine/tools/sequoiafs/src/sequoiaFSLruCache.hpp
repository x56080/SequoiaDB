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

   Source File Name = sequoiaFSLruCache.cpp

   Descriptive Name = sequoiafs fuse file lru cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
       03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSLRUCACHE_HPP__
#define __SEQUOIAFSLRUCACHE_HPP__

#include "sequoiaFS.hpp"
#include <iostream>
#include <map>

namespace sequoiafs
{
   class Node 
   {
   public:
      Node(int k, const struct dirMetaNode* v): key(k), prev(NULL), next(NULL)
      {
         node.name= v->name;
         node.mode= v->mode;
         node.uid = v->uid;
         node.gid = v->gid;
         node.pid= v->pid;
         node.nLink = v->nLink;
         node.id= v->id;
         node.size= v->size;
         node.atime= v->atime;
         node.ctime= v->ctime;
         node.mtime= v->mtime;
         node.symLink= v->symLink;
      }
   public:
      int key;
      struct dirMetaNode node;
      Node *prev, *next;
   };


   class DoublyLinkedList : public SDBObject
   {
      public:
         DoublyLinkedList(): front(NULL), rear(NULL){}
         Node* getRearDir()
         {
            return rear;
         }
         void removeRearDir();
         void moveDirToHead(Node *Dir);
         Node* addDirToHead(int key,const struct dirMetaNode* value);
         bool isEmpty()
         {
            return rear == NULL;
         }
      private:
         Node *front, *rear;
   };

   class LRUCache : public SDBObject
   {
      public:
         LRUCache(int capacity)
         {
            this->capacity = capacity;
            size = 0;
            dirList = new DoublyLinkedList();
            DirMap = map<int, Node*>();
         }

         ~LRUCache()
         {
            map<int, Node*>::iterator i1;
            for(i1=DirMap.begin();i1!=DirMap.end();i1++)
            {
               delete i1->second;
            }
            delete dirList;
         }
         int initMutex();
         void put(int key,const struct dirMetaNode * value);
         struct dirMetaNode * get(int key) ;

      private:
         int capacity, size;
         DoublyLinkedList *dirList;
         map<int, Node*> DirMap;

         pthread_mutex_t mutex;
         pthread_mutexattr_t attr;
   };

}

#endif
