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

   Source File Name = DBLobTest.cs

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
﻿using SequoiaDB;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using SequoiaDB.Bson;
using System.IO;

namespace DriverTest
{
    
    
    /// <summary>
    ///这是 DBLobTest 的测试类，旨在
    ///包含所有 DBLobTest 单元测试
    ///</summary>
    [TestClass()]
    public class DBLobTest
    {

        private TestContext testContextInstance;
        private static Config config = null;

        Sequoiadb sdb = null;
        CollectionSpace cs = null;
        DBCollection cl = null;
        string csName = "lobtestfoo";
        string cName = "lobtestbar";

        public TestContext TestContext
        {
            get
            {
                return testContextInstance;
            }
            set
            {
                testContextInstance = value;
            }
        }

        #region 附加测试特性
        // 
        //编写测试时，还可使用以下特性:
        //
        //使用 ClassInitialize 在运行类中的第一个测试前先运行代码
        [ClassInitialize()]
        public static void SequoiadbInitialize(TestContext testContext)
        {
            if ( config == null )
                config = new Config();
        }

        //使用 ClassCleanup 在运行完类中的所有测试后再运行代码
        [ClassCleanup()]
        public static void MyClassCleanup()
        {
        }

        //使用 TestInitialize 在运行每个测试前先运行代码
        [TestInitialize()]
        public void MyTestInitialize()
        {
            try
            {
                sdb = new Sequoiadb(config.conf.Coord.Address);
                sdb.Connect(config.conf.UserName, config.conf.Password);
                if (sdb.IsCollectionSpaceExist(csName))
                    sdb.DropCollectionSpace(csName);
                cs = sdb.CreateCollectionSpace(csName);
                cl = cs.CreateCollection(cName);
            }
            catch (BaseException e)
            {
                Console.WriteLine("Failed to Initialize in DBLobTest, ErrorType = {0}", e.ErrorType);
                Environment.Exit(0);
            }
        }
        
        //使用 TestCleanup 在运行完每个测试后运行代码
        [TestCleanup()]
        public void MyTestCleanup()
        {
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
        
        #endregion

        [TestMethod()]
        public void LobGlobalTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            DBLob lob3 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            ObjectId oid2 = ObjectId.Empty;
            ObjectId oid3 = ObjectId.Empty;
            ObjectId oid4 = ObjectId.Empty;
            long size1 = 0;
            long time1 = 0;
            int bufSize = 1000;
            int readNum = 0;
            int retNum = 0;
            int offset = 0;
            int i = 0;
            byte[] readBuf = null;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }
            DBCursor cursor = null;
            BsonDocument record = null;
            long lobSize = 0;

            /// case 1: create a new lob
            // CreateLob
            lob = cl.CreateLob();
            Assert.IsNotNull(lob);
            // IsClosed
            flag = true;
            flag = lob.IsClosed();
            Assert.IsFalse(flag);
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write
            lob.Write(buf);
            // Close
            lob.Close();
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);

            // case 2: open an exsiting lob
            lob2 = cl.OpenLob(oid1);
            Assert.IsNotNull(lob2);
            // IsClosed
            flag = true;
            flag = lob2.IsClosed();
            Assert.IsFalse(flag);
            // GetID
            oid2 = lob2.GetID();
            Assert.IsTrue(ObjectId.Empty != oid2);
            Assert.IsTrue(oid1 == oid2);
            // GetSize
            size1 = lob2.GetSize();
            Assert.IsTrue(bufSize == size1);
            // GetCreateTime
            time1 = lob2.GetCreateTime();
            Assert.IsTrue(time1 > 0);
            // Read
            readNum = bufSize / 4;
            readBuf = new Byte[readNum];
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum);
            // Seek
            offset = bufSize / 2;
            lob2.Seek(offset, DBLob.SDB_LOB_SEEK_CUR);
            // Read
            retNum = 0;
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum);
            // Close
            lob2.Close();
            // IsClosed
            flag = false;
            flag = lob2.IsClosed();
            Assert.IsTrue(flag);

            /// case 3: create a lob with specified oid
            oid3 = ObjectId.GenerateNewId();
            lob3 = cl.CreateLob(oid3);
            Assert.IsNotNull(lob3);
            // GetID
            oid4 = lob3.GetID();
            Assert.IsTrue(ObjectId.Empty != oid4);
            Assert.IsTrue(oid3 == oid4);
            // Write
            lob3.Write(buf);
            // Close
            lob3.Close();
            // IsClosed
            flag = false;
            flag = lob3.IsClosed();
            Assert.IsTrue(flag);

            /// case 4: test api in cl
            // ListLobs
            cursor = cl.ListLobs();
            Assert.IsNotNull(cursor);
            i = 0;
            while(null != (record = cursor.Next()))
            {
                i++;
                if (record.Contains("Size") && record["Size"].IsInt64)
                {
                    lobSize = record["Size"].AsInt64;
                }
                else
                {
                    Assert.Fail();
                }
            }
            Assert.IsTrue(2 == i);
            // RemoveLobs
            cl.RemoveLob(oid3);
            // ListLobs
            cursor = cl.ListLobs();
            Assert.IsNotNull(cursor);
            i = 0;
            while (null != (record = cursor.Next()))
            {
                i++;
                if (record.Contains("Size") && record["Size"].IsInt64)
                {
                    lobSize = record["Size"].AsInt64;
                }
                else
                {
                    Assert.Fail();
                }
            }
            Assert.IsTrue(1 == i);
        }

        [TestMethod()]
        //[Ignore]
        public void LobWriteTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            ObjectId oid2 = ObjectId.Empty;
            ObjectId oid3 = ObjectId.Empty;
            ObjectId oid4 = ObjectId.Empty;
            long size1 = 0;
            int bufSize = 1000;
            int i = 0;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }

            /// case 1: write double times
            // CreateLob
            lob = cl.CreateLob();
            Assert.IsNotNull(lob);
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write, first time
            lob.Write(buf);
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Write the second time
            lob.Write(buf);
            size1 = lob.GetSize();
            Assert.AreEqual(bufSize * 2, size1);
            // Close
            lob.Close();
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);

            /// case 2: write large date
            bufSize = 1024*1024*100;
            byte[] buf2 = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf2[i] = 65;
            }

            // CreateLob
            lob2 = cl.CreateLob();
            Assert.IsNotNull(lob2);
            // Write, first time
            lob2.Write(buf2);
            size1 = lob2.GetSize();
            Assert.AreEqual(bufSize, size1);
            // Close
            lob2.Close();
            // IsClosed
            flag = false;
            flag = lob2.IsClosed();
            Assert.IsTrue(flag);
            // GetSize
            size1 = lob2.GetSize();
            Assert.AreEqual(bufSize, size1);
        }

        [TestMethod()]
        public void LobReadTest()
        {
            DBLob lob = null;
            DBLob lob2 = null;
            bool flag = false;
            ObjectId oid1 = ObjectId.Empty;
            int bufSize = 1024 * 1024 * 100;
            int readNum = 0;
            int retNum = 0;
            int i = 0;
            byte[] readBuf = null;
            byte[] buf = new byte[bufSize];
            for (i = 0; i < bufSize; i++)
            {
                buf[i] = 65;
            }
            long lobSize = 0;

            // CreateLob
            lob = cl.CreateLob();
            Assert.IsNotNull(lob);
            // GetID
            oid1 = lob.GetID();
            Assert.IsTrue(ObjectId.Empty != oid1);
            // Write
            lob.Write(buf);
            lobSize = lob.GetSize();
            Assert.AreEqual(bufSize, lobSize);
            // Close
            lob.Close();

            // Open lob
            lob2 = cl.OpenLob(oid1);
            lobSize = lob2.GetSize();
            Assert.AreEqual(bufSize, lobSize);
            // Read
            int skipNum = 1024*1024*50;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_SET);  // after this, the offset is 1024*1024*50
            readNum = 1024*1024*10;
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf);  // after this, the offset is 1024*1024*60
            Assert.IsTrue(readNum == retNum);
            // check
            for(i = 0; i < readBuf.Length; i++)
            {
                Assert.IsTrue(readBuf[i] == 65);
            }
            skipNum = 1024*1024*10;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_CUR); // after this, the offset is 1024*1024*70
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf);
            Assert.IsTrue(readNum == retNum); // after this, the offset is 1024*1024*80
            // check
            for(i = 0; i < readBuf.Length; i++)
            {
                Assert.IsTrue(readBuf[i] == 65);
            } 
            skipNum = 1024*1024*20;
            lob2.Seek(skipNum, DBLob.SDB_LOB_SEEK_END);
            readNum = 1024*1024*30; // will only read 1024*1024*20
            readBuf = new byte[readNum];
            retNum = lob2.Read(readBuf); // after this, the offset is 1024*1024*100
            Assert.AreEqual(readNum, (retNum + 1024 * 1024 * 10));

            // Close
            lob2.Close();
            // IsClosed
            flag = false;
            flag = lob.IsClosed();
            Assert.IsTrue(flag);
        }

        static int[] GenerateSequenceNumber(int Length)
        {
            int[] ret = new int[Length];
            for (int i = 0; i < Length; i++)
            {
                ret[i] = i + 1;
            }
            return ret;
        }

        [TestMethod]
        public void LobReadWriteSequenceNumber()
        {
            // gen data
            Random random = new Random();
            int size = random.Next(10 * 1024 * 1024);
            byte[] content_bytes = new byte[size * 4];
            int[] content = GenerateSequenceNumber(size);
            for (int i = 0; i < size; i++)
            {
                byte[] tmp_buf = System.BitConverter.GetBytes(content[i]);
                Array.Copy(tmp_buf, 0, content_bytes, i * 4, tmp_buf.Length);
            }
            //Console.WriteLine("content_bytes is: {0}", BitConverter.ToString(content_bytes));
                       
            int end = content_bytes.Length;
            int beg = 0;
            int len = end - beg;
            byte[] output_bytes = new byte[content_bytes.Length];
            output_bytes.Initialize();

            DBLob lob = null;
            DBLob lob2 = null;

            // write to lob
            try
            {
                lob = cl.CreateLob();
                lob.Write(content_bytes, beg, len);
            }
            finally
            {
                if (lob != null) lob.Close();
            }

            // read from lob
            ObjectId id = lob.GetID();

            try
            {
                lob2 = cl.OpenLob(id);
                lob2.Read(output_bytes, beg, len);
            }
            finally
            {
                if (lob2 != null) lob2.Close();
            }

            // check
            for (int i = 0; i < beg; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }

            for (int i = beg; i < end; i++)
            {
                try
                {
                    Assert.AreEqual(content_bytes[i], output_bytes[i],
                        string.Format("beg: {0}, end: {1}, len: {2}, i: {3}", beg, end, len, i));
                }
                catch (Exception e)
                {
                    Console.WriteLine("source: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(content_bytes[j]);
                    }
                    Console.WriteLine("target: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(output_bytes[j]);
                    }
                    throw e;
                }
            }

            for (int i = end; i < output_bytes.Length; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }
            
        }

        [TestMethod]
        public void LobReadWriteRandomNumber()
        {
            // gen data
            Random random = new Random();
            int size = random.Next(20 * 1024 * 1024);
            string content = Constants.GenerateRandomNumber(size);
            byte[] content_bytes = System.Text.Encoding.Default.GetBytes(content);
            int end = random.Next(content_bytes.Length);
            int beg = random.Next(end);
            int len = end - beg;
            byte[] output_bytes = new byte[content_bytes.Length];
            output_bytes.Initialize();

            DBLob lob = null;
            DBLob lob2 = null;

            // write to lob
            try
            {
                lob = cl.CreateLob();    
                lob.Write(content_bytes, beg, len);
            }
            finally
            {
                if (lob != null) lob.Close();
            }

            // read from lob
            ObjectId id = lob.GetID();
            try
            {
                lob2 = cl.OpenLob(id);
                lob2.Read(output_bytes, beg, len);
            }
            finally
            {
                if (lob2 != null) lob2.Close();
            }

            // check
            for (int i = 0; i < beg; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }

            for (int i = beg; i < end; i++)
            {
                try
                {
                    Assert.AreEqual(content_bytes[i], output_bytes[i],
                        string.Format("beg: {0}, end: {1}, len: {2}, i: {3}", beg, end, len, i));
                }
                catch (Exception e)
                {
                    Console.WriteLine("source: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(content_bytes[j]);
                    }
                    Console.WriteLine("target: ");
                    for (int j = i; j < i + 8; j++)
                    {
                        Console.WriteLine(output_bytes[j]);
                    }
                    throw e;
                }
            }

            for (int i = end; i < output_bytes.Length; i++)
            {
                Assert.AreEqual(0, output_bytes[i]);
            }
        }

        [TestMethod()]
        //[Ignore]
        public void LobAbnormalTest()
        {
            //// case 1: oid is null
            //// null can't convert to ObjectId, so, no need to worry about
            //ObjectId id = null;
            //cl.OpenLob(id);
        }

    }
}
