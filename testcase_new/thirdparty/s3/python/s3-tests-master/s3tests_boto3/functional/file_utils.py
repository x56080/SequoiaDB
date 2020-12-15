import os


class FakeFile(object):
    """
    file that simulates seek, tell, and current character
    """

    def __init__(self, char='A', interrupt=None):
        self.offset = 0
        self.char = char.encode('utf-8')
        self.interrupt = interrupt

    def seek(self, offset, whence=os.SEEK_SET):
        if whence == os.SEEK_SET:
            self.offset = offset
        elif whence == os.SEEK_END:
            self.offset = self.size + offset;
        elif whence == os.SEEK_CUR:
            self.offset += offset

    def tell(self):
        return self.offset


class FakeWriteFile(FakeFile):
    """
    file that simulates interruptable reads of constant data
    """

    def __init__(self, size, char='A', interrupt=None):
        FakeFile.__init__(self, char, interrupt)
        self.size = size

    def read(self, size=-1):
        if size < 0:
            size = self.size - self.offset
        count = min(size, self.size - self.offset)
        self.offset += count

        # Sneaky! do stuff before we return (the last time)
        if self.interrupt is not None and self.offset == self.size and count > 0:
            self.interrupt()

        return self.char * count


class FakeReadFile(FakeFile):
    """
    file that simulates writes, interrupting after the second
    """

    def __init__(self, size, char='A', interrupt=None):
        FakeFile.__init__(self, char, interrupt)
        self.interrupted = False
        self.size = 0
        self.expected_size = size

    def write(self, chars):
        if chars != (self.char * len(chars)):
            raise Exception('act chars is not equal to exp chars')
        self.offset += len(chars)
        self.size += len(chars)

        # Sneaky! do stuff on the second seek
        if not self.interrupted and self.interrupt is not None \
                and self.offset > 0:
            self.interrupt()
            self.interrupted = True

    def close(self):
        if self.size != self.expected_size:
            raise Exception('act size is not equal to exp size')


class FakeFileVerifier(object):
    """
    file that verifies expected data has been written
    """

    def __init__(self, char=None):
        self.char = char
        self.size = 0

    def write(self, data):
        size = len(data)
        if self.char is None:
            self.char = data[0]
        self.size += size

        if type(self.char) is int:
            self.char = chr(self.char)
        exp_data = str(self.char * size)
        if data != exp_data.encode():
            raise Exception('act data is not equal to exp data')
