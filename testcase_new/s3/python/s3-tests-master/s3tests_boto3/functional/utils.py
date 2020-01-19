import random
import string


def assert_raises(excClass, callableObj, *args, **kwargs):
    """
    * tuple
    ** dict
    Like unittest.TestCase.assertRaises, but returns the exception.
    """
    try:
        callableObj(*args, **kwargs)
    except excClass as e:
        return e
    except Exception as e:
        return e
    else:
        if hasattr(excClass, '__name__'):
            excName = excClass.__name__
        else:
            excName = str(excClass)
        raise AssertionError("%s not raised" % excName)


def generate_random(size, part_size=5 * 1024 * 1024):
    """
    Generate the specified number random data.
    (actually each MB is a repetition of the first KB)
    """
    chunk = 1024
    allowed = string.ascii_letters
    for x in range(0, size, part_size):
        strpart = ''.join([allowed[random.randint(0, len(allowed) - 1)] for _ in range(chunk)])
        s = ''
        left = size - x
        this_part_size = min(left, part_size)
        for y in range(int(this_part_size / chunk)):
            s = s + strpart
        s = s + strpart[:(this_part_size % chunk)]
        yield s
        if x == size:
            return


def _get_status(response):
    status = response['ResponseMetadata']['HTTPStatusCode']
    return status


def _get_status_and_error_code(response):
    status = response['ResponseMetadata']['HTTPStatusCode']
    error_code = response['Error']['Code']
    return status, error_code


class Counter:
    def __init__(self, default_val):
        self.val = default_val

    def inc(self):
        self.val = self.val + 1


class ActionOnCount:
    def __init__(self, trigger_count, action):
        self.count = 0
        self.trigger_count = trigger_count
        self.action = action
        self.result = 0

    def trigger(self):
        self.count = self.count + 1
        if self.count == self.trigger_count:
            self.result = self.action()
