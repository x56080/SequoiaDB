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

   Source File Name = CollectionSpaceTest.cs

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

namespace DriverTest
{
    
    
    /// <summary>
    ///这是 CollectionSpaceTest 的测试类，旨在
    ///包含所有 CollectionSpaceTest 单元测试
    ///</summary>
    [TestClass()]
    public class CollectionSpaceTest
    {


        private TestContext testContextInstance;
        private static Config config = null;

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
        //
        //使用 ClassCleanup 在运行完类中的所有测试后再运行代码
        //[ClassCleanup()]
        //public static void MyClassCleanup()
        //{
        //}
        //
        //使用 TestInitialize 在运行每个测试前先运行代码
        //[TestInitialize()]
        //public void MyTestInitialize()
        //{
        //}
        //
        //使用 TestCleanup 在运行完每个测试后运行代码
        //[TestCleanup()]
        //public void MyTestCleanup()
        //{
        //}
        //
        #endregion


        [TestMethod()]
        public void CollectionTest()
        {
            string csName = "testCS1";
            string clName = "testCL1";
            CollectionSpace cs = null;
            Sequoiadb sdb = new Sequoiadb(config.conf.Coord.Address);
            sdb.Connect(config.conf.UserName, config.conf.Password);
            if(sdb.IsCollectionSpaceExist(csName))
                cs = sdb.GetCollecitonSpace(csName);
            else
                cs = sdb.CreateCollectionSpace(csName);
            if (!cs.IsCollectionExist(clName))
                cs.CreateCollection(clName);
            cs.DropCollection(clName);
            Assert.IsFalse(cs.IsCollectionExist(clName));
            sdb.DropCollectionSpace(csName);
            sdb.Disconnect();
        }
    }
}
