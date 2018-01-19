package com.sequoiadb.datasource;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


abstract class AbstractStrategy implements IConnectStrategy {

    protected ArrayDeque<ConnItem> _idleConnItemDeque = new ArrayDeque<ConnItem>();
    protected ArrayList<String> _addrs = new ArrayList<String>();
    protected Lock _opLock = new ReentrantLock();
    protected Lock _addrLock = new ReentrantLock();

    @Override
    public void init(List<String> addressList, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
        // Notice that, we won't depend on the address in used queue, for
        // some addresses may have been removed, but, they may be still in used pool.

        // get addresses to local
        Iterator<String> addrListItr = addressList.iterator();
        while (addrListItr.hasNext()) {
            String addr = addrListItr.next();
            if (!_addrs.contains(addr)) {
                _addrs.add(addr);
            }
        }
        // get idle connections information
        if (_idleConnPairs != null) {
            Iterator<Pair> idleConnPairItr = _idleConnPairs.iterator();
            while (idleConnPairItr.hasNext()) {
                Pair pair = idleConnPairItr.next();
                String addr = pair.first().getAddr();
                _idleConnItemDeque.add(pair.first());
                if (!_addrs.contains(addr)) {
                    _addrs.add(addr);
                }
            }
        }
    }

    @Override
    public abstract String getAddress();


    @Override
    public ConnItem pollConnItemForGetting() {
        _opLock.lock();
        try {
            return _idleConnItemDeque.pollFirst();
        } finally {
            _opLock.unlock();
        }
    }

    @Override
    public ConnItem pollConnItemForDeleting() {
        _opLock.lock();
        try {
            return _idleConnItemDeque.pollLast();
        } finally {
            _opLock.unlock();
        }
    }

    @Override
    public void addAddress(String addr) {
        _addrLock.lock();
        try {
            if (!_addrs.contains(addr)) {
                _addrs.add(addr);
            }
        } finally {
            _addrLock.unlock();
        }
    }

    @Override
    public List<ConnItem> removeAddress(String addr) {
        List<ConnItem> connItemList = new ArrayList<ConnItem>();
        // remove address
        _addrLock.lock();
        try {
            if (_addrs.contains(addr)) {
                _addrs.remove(addr);
            }
        } finally {
            _addrLock.unlock();
        }
        // remove item
        _opLock.lock();
        try {
            // Prepare the return ConnItem.
            // We will remove the returning positions.
            Iterator<ConnItem> connItemListItr = _idleConnItemDeque.iterator();
            while (connItemListItr.hasNext()) {
                ConnItem connItem = connItemListItr.next();
                if (addr.equals(connItem.getAddr())) {
                	connItemList.add(connItem);
                    connItemListItr.remove();
                }
            }
        } finally {
            _opLock.unlock();
        }
        return connItemList;
    }

    @Override
    public void update(PoolType poolType, ConnItem connItem, int change) {
        _opLock.lock();
        try {
            if (poolType == PoolType.IDLE_POOL && change > 0) {
                _idleConnItemDeque.addFirst(connItem);
            }
        } finally {
            _opLock.unlock();
        }
        return;
    }

}
