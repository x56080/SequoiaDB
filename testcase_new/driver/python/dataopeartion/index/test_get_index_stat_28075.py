# -*- coding: utf-8 -*-
# @decription: seqDB-28075:getIndexStat获取索引统计信息指定detail参数
# @author:     liuli
# @createTime: 2022.10.02

from lib import testlib
from pysequoiadb.error import SDBBaseError

cs_name = "cs_28075"
cl_name = "cl_28075"
index_name = "index_28075"


class TestGetIndexStat28075(testlib.SdbTestBase):
    def setUp(self):
        testlib.drop_cs(self.db, cs_name, ignore_not_exist=True)
        self.cs = self.db.create_collection_space(cs_name)
        self.cl = self.cs.create_collection(cl_name)

    def test_get_index_stat_28075(self):
        # 插入数据并创建索引
        self.cl.insert({"_id": 1, "a": 1})
        self.cl.create_index_with_option({"a": 1}, index_name)
        # 收集对应的统计信息
        self.db.analyze({"Collection": cs_name + "." + cl_name, "Index": index_name})
        # 获取指定的索引统计信息并校验，指定detail为False
        actResult = self.cl.get_index_stat(index_name, False)
        del actResult["StatTimestamp"]
        expResult = {"Collection": cs_name + "." + cl_name, "Index": index_name, "Unique": False,
                     "KeyPattern": {"a": 1}, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [1],
                     "MinValue": {"a": 1}, "MaxValue": {"a": 1}, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 1,
                     "TotalRecords": 1}
        self.assertEqual(len(expResult), len(actResult),
                         "actResult : " + str(actResult) + ", expResult : " + str(expResult))
        for key in actResult:
            self.assertEqual(expResult[key], actResult[key],
                             "actResult : " + str(actResult) + ", expResult : " + str(expResult))

        # 获取指定的索引统计信息并校验，指定detail为True
        actResult = self.cl.get_index_stat(index_name, True)
        del actResult["StatTimestamp"]
        expResult = {"Collection": cs_name + "." + cl_name, "Index": index_name, "Unique": False,
                     "KeyPattern": {"a": 1}, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [1],
                     "MinValue": {"a": 1}, "MaxValue": {"a": 1}, "NullFrac": 0, "UndefFrac": 0,
                     "MCV": {"Values": [{"a": 1}], "Frac": [10000]}, "SampleRecords": 1, "TotalRecords": 1}
        self.assertEqual(len(expResult), len(actResult),
                         "actResult : " + str(actResult) + ", expResult : " + str(expResult))

        for key in actResult:
            self.assertEqual(expResult[key], actResult[key],
                             "actResult : " + str(actResult) + ", expResult : " + str(expResult))
        # 获取索引统计信息，指定detail为字符串
        try:
            self.cl.get_index_stat(index_name, "True")
            self.fail('should error but success')
        except SDBBaseError as e:
            if -6 != e.code:
                raise e

    def tearDown(self):
        testlib.drop_cs(self.db, cs_name, ignore_not_exist=True)
