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
      const btreeNodePageHead *getReadableHead()
      {
         return _getReadableHead();
      }
      prefixedKeyString getPrefixedKeyString(RECORD_SLOT_POS pos)
      {
         return _getPrefixedKeyString(pos);
      }

      const btreeNodePrefixSlot *getReadablePrefixSlot(INT16 pos)
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
      EXPECT_EQ(res.slotPos, 4);
   }

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
      EXPECT_EQ(res.slotPos, 4);
   }

   TEST_F(btree_node_test, base_insert_with_prefix)
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
   }

   TEST_F(btree_node_test, base_insert_without_prefix)
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
   }

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
   }

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
   }

   TEST_F(btree_node_test, base_truncate_1)
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
   }

   TEST_F(btree_node_test, base_truncate_2)
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
      UINT32 keptItemNum = 5;
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
   }

   TEST_F(btree_node_test, base_split)
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
         }else {
            DPS_TRANS_ID transID;
            rc = node.splitAndInsert(entry, transID, raisedKey);
            ASSERT_EQ(SDB_OK, rc);
            break;
         }
         ksTotalSize += BTREE_NODE_SLOT_SIZE + ksV.back().getRawDataSize();
         ksb.reset();
         bsb.reset();
         i++;
      }
      btreeContextTest *ctx = node.getTreeCtx();
      CHAR * rightData = ctx->buffers[raisedKey.rightChild].get();
      strictBuffer rightBuffer(btreeNodeTest::PAGE_SIZE - PAGE_HEAD_SIZE -
                              PAGE_TAIL_SIZE,
                          rightData + PAGE_HEAD_SIZE);
      btreeNodeTest rightNode(raisedKey.rightChild, 0, rightBuffer, rightData);
      EXPECT_EQ(i, node.getItemCount() + rightNode.getItemCount());
   }
} // namespace vessel
} // namespace engine
