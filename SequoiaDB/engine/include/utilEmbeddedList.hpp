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

   Source File Name = utilEmbeddedList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_EMBEDDED_LIST_HPP_
#define UTIL_EMBEDDED_LIST_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"

namespace engine
{

#pragma pack(4)
   template<typename T>
   class utilEmbeddedListNode
   {
      template<typename NodeT, typename N, typename R> friend class utilEmbeddedList;
      public:
         utilEmbeddedListNode() = default;
         ~utilEmbeddedListNode() = default;

      public:
         void reset()
         {
            _pre = nullptr;
            _next = nullptr;
         }

      private:
         void _setNext(T *n)
         {
            _next = n;
         }
         void _setPre(T *n)
         {
            _pre = n;
         }
         void _set(T *pre, T *next)
         {
            _pre = pre;
            _next = next;
         }

         T *_getPre() {return _pre;}
         T *_getNext() {return _next;}

      private:
         T *_pre = nullptr;
         T *_next = nullptr;
   };//class utilEmbeddedListNode

   template<typename T>
   using UTIL_EMBEDDED_LIST_NODE = utilEmbeddedListNode<T>;

   template<typename T>
   struct utilEmListObjDeleter
   {
      void operator()(T *p)const
      {
         if (nullptr != p)
         {
            SDB_OSS_DEL p;
         }
      }
   };

   struct utilEmListPoolBlockDeleter
   {
      void operator()(void *p)const
      {
         if (nullptr != p)
         {
            SDB_THREAD_FREE(p);
         }
      }
   };

   struct utilEmListBlockDeleter
   {
      void operator()(void *p)const
      {
         if (nullptr != p)
         {
            SDB_OSS_FREE(p);
         }
      }
   };

   template<typename T, typename NodeReference, typename DeleteNode>
   class utilEmbeddedList
   {
      public:
         utilEmbeddedList() = default;
         ~utilEmbeddedList()
         {
            clear();
         }

         utilEmbeddedList(const utilEmbeddedList &) = delete;
         utilEmbeddedList &operator=(const utilEmbeddedList &) = delete;

         utilEmbeddedList(utilEmbeddedList &&o):
         _front(o._front),
         _back(o._back),
         _size(o._size)
         {
            o._release();
         }

         utilEmbeddedList &operator=(utilEmbeddedList &&o)
         {
            clear();
            _front = o._front;
            _back = o._back;
            _size = o._size;
            o._release();
            return *this;
         }

      public:
         using T_NODE_PTR = T*;

      public:
         UINT32 getSize()const {return _size;}
         BOOLEAN isEmpty()const {return 0 == _size;}

         T_NODE_PTR getFront() {return _front;}
         T_NODE_PTR getBack() {return _back;}
         T_NODE_PTR getNext(T_NODE_PTR node)
         {
            return _getNodeRef(node)->_getNext();
         }
         T_NODE_PTR getPre(T_NODE_PTR node)
         {
            return _getNodeRef(node)->_getPre();
         }

         void pushBack(T_NODE_PTR node);
         T_NODE_PTR popBack();

         void pushFront(T_NODE_PTR node);
         T_NODE_PTR popFront();

         void clear();

         /// node must be managed by current list!
         /// return the next node ptr.
         T_NODE_PTR erase(T_NODE_PTR node);

      private:
         void _release()
         {
            _size = 0;
            _front = nullptr;
            _back = nullptr;
         }

      private:
         NodeReference _getNodeRef = NodeReference();
         DeleteNode _deleteNode = DeleteNode();
         T_NODE_PTR _front = nullptr;
         T_NODE_PTR _back = nullptr;
         UINT32 _size = 0;
   };//class utilEmbeddedList

   template<typename T, typename NodeReference, typename DeleteNode>
   using UTIL_EMBEDDED_LIST = utilEmbeddedList<T, NodeReference, DeleteNode>;

   template<typename T, typename NodeReference, typename DeleteNode>
   void utilEmbeddedList<T, NodeReference, DeleteNode>::pushBack(T_NODE_PTR node)
   {
      SDB_ASSERT(nullptr != node, "can not be invalid");
      UTIL_EMBEDDED_LIST_NODE<T> *ref = _getNodeRef(node);
      SDB_ASSERT(nullptr == ref->_getPre(), "must be null");
      SDB_ASSERT(nullptr == ref->_getNext(), "must be null");

      if (nullptr != _back)
      {
         ref->_set(_back, nullptr);
         _getNodeRef(_back)->_setNext(node);
         _back = node;
      }
      else
      {
         ref->reset();
         _front = node;
         _back = node;
      }

      ++_size;
      return;
   }

   template<typename T, typename NodeReference, typename DeleteNode>
   typename utilEmbeddedList<T, NodeReference, DeleteNode>::T_NODE_PTR
   utilEmbeddedList<T, NodeReference, DeleteNode>::popBack()
   {
      T_NODE_PTR node = nullptr;
      
      if (!isEmpty())
      {
         node = _back;
         if (_front == _back)
         {
            _front = nullptr;
            _back = nullptr;
            _getNodeRef(node)->reset();
         }
         else
         {
            UTIL_EMBEDDED_LIST_NODE<T> *ref = _getNodeRef(node);
            T_NODE_PTR pre = ref->_getPre();
            ref->reset();

            _getNodeRef(pre)->_setNext(nullptr);
            _back = pre;
         }

         --_size;
      }

      return node;
   }

   template<typename T, typename NodeReference, typename DeleteNode>
   void utilEmbeddedList<T, NodeReference, DeleteNode>::pushFront(T_NODE_PTR node)
   {
      SDB_ASSERT(nullptr != node, "can not be invalid");
      UTIL_EMBEDDED_LIST_NODE<T> *ref = _getNodeRef(node);
      SDB_ASSERT(nullptr == ref->_getPre(), "must be null");
      SDB_ASSERT(nullptr == ref->_getNext(), "must be null");

      if (nullptr != _front)
      {
         ref->_set(nullptr, _front);
         _getNodeRef(_front)->_setPre(node);
         _front = node;
      }
      else
      {
         ref->reset();
         _front = node;
         _back = node;
      }

      ++_size;
      return;
   }

   template<typename T, typename NodeReference, typename DeleteNode>
   typename utilEmbeddedList<T, NodeReference, DeleteNode>::T_NODE_PTR
   utilEmbeddedList<T, NodeReference, DeleteNode>::popFront()
   {
      T_NODE_PTR node = nullptr;
      
      if (!isEmpty())
      {
         node = _front;
         if (_front == _back)
         {
            _front = nullptr;
            _back = nullptr;
            _getNodeRef(node)->reset();
         }
         else
         {
            UTIL_EMBEDDED_LIST_NODE<T> *ref = _getNodeRef(node);
            T_NODE_PTR next = ref->_getNext();
            ref->reset();

            _getNodeRef(next)->_setPre(nullptr);
            _front = next;
         }

         --_size;
      }

      return node;
   }

   template<typename T, typename NodeReference, typename DeleteNode>
   void utilEmbeddedList<T, NodeReference, DeleteNode>::clear()
   {
      while (!isEmpty())
      {
         _deleteNode(popFront());
      }
      return;
   }

   template<typename T, typename NodeReference, typename DeleteNode>
   typename utilEmbeddedList<T, NodeReference, DeleteNode>::T_NODE_PTR
   utilEmbeddedList<T, NodeReference, DeleteNode>::erase(T_NODE_PTR node)
   {
      SDB_ASSERT(0 < _size, "invalid size");
      SDB_ASSERT(nullptr != node, "can not be invalid");
      UTIL_EMBEDDED_LIST_NODE<T> *ref = _getNodeRef(node);
      T_NODE_PTR next = ref->_getNext();
      if (node == _front)
      {
         _deleteNode(popFront());
      }
      else if (node == _back)
      {
         _deleteNode(popBack());
      }
      else
      {
         T_NODE_PTR pre = ref->_getPre();
         T_NODE_PTR next = ref->_getNext();
         _getNodeRef(pre)->_setNext(next);
         _getNodeRef(next)->_setPre(pre);
         _deleteNode(node);
         
      }

      --_size;

      return next;
   }

#pragma pack()

} // namespace engine


#endif//UTIL_EMBEDDED_LIST_HPP_