package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


abstract class AbstractStrategy implements IConnectStrategy {

    protected LinkedList<ConnItem> _idleConnItemList = new LinkedList<ConnItem>();
    protected ArrayList<String> _addrs = new ArrayList<String>();
    protected Lock _lockForConnItemList = new ReentrantLock();
    protected Lock _lockForAddr = new ReentrantLock();


    @Override
    public void init(List<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
        // Notice that, we won't depend on the address in used queue, for
        // some addresses may have been removed, but, they may be still in used pool.

        // get addresses to local
        Iterator<String> addrListItr = addresses.iterator();
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
                _idleConnItemList.add(pair.first());
                if (!_addrs.contains(addr)) {
                    _addrs.add(addr);
                }
            }
        }
    }

    @Override
    public abstract String getAddress();


    @Override
    public ConnItem pollConnItem(Operation opt) {
        _lockForConnItemList.lock();
        try {
            return _idleConnItemList.poll();
        } finally {
            _lockForConnItemList.unlock();
        }
    }

    @Override
    public void addAddress(String addr) {
        _lockForAddr.lock();
        try {
            if (!_addrs.contains(addr)) {
                _addrs.add(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
    }

    @Override
    public List<ConnItem> removeAddress(String addr) {
        List<ConnItem> recycleConnItemList = new ArrayList<ConnItem>();
        _lockForAddr.lock();
        try {
            if (_addrs.contains(addr)) {
                _addrs.remove(addr);
            }
        } finally {
            _lockForAddr.unlock();
        }
        _lockForConnItemList.lock();
        try {
            // Prepare the return ConnItem.
            // We will remove the returning positions.
            Iterator<ConnItem> idleConnItemListItr = _idleConnItemList.iterator();
            while (idleConnItemListItr.hasNext()) {
                ConnItem connItem = idleConnItemListItr.next();
                if (addr.equals(connItem.getAddr())) {
                	recycleConnItemList.add(connItem);
                    idleConnItemListItr.remove();
                }
            }
        } finally {
            _lockForConnItemList.unlock();
        }
        return recycleConnItemList;
    }

    @Override
    public void update(ItemStatus itemStatus, ConnItem connItem, int incDecItemCount) {
        _lockForConnItemList.lock();
        try {
            if (itemStatus == ItemStatus.IDLE && incDecItemCount > 0) {
                _idleConnItemList.add(connItem);
            }
        } finally {
            _lockForConnItemList.unlock();
        }
        return;
    }

}
