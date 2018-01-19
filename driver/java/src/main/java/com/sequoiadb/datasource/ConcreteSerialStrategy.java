package com.sequoiadb.datasource;

class ConcreteSerialStrategy extends AbstractStrategy {

    private int _counter = 0;

    @Override
    public String getAddress() {
        String addr = null;
        _addrLock.lock();
        try {
            if (1 <= _addrs.size()) {
                addr = _addrs.get((0x7fff & (_counter++)) % (_addrs.size()));
            }
        } finally {
            _addrLock.unlock();
        }
        return addr;
    }
}
