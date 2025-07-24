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

   Source File Name = btree_node_test.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "randomBsonGenerator.h"
#include "vessel/btreeNodeBase.h"
#include "vessel/btreeNodePage.h"
#include "vessel/btreeSplitRaisedKey.h"
#include "vessel/btreeContext.h"
#include "vessel/globalIndexID.h"
#include "vessel/keyString.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/orderingWrapper.h"
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
namespace vessel
{
   class btreeNodeTest;
   class btreeContextTest : public btreeContext
   {
   public:
      virtual INT32 allocateNewNode(UINT32 depth,
                                    const btreeNodePageHead &header,
                                    BTREE_NODE_UPTR &node) override;

      virtual void destroyNode(BTREE_NODE_UPTR &node) override;

   public:
      PAGE_ID curPageID = 1;
      unordered_map<PAGE_ID, unique_ptr<CHAR[]>> buffers;
   };

   class btreeNodeTest : public btreeNodeBase
   {
      friend class btree_node_test;

   public:
      // Generate a node with 500 elements, which should be divided into 100
      // prefixes after compression.
      static btreeNodeTest gen(PAGE_ID nodeId,
                               UINT32 depth,
                               btreeContextTest *ctx,
                               const strictBuffer &buffer,
                               CHAR *data,
                               const orderingWrapper &ord);

   public:
      btreeNodeTest(PAGE_ID nodeId,
                    UINT32 depth,
                    btreeContextTest *ctx,
                    const strictBuffer &buffer,
                    CHAR *data)
          : btreeNodeBase(nodeId, depth, ctx, buffer), _data(data)
      {
      }
      virtual ~btreeNodeTest()
      {
      }

   public:
      INT32 compactWhenHasPrefixes()
      {
         return _compactWhenHasPrefixes();
      }
      INT32 truncate(UINT32 keptItemNum)
      {
         return _truncate(keptItemNum);
      }
      CHAR *getData() const
      {
         return _data;
      }
      const btreeNodePageHead *getReadableHead() const
      {
         return _getReadableHead();
      }
      prefixedKeyString getPrefixedKeyString(RECORD_SLOT_POS pos)
      {
         return _getPrefixedKeyString(pos);
      }

      const btreeNodePrefixSlot *getReadablePrefixSlot(INT16 pos) const
      {
         return _getReadablePrefixSlot(pos);
      }
      INT32 destroyItem(RECORD_SLOT_POS pos)
      {
         return _destroyItem(pos);
      }
      btreeContextTest *getTreeCtx()
      {
         return _ctx;
      }

   protected:
      virtual INT32 _makeBufferWritable()
      {
         _buffer.makeWritable(_buffer.getSize(), _data);
         return SDB_OK;
      }
      virtual btreeContext *_getTreeCtx()
      {
         return static_cast<btreeContext *>(_ctx);
      }

   public:
      static constexpr UINT32 PAGE_SIZE = 65536;

   private:
      CHAR *_data = nullptr;
      btreeContextTest *_ctx = nullptr;
   };

   INT32 btreeContextTest::allocateNewNode(UINT32 depth,
                                           const btreeNodePageHead &header,
                                           BTREE_NODE_UPTR &node)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data(new CHAR[btreeNodeTest::PAGE_SIZE]);
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      node.reset(SDB_OSS_NEW btreeNodeTest(
          curPageID, depth, this, buffer, data.get() + PAGE_HEAD_SIZE));
      buffers.emplace(curPageID, std::move(data));
      return rc;
   }

   void btreeContextTest::destroyNode(BTREE_NODE_UPTR &node)
   {
      buffers.erase(node->getNodeId());
   }

   class btree_node_test : public testing::Test
   {
   public:
      static void SetUpTestCase()
      {
         sdbEnablePD("/opt/diaglog/sdb.log", 1, 1000);
         setPDLevel(PDDEBUG);
      }

      static void TearDownTestCase()
      {
      }

      virtual void SetUp()
      {
      }

      // 测试前缀槽的low high是否与索引槽的prefixSlot匹配
      static void checkPrefixSlotAndItemSlot(const btreeNodeTest &node)
      {

         INT16 low = -1;
         INT16 high = -1;
         RECORD_SLOT_POS curPrefixPos = INVALID_RECORD_SLOT_POS;
         for (UINT32 i = 0; i < node.getItemCount(); ++i)
         {
            const btreeItemSlot slot = node.getItemSlot(i);
            if (curPrefixPos != slot.data.lf.prefixSlot)
            {
               low = i;
               high = i + 1;
               curPrefixPos = slot.data.lf.prefixSlot;
            }
            else
            {
               high += 1;
            }
            if (curPrefixPos != INVALID_RECORD_SLOT_POS)
            {
               if (i + 1 == node.getItemCount() ||
                   curPrefixPos != node.getItemSlot(i + 1).data.lf.prefixSlot)
               {
                  EXPECT_EQ(curPrefixPos, slot.data.lf.prefixSlot);
                  const btreeNodePrefixSlot *prefixSlot =
                      node.getReadablePrefixSlot(slot.data.lf.prefixSlot);
                  EXPECT_EQ(prefixSlot->low, low);
                  EXPECT_EQ(prefixSlot->high, high);
               }
            }
         }
      }

      static vector<keyString> genKsVector()
      {
         std::vector<keyString> ksV;
         keyStringBuilder<> ksb;
         BSONObjBuilder bsb;
         UINT32 nums = 500;
         ksV.reserve(nums);
         UINT32 ksTotalSize = 0;
         orderingWrapper ord(0, 3);
         for (UINT32 i = 0; i < nums; ++i)
         {
            INT32 rc = SDB_OK;
            bsb.appendIntOrLL("a", i / 20);
            bsb.appendIntOrLL("b", i / 5);
            bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
            recordID rid(0, i);
            ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
            keyString ks = ksb.getShallowKeyString();
            ksV.push_back(ks);
            ksV.back().getOwned();
            btreeKeyStringEntry entry(ksV.back().getDataSlice());
            EXPECT_EQ(SDB_OK, rc);
            ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
            ksb.reset();
            bsb.reset();
         }
         return ksV;
      }

      static BOOLEAN checkItems(const vector<keyString>::const_iterator &begin,
                                const vector<keyString>::const_iterator &end,
                                const btreeNodeTest &node)
      {
         EXPECT_EQ(node.getItemCount(), std::distance(begin, end));
         if (node.getItemCount() != std::distance(begin, end))
         {
            return FALSE;
         }
         for (auto it = begin; it != end; ++it)
         {
            UINT32 i = std::distance(begin, it);
            btreeKeyStringEntry entry;
            node.getOwnedEntry(i, entry);
            if (entry.compare(*it) != 0)
            {
               return FALSE;
            }
         }
         return TRUE;
      }

      static BOOLEAN checkItems(const vector<keyString> &ksV,
                                const btreeNodeTest &node)
      {
         return checkItems(ksV.begin(), ksV.cend(), node);
      }

   public:
      btreeContextTest ctx;
   };

   btreeNodeTest btreeNodeTest::gen(PAGE_ID nodeId,
                                    UINT32 depth,
                                    btreeContextTest *ctx,
                                    const strictBuffer &buffer,
                                    CHAR *data,
                                    const orderingWrapper &ord)
   {
      INT32 rc = SDB_OK;
      btreeNodeTest node(0, 1, ctx, buffer, data + PAGE_HEAD_SIZE);
      initBtreeNodePage(btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV = btree_node_test::genKsVector();
      UINT32 nums = ksV.size();
      ksV.reserve(nums);
      UINT32 ksTotalSize = 0;
      for (UINT32 i = 0; i < nums; ++i)
      {
         btreeKeyStringEntry entry(ksV[i].getDataSlice());
         rc = node.insert(entry);
         EXPECT_EQ(SDB_OK, rc);
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV[i].getRawDataSize();
      }
      const btreeNodePageHead *head = node.getReadableHead();
      EXPECT_EQ(head->totalFreeSpace,
                PAGE_SIZE - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE -
                    BTREE_NODE_PAGE_HEAD_SIZE - ksTotalSize);

      BOOLEAN recompressed = FALSE;
      rc = node.recompress(recompressed);
      EXPECT_EQ(SDB_OK, rc);
      for (UINT32 i = 0; i < nums; ++i)
      {
         prefixedKeyString pks = node.getPrefixedKeyString(i);
         keyString ks = pks.getOwnedKeyString();
         EXPECT_EQ(ks.isValid(), TRUE);
         EXPECT_EQ(ks.compare(ksV[i]), 0);
         EXPECT_EQ(node.getItemSlot(i).isKeyCompressed(), TRUE);
         EXPECT_EQ(node.getItemSlot(i).data.lf.prefixSlot, i / 5);
      }
      return node;
   }

   /*
   Name: base_locate
   Description:
      查找大于等于待查找的第一个entry
      1. 生成已压缩的b树节点
      2. 构建与下标为4的entry完全相同的查找条件
      3. 查找并校验定位的位置
   Input: 待查找entry
   Output: 查找结果
   Expected Result:
      查找结果entry在预期的位置4
   */
   TEST_F(btree_node_test, base_locate)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);

      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_string4");
      ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
      keyString ks = ksb.getShallowKeyString();
      btreeKeyStringEntry entry(ks.getDataSlice());
      rc = entry.getOwned();
      ASSERT_EQ(SDB_OK, rc);
      btreeNodeSeekResult res;
      rc = node.locateEntry(entry, res);
      ASSERT_EQ(SDB_OK, rc);
      EXPECT_EQ(res.getPos(), 4);
      checkPrefixSlotAndItemSlot(node);
   }

   /*
   Name: base_seek_ks_with_head
   Description:
      查找大于等于待查找的第一个entry，且输入的keyString包含header部分
      1. 生成已压缩的b树节点
      2. 构建与下标为4的entry在header部分后相同的keyString查找条件，header部分不为空
      3. 查找并校验定位的位置
   Input: 有header部分的待查找keyString
   Output: 查找结果
   Expected Result:
      自动去除header部分进行查找，待查找keyString在预期的位置4
   */
   TEST_F(btree_node_test, base_seek_ks_with_head)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);

      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_string4");
      globalIndexID indexId(1, 1, 1);
      ksb.buildIndexEntryKey(bsb.obj(), ord, rid, &indexId);
      keyString ks = ksb.getShallowKeyString();
      rc = ks.getOwned();
      ASSERT_EQ(SDB_OK, rc);
      btreeNodeSeekResult res;
      rc = node.seek(ks, res);
      ASSERT_EQ(SDB_OK, rc);
      EXPECT_EQ(res.getPos(), 4);
      checkPrefixSlotAndItemSlot(node);
   }

   /*
   Name: base_insert_expect_compressed
   Description:
      向已压缩节点中插入entry，且预期会使用已有的前缀
      1. 生成已压缩的b树节点
      2. 构建entry，预期其插入位置应为5，且可以使用前一项的前缀
      3. 校验页元数据、前缀槽的low，high和索引槽前缀槽号、索引记录键值
   Input:
   Output:
   Expected Result:
      插入到的位置为5，页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，校验得到预期结果
   */
   TEST_F(btree_node_test, base_insert_expect_compressed)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);

      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_string5");
      ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
      keyString ks = ksb.getShallowKeyString();
      btreeKeyStringEntry entry(ks.getDataSlice());
      entry.getOwned();
      rc = node.insert(entry);
      ASSERT_EQ(SDB_OK, rc);
      RECORD_SLOT_POS expectedPos = 5;
      prefixedKeyString pks = node.getPrefixedKeyString(expectedPos);
      keyString ksInserted = pks.getOwnedKeyString();
      ASSERT_EQ(ksInserted.isValid(), TRUE);
      EXPECT_EQ(ksInserted.compare(entry), 0);
      EXPECT_EQ(node.getItemSlot(expectedPos).data.lf.prefixSlot, 0);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 501);
      checkPrefixSlotAndItemSlot(node);
      vector<keyString> ksV = genKsVector();
      ksV.insert(ksV.begin() + 5, ks);
      ASSERT_EQ(checkItems(ksV, node), TRUE);
   }

   /*
   Name: base_insert_expect_uncompressed
   Description:
      向已压缩节点中插入entry，且预期不会使用已有的前缀
      1. 生成已压缩的b树节点
      2. 构建entry，预期其插入位置应为5，但不可以使用当前一项和前一项的前缀
      3. 校验页元数据、前缀槽的low，high和索引槽前缀槽号、索引记录键值
   Input:
   Output:
   Expected Result:
      插入到的位置为5，页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，校验得到预期结果
   */
   TEST_F(btree_node_test, base_insert_expect_uncompressed)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);

      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_strinh");
      ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
      keyString ks = ksb.getShallowKeyString();
      btreeKeyStringEntry entry(ks.getDataSlice());
      entry.getOwned();
      rc = node.insert(entry);
      ASSERT_EQ(SDB_OK, rc);
      RECORD_SLOT_POS expectedPos = 5;
      prefixedKeyString pks = node.getPrefixedKeyString(expectedPos);
      keyString ksInserted = pks.getOwnedKeyString();
      ASSERT_EQ(ksInserted.isValid(), TRUE);
      EXPECT_EQ(ksInserted.compare(entry), 0);
      EXPECT_EQ(node.getItemSlot(expectedPos).data.lf.prefixSlot, -1);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500);
      checkPrefixSlotAndItemSlot(node);
      vector<keyString> ksV = genKsVector();
      ksV.insert(ksV.begin() + 5, ks);
      ASSERT_EQ(checkItems(ksV, node), TRUE);
   }

   /*
   Name: base_insert_compact_when_has_prefixes
   Description:
      页面compact操作测试
      1. 生成已压缩的b树节点
      2. 删除4次下标为1的元素，下标为0的前缀槽剩余1个索引；再次删除5次下标为1的元素，
   下标为1的前缀槽剩余0个索引
      3. 执行compact操作
      4. 校验页元数据、前缀槽的low，high和索引槽前缀槽号、索引记录键值
   Input:
   Output:
   Expected Result:
      页面前缀槽从100个减为99个，页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，校验得到预期结果
   */
   TEST_F(btree_node_test, base_compact_when_has_prefixes)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);
      for (UINT32 i = 0; i < 4; i++)
      {
         rc = node.destroyItem(1);
         ASSERT_EQ(SDB_OK, rc);
      }
      for (UINT32 i = 0; i < 5; i++)
      {
         rc = node.destroyItem(1);
         ASSERT_EQ(SDB_OK, rc);
      }
      rc = node.compactWhenHasPrefixes();
      ASSERT_EQ(SDB_OK, rc);
      EXPECT_EQ(node.getReadablePrefixSlot(0)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->high, 6);
      EXPECT_EQ(node.getItemSlot(0).data.lf.prefixSlot, 0);
      EXPECT_EQ(node.getItemSlot(1).data.lf.prefixSlot, 1);
      EXPECT_EQ(node.getPrefixCount(), 99);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500 - 4 - 5);
      checkPrefixSlotAndItemSlot(node);
      vector<keyString> ksV = genKsVector();
      for (UINT32 i = 0; i < 9; i++)
      {
         ksV.erase(ksV.begin() + 1);
      }
      ASSERT_EQ(checkItems(ksV, node), TRUE);
   }

   /*
   Name: base_destroy
   Description:
      删除指定下标的索引
      1. 生成已压缩的b树节点
      2. 删除4次下标为1的元素，下标为0的前缀槽剩余1个索引；再次删除5次下标为1的元素，下标为1的前缀槽剩余0个索引
      3. 校验页元数据、前缀槽的low，high和索引槽指向的前缀槽号
      4. 插入一个不会被压缩的索引，然后删除该索引，校验页元数据、前缀槽的low，high和索引槽指向的前缀槽号
      5. 校验页元数据、前缀槽的low，high和索引槽前缀槽号、索引记录键值
   Input:
   Output:
   Expected Result:
      页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，校验得到预期结果
   */
   TEST_F(btree_node_test, base_destroy)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);
      for (UINT32 i = 0; i < 4; i++)
      {
         rc = node.destroyItem(1);
         ASSERT_EQ(SDB_OK, rc);
      }
      EXPECT_EQ(node.getReadablePrefixSlot(0)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->high, 6);
      EXPECT_EQ(node.getItemSlot(0).data.lf.prefixSlot, 0);
      EXPECT_EQ(node.getItemSlot(1).data.lf.prefixSlot, 1);
      EXPECT_EQ(node.getItemCount(), 500 - 4);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500 - 4);

      for (UINT32 i = 0; i < 5; i++)
      {
         rc = node.destroyItem(1);
         ASSERT_EQ(SDB_OK, rc);
      }
      EXPECT_EQ(node.getReadablePrefixSlot(0)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(2)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(2)->high, 6);
      EXPECT_EQ(node.getItemSlot(0).data.lf.prefixSlot, 0);
      EXPECT_EQ(node.getItemSlot(1).data.lf.prefixSlot, 2);
      EXPECT_EQ(node.getItemCount(), 500 - 4 - 5);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500 - 4 - 5);
      checkPrefixSlotAndItemSlot(node);

      bsb.reset();
      ksb.reset();
      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_strinh");
      ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
      keyString ks = ksb.getShallowKeyString();
      btreeKeyStringEntry entry(ks.getDataSlice());
      entry.getOwned();
      rc = node.insert(entry);
      ASSERT_EQ(SDB_OK, rc);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500 - 4 - 5);
      btreeNodeSeekResult res;
      rc = node.locateEntry(entry, res);
      ASSERT_EQ(SDB_OK, rc);
      rc = node.destroyItem(res.getPos());
      ASSERT_EQ(SDB_OK, rc);
      EXPECT_EQ(node.getReadablePrefixSlot(0)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(1)->high, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(2)->low, 1);
      EXPECT_EQ(node.getReadablePrefixSlot(2)->high, 6);
      EXPECT_EQ(node.getItemSlot(0).data.lf.prefixSlot, 0);
      EXPECT_EQ(node.getItemSlot(1).data.lf.prefixSlot, 2);
      EXPECT_EQ(node.getItemCount(), 500 - 4 - 5);
      EXPECT_EQ(node.getReadableHead()->compressedItemCount, 500 - 4 - 5);
      checkPrefixSlotAndItemSlot(node);
      vector<keyString> ksV = genKsVector();
      for (UINT32 i = 0; i < 9; i++)
      {
         ksV.erase(ksV.begin() + 1);
      }
      ASSERT_EQ(checkItems(ksV, node), TRUE);
   }

   /*
   Name: base_truncate
   Description:
      truncate保留3个索引
      1. 生成已压缩的b树节点
      2. 执行truncate操作，保留3个索引
      3. 校验页元数据、前缀槽的low，high和索引槽前缀槽号、索引记录键值
   Input: 保留的索引个数3
   Output:
   Expected Result:
      页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，校验得到预期结果
   */
   TEST_F(btree_node_test, base_truncate)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      btreeNodeTest node =
          btreeNodeTest::gen(0, 0, &ctx, buffer, data.get(), ord);
      UINT32 keptItemNum = 3;
      rc = node.truncate(keptItemNum);
      ASSERT_EQ(SDB_OK, rc);
      const btreeNodePageHead *head = node.getReadableHead();
      EXPECT_EQ(head->compressedItemCount, keptItemNum);
      EXPECT_EQ(head->totalSlotCount, keptItemNum);
      INT32 sumSize = 0;
      for (UINT32 i = 0; i < keptItemNum; i++)
      {
         prefixedKeyString pks = node.getPrefixedKeyString(i);
         sumSize += pks.getSuffix().size() + BTREE_NODE_SLOT_SIZE;
      }

      for (UINT32 i = 0; i < head->prefixCount; i++)
      {
         const btreeNodePrefixSlot *slot = node.getReadablePrefixSlot(i);
         sumSize += slot->prefixSize + BTREE_NODE_PREFIX_SLOT_SIZE;
         if (i != 0)
         {
            EXPECT_EQ(slot->getRefCnt(), 0);
            EXPECT_EQ(slot->low, i * 5);
            EXPECT_EQ(slot->high, i * 5);
         }
         else
         {
            EXPECT_EQ(slot->getRefCnt(), keptItemNum);
            EXPECT_EQ(slot->low, 0);
            EXPECT_EQ(slot->high, keptItemNum);
         }
      }

      EXPECT_EQ(head->totalFreeSpace,
                btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE -
                    BTREE_NODE_PAGE_HEAD_SIZE - sumSize);
      checkPrefixSlotAndItemSlot(node);
      vector<keyString> ksV = genKsVector();
      ksV.resize(keptItemNum);
      ASSERT_EQ(checkItems(ksV, node), TRUE);
   }

   /*
   Name: base_split_one_tenth
   Description:
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向末端写入索引，在空间不足时执行splitAndInsert操作，触发9:1分裂
      校验分裂的右节点页面数据，校验右节点的页元数据、前缀槽的low，high和索引槽指向的
   前缀槽号、索引记录
   Input:
   Output:
   Expected Result:
      右节点页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，当前节点与
   右节点索引记录与插入的所有索引记录匹配
   */
   TEST_F(btree_node_test, base_split_one_tenth)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, &ctx, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(
          btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;
      UINT32 ksTotalSize = 0;
      UINT32 i = 0;
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20);
         bsb.appendIntOrLL("b", i / 5);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
         i++;
      }
      const btreeNodePageHead *head = node.getReadableHead();
      EXPECT_EQ(head->totalFreeSpace,
                btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE -
                    BTREE_NODE_PAGE_HEAD_SIZE - ksTotalSize);
      BOOLEAN recompressed = FALSE;
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20);
         bsb.appendIntOrLL("b", i / 5);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         if (node.hasFreeSpaceToInsert(entry.getRawDataSize()))
         {
            rc = node.insert(entry);
            ASSERT_EQ(SDB_OK, rc);
            i++;
         }
         else
         {
            DPS_TRANS_ID transID;
            rc = node.splitAndInsert(entry, transID, raisedKey);
            ASSERT_EQ(SDB_OK, rc);
            i++;
            break;
         }
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
      }
      std::sort(ksV.begin(),
                ksV.end(),
                [&](const keyString &l, const keyString &r) -> BOOLEAN {
                   if (l.compare(r) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });
      CHAR *rightData = ctx.buffers[raisedKey.rightChild].get();
      strictBuffer rightBuffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                                   PAGE_TAIL_SIZE,
                               rightData + PAGE_HEAD_SIZE);
      btreeNodeTest rightNode(
          raisedKey.rightChild, 0, &ctx, rightBuffer, rightData);
      EXPECT_EQ(node.isRoot(), FALSE);
      EXPECT_EQ(i, node.getItemCount() + rightNode.getItemCount() + 1);
      checkPrefixSlotAndItemSlot(node);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
      ASSERT_EQ(
          checkItems(ksV.cbegin() + node.getReadableHead()->totalSlotCount + 1,
                     ksV.cend(),
                     rightNode),
          TRUE);
   }

   /*
   Name: base_split_one_second
   Description:
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向末端写入索引，在空间不足时执行splitAndInsert操作，触发1:1分裂
      校验分裂的右节点页面数据，校验右节点的页元数据、前缀槽的low，high和索引槽指向的前缀
   槽号、索引记录
   Input:
   Output:
   Expected Result:
      右节点页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期，当前节点与
   右节点索引记录与插入的所有索引记录匹配
   */
   TEST_F(btree_node_test, base_split_one_second)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, &ctx, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(
          btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;
      UINT32 ksTotalSize = 0;
      UINT32 i = 0;
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20);
         bsb.appendIntOrLL("b", i / 5);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
         i++;
      }
      const btreeNodePageHead *head = node.getReadableHead();
      EXPECT_EQ(head->totalFreeSpace,
                btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE -
                    BTREE_NODE_PAGE_HEAD_SIZE - ksTotalSize);
      BOOLEAN recompressed = FALSE;
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      std::default_random_engine generator;
      std::uniform_int_distribution<INT32> distribution(0, i / 5);
      while (TRUE)
      {
         bsb.appendIntOrLL("a", distribution(generator));
         bsb.appendIntOrLL("b", distribution(generator));
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         if (node.hasFreeSpaceToInsert(entry.getRawDataSize()))
         {
            rc = node.insert(entry);
            ASSERT_EQ(SDB_OK, rc);
            i++;
         }
         else
         {
            DPS_TRANS_ID transID;
            rc = node.splitAndInsert(entry, transID, raisedKey);
            ASSERT_EQ(SDB_OK, rc);
            i++;
            break;
         }
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
      }
      std::sort(ksV.begin(),
                ksV.end(),
                [&](const keyString &l, const keyString &r) -> BOOLEAN {
                   if (l.compare(r) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });
      CHAR *rightData = ctx.buffers[raisedKey.rightChild].get();
      strictBuffer rightBuffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                                   PAGE_TAIL_SIZE,
                               rightData + PAGE_HEAD_SIZE);
      btreeNodeTest rightNode(
          raisedKey.rightChild, 0, &ctx, rightBuffer, rightData);
      EXPECT_EQ(node.isRoot(), FALSE);
      EXPECT_EQ(i, node.getItemCount() + rightNode.getItemCount() + 1);
      checkPrefixSlotAndItemSlot(rightNode);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
      ASSERT_EQ(
          checkItems(ksV.cbegin() + node.getReadableHead()->totalSlotCount + 1,
                     ksV.cend(),
                     rightNode),
          TRUE);
   }

   /*
   Name: base_recompress_1
   Description:
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向节点中间写入不会使用已有前缀的索引，在空间不足时再次执行recompress操作，
   重新压缩，预期使用部分已有的前缀,预期返回值recompressed为TRUE
      4. 再次执行recompress操作，并启用fullyRegenerate参数为TRUE，预期返回值recompressed为TRUE
      5. 校验节点所有索引记录、页面元数据、前缀槽low，high和索引槽指向的前缀槽号
   Input:
   Output:
   Expected Result:
      节点索引记录与插入的所有索引记录匹配，页面元数据、前缀槽low，high和索引槽指向的
   前缀槽号变更符合预期
   */
   TEST_F(btree_node_test, base_recompress_1)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, &ctx, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(
          btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;

      UINT32 i = 0;
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20);
         bsb.appendIntOrLL("b", i / 5);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         ksb.reset();
         bsb.reset();
         i++;
      }
      BOOLEAN recompressed = FALSE;
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(recompressed, TRUE);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20 / 2);
         bsb.appendIntOrLL("b", i / 5 / 2);
         bsb.append("c", "test_fixed_strinh" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         i++;
         ksb.reset();
         bsb.reset();
      }
      std::sort(ksV.begin(),
                ksV.end(),
                [&](const keyString &l, const keyString &r) -> BOOLEAN {
                   if (l.compare(r) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(recompressed, TRUE);
      checkPrefixSlotAndItemSlot(node);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
      rc = node.recompress(recompressed, TRUE);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(recompressed, TRUE);
      checkPrefixSlotAndItemSlot(node);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
   }

   /*
   Name: base_recompress_2
   Description:
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向节点中间写入会使用已有前缀的索引，在空间不足时再次执行recompress操作，
   重新压缩，预期使用所有已有的前缀
      4. 再次执行recompress操作，并启用fullyRegenerate参数为TRUE
      5. 校验节点所有索引记录、页面元数据、前缀槽low，high和索引槽指向的前缀槽号
   Input:
   Output:
   Expected Result:
      节点索引记录与插入的所有索引记录匹配，页面元数据、前缀槽low，high和索引槽指向的
   前缀槽号变更符合预期
   */
   TEST_F(btree_node_test, base_recompress_2)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, &ctx, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(
          btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;

      UINT32 i = 0;
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20);
         bsb.appendIntOrLL("b", i / 5);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         ksb.reset();
         bsb.reset();
         i++;
      }
      BOOLEAN recompressed = FALSE;
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(recompressed, TRUE);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      while (TRUE)
      {
         bsb.appendIntOrLL("a", i / 20 / 2);
         bsb.appendIntOrLL("b", i / 5 / 2);
         bsb.append("c", "test_fixed_string" + std::to_string(i % 5));
         recordID rid(0, i);
         ksb.buildIndexEntryKey(bsb.obj(), ord, rid);
         keyString ks = ksb.getShallowKeyString();
         ksV.push_back(ks);
         ksV.back().getOwned();
         btreeKeyStringEntry entry(ksV.back().getDataSlice());
         rc = node.insert(entry);
         if (SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
            ksV.pop_back();
            break;
         }
         ASSERT_EQ(SDB_OK, rc);
         i++;
         ksb.reset();
         bsb.reset();
      }
      std::sort(ksV.begin(),
                ksV.end(),
                [&](const keyString &l, const keyString &r) -> BOOLEAN {
                   if (l.compare(r) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });
      rc = node.recompress(recompressed);
      ASSERT_EQ(SDB_OK, rc);
      checkPrefixSlotAndItemSlot(node);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
      rc = node.recompress(recompressed, TRUE);
      ASSERT_EQ(SDB_OK, rc);
      checkPrefixSlotAndItemSlot(node);
      ASSERT_EQ(
          checkItems(ksV.cbegin(),
                     ksV.cbegin() + node.getReadableHead()->totalSlotCount,
                     node),
          TRUE);
   }
} // namespace vessel
} // namespace engine