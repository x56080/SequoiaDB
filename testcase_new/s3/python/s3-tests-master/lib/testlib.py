# -- coding: utf-8 --
import unittest
import xmlrunner
from datetime import datetime
from s3tests_boto3.functional import(
    setup,
    teardown
)


class S3TestBase(unittest.TestCase):
    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)

    @classmethod
    def setUpClass(cls):
        print("\n" + cls.__name__ + " setup: " + str(datetime.now()))
        setup()


    @classmethod
    def tearDownClass(cls):
        teardown()
        print("\n" + cls.__name__ + " teardown: " + str(datetime.now()))

    def __is_testcase_success(self):
        """
        判断当前用例是否成功
        :param self:
        :return:
        """
        if hasattr(self, "_outcome"):
            # for python 3.6.2
            for x in self._outcome.errors:
                if x[1] != None:
                    return False
            return True
        elif hasattr(self, "_outcomeForDoCleanups"):
            # for python 3.3.4
            return self._outcomeForDoCleanups.success
        elif hasattr(self, "_resultForDoCleanups"):
            # for python 2 unittest
            failures = self._resultForDoCleanups.failures
            errors = self._resultForDoCleanups.errors
            l = []
            l.extend(failures)
            l.extend(errors)
            for x in l:
                if isinstance(x, xmlrunner._TestInfo) and self == x.test_method:
                    return False
            return True
        else:
            # can not judge this testcase success or failed ,so think it was success
            print("warn: can not judge this testcase success.")
            return True
