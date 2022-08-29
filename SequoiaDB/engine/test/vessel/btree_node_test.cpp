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
                               const strictBuffer &buffer,
                               CHAR *data,
                               const orderingWrapper &ord);

   public:
      btreeNodeTest(PAGE_ID nodeId,
                    UINT32 depth,
                    const strictBuffer &buffer,
                    CHAR *data)
          : btreeNodeBase(nodeId, depth, buffer), _data(data),_ctx(new btreeContextTest)
      {
      }
      virtual ~btreeNodeTest()
      {
         delete _ctx;
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
      node.reset(
          SDB_OSS_NEW btreeNodeTest(curPageID, depth, buffer, data.get() + PAGE_HEAD_SIZE));
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
      void checkPrefixSlotAndItemSlot(const btreeNodeTest &node)
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

   private:
   };

   btreeNodeTest btreeNodeTest::gen(PAGE_ID nodeId,
                                    UINT32 depth,
                                    const strictBuffer &buffer,
                                    CHAR *data,
                                    const orderingWrapper &ord)
   {
      INT32 rc = SDB_OK;
      btreeNodeTest node(0, 1, buffer, data + PAGE_HEAD_SIZE);
      initBtreeNodePage(btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      UINT32 nums = 500;
      std::vector<keyString> ksV;
      ksV.reserve(nums);
      UINT32 ksTotalSize = 0;
      for (UINT32 i = 0; i < nums; ++i)
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
         EXPECT_EQ(SDB_OK, rc);
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);

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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);

      bsb.appendIntOrLL("a", 0);
      bsb.appendIntOrLL("b", 0);
      bsb.append("c", "test_fixed_string4");
      globalIndexID indexId(1,1,1);
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
      3. 校验页元数据、prefixSlots的low，high和itemSlots的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      插入到的位置为5，校验得到预期结果
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);

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
   }

   /*
   Name: base_insert_expect_uncompressed
   Description: 
      向已压缩节点中插入entry，且预期不会使用已有的前缀
      1. 生成已压缩的b树节点
      2. 构建entry，预期其插入位置应为5，但不可以使用当前一项和前一项的前缀
      3. 校验页元数据、prefixSlots的low，high和itemSlots的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      插入到的位置为5，校验得到预期结果
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);

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
   }

   /*
   Name: base_insert_compact_when_has_prefixes
   Description: 
      页面compact操作测试
      1. 生成已压缩的b树节点
      2. 删除4次下标为1的元素，下标为0的前缀槽剩余1个索引；再次删除5次下标为1的元素，下标为1的前缀槽剩余0个索引
      3. 执行compact操作
      4. 校验页元数据、prefixSlots的low，high和itemSlots的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      页面前缀槽从100个减为99个，校验结果符合预期
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);
      for(UINT32 i = 0; i < 4; i++)
      {
         rc = node.destroyItem(1);
         ASSERT_EQ(SDB_OK, rc);
      }
      for(UINT32 i = 0; i < 5; i++)
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
   }

   /*
   Name: base_destroy
   Description: 
      删除指定下标的索引
      1. 生成已压缩的b树节点
      2. 删除4次下标为1的元素，下标为0的前缀槽剩余1个索引；再次删除5次下标为1的元素，下标为1的前缀槽剩余0个索引
      3. 校验页元数据、前缀槽的low，high和索引槽指向的前缀槽号
      4. 插入一个不会被压缩的索引，然后删除该索引，校验页元数据、前缀槽的low，high和索引槽指向的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);
      for(UINT32 i = 0; i < 4; i++)
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

      for(UINT32 i = 0; i < 5; i++)
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
   }

   /*
   Name: base_truncate
   Description: 
      truncate保留3个索引
      1. 生成已压缩的b树节点
      2. 执行truncate操作，保留3个索引
      3. 校验页元数据、前缀槽的low，high和索引槽指向的前缀槽号
   Input: 保留的索引个数3
   Output: 
   Expected Result: 
      页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期
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
      btreeNodeTest node = btreeNodeTest::gen(0, 0, buffer, data.get(), ord);
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
   }

   /*
   Name: base_split_one_tenth
   Description: 
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向末端写入索引，在空间不足时执行splitAndInsert操作，触发9:1分裂
      4. 校验分裂的右节点页面数据，校验右节点的页元数据、前缀槽的low，high和索引槽指向的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      右节点页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期
   */
   TEST_F(btree_node_test, base_split_one_tenth)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;
      UINT32 ksTotalSize = 0;
      UINT32 i = 0;
      while(TRUE)
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
         if(SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
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
      EXPECT_EQ(SDB_OK, rc);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      while(TRUE)
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
      btreeContextTest *ctx = node.getTreeCtx();
      CHAR * rightData = ctx->buffers[raisedKey.rightChild].get();
      strictBuffer rightBuffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          rightData + PAGE_HEAD_SIZE);
      btreeNodeTest rightNode(raisedKey.rightChild, 0, rightBuffer, rightData);
      EXPECT_EQ(node.isRoot(), FALSE);
      EXPECT_EQ(i, node.getItemCount() + rightNode.getItemCount());
      checkPrefixSlotAndItemSlot(node);
   }

   /*
   Name: base_split_one_second
   Description: 
      删除指定下标的索引
      1. 生成节点，向其中写入索引直到空间不足
      2. 执行recompress操作
      3. 向末端写入索引，在空间不足时执行splitAndInsert操作，触发1:1分裂
      4. 校验分裂的右节点页面数据，校验右节点的页元数据、前缀槽的low，high和索引槽指向的前缀槽号
   Input: 
   Output: 
   Expected Result: 
      右节点页面元数据、前缀槽low，high和索引槽指向的前缀槽号变更符合预期
   */
   TEST_F(btree_node_test, base_split_one_second)
   {
      INT32 rc = SDB_OK;
      unique_ptr<CHAR[]> data{new CHAR[btreeNodeTest::PAGE_SIZE]};
      strictBuffer buffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          data.get() + PAGE_HEAD_SIZE);
      recordID rid(0, 1);
      btreeNodeTest node(0, 1, buffer, data.get() + PAGE_HEAD_SIZE);
      initBtreeNodePage(btreeNodeTest::PAGE_SIZE, 0, 0, 1, 1, TRUE, TRUE, data.get());
      UINT32 nkeys = 3;
      orderingWrapper ord(0, nkeys);
      randomBsonGenerator bg;
      BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
      std::vector<keyString> ksV;
      UINT32 ksTotalSize = 0;
      UINT32 i = 0;
      while(TRUE)
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
         if(SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE == rc)
         {
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
      EXPECT_EQ(SDB_OK, rc);
      btreeSplitRaisedKey raisedKey;
      ksb.reset();
      bsb.reset();
      while(TRUE)
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
      btreeContextTest *ctx = node.getTreeCtx();
      CHAR * rightData = ctx->buffers[raisedKey.rightChild].get();
      strictBuffer rightBuffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          rightData + PAGE_HEAD_SIZE);
      btreeNodeTest rightNode(raisedKey.rightChild, 0, rightBuffer, rightData);
      EXPECT_EQ(node.isRoot(), FALSE);
      EXPECT_EQ(i, node.getItemCount() + rightNode.getItemCount());
      checkPrefixSlotAndItemSlot(rightNode);
   }
} // namespace vessel
} // namespace engine
