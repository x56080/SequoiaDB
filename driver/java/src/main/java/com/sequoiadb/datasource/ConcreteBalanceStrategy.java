package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.util.*;
import java.util.concurrent.locks.ReentrantLock;


class CountInfo implements Comparable<CountInfo> {
    private String _addr;
    private int _count;
    private boolean _available;

    public CountInfo(String addr, int count, boolean availdable) {
        _addr = addr;
        _count = count;
        _available = availdable;
    }

    public void setAddr(String addr) {
        _addr = addr;
    }

    public String getAddr() {
        return _addr;
    }

    public void setCount(int count) {
        _count = count;
    }

    public int getCount() {
        return _count;
    }

    public boolean getAvailable() {
        return _available;
    }

    public void setAvailable(boolean available) {
        _available = available;
    }

    private void _changeCount(int count) {
        _count += count;
    }

    public void increaseCount(int count) {
        _changeCount(count);
    }

    public void decreaseCount(int count) {
        _changeCount(count);
    }

    @Override
    public int compareTo(CountInfo other) {
        if (true == this._available && false == other._available) {
            return -1;
        } else if (false == this._available && true == other._available) {
            return 1;
        } else {
            if (this._count != other._count) {
                return this._count - other._count;
            } else {
                return this._addr.compareTo(other._addr);
            }
        }
    }
}

class ConcreteBalanceStrategy implements IConnectStrategy {

    private HashMap<String, ArrayDeque<ConnItem>> _idleConnItemMap = new HashMap<String, ArrayDeque<ConnItem>>();
    private HashMap<String, CountInfo> _countInfoMap = new HashMap<String, CountInfo>();
    private TreeSet<CountInfo> _countInfoSet = new TreeSet<CountInfo>();
    private static CountInfo _dumpCountInfo = new CountInfo("", 0, false);

    @Override
    public synchronized void init(List<String> addressList, List<Pair> _idleConnPairs,
                                  List<Pair> _usedConnPairs) {
        // initialize info from giving addresses
        Iterator<String> itr1 = addressList.iterator();
        while (itr1.hasNext()) {
            String addr = itr1.next();
            if (!_idleConnItemMap.containsKey(addr)) {
                _idleConnItemMap.put(addr, new ArrayDeque<ConnItem>());
                CountInfo obj = new CountInfo(addr, 0, false);
                _countInfoMap.put(addr, obj);
                _countInfoSet.add(obj);
            }
        }

        // Initialize info from idle connections.
        // If a connection in idle pool has no information in _idleConnItemMap,
        // let's register this connection to _idleConnItemMap, _countInfoMap and
        // _countInfoSet.
        Iterator<Pair> itr2 = null;
        if (_idleConnPairs != null) {
            itr2 = _idleConnPairs.iterator();
            while (itr2.hasNext()) {
                Pair pair = itr2.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (!_idleConnItemMap.containsKey(addr)) {
                    ArrayDeque<ConnItem> deque = new ArrayDeque<ConnItem>();
                    _idleConnItemMap.put(addr, deque);
                    deque.add(item);
                    // we set this count info to be usable, for now we initialize from
                    // idle connections, but, we don't know how many connections had been
                    // used, so we it to be 0
                    CountInfo info = new CountInfo(addr, 0, true);
                    _countInfoMap.put(addr, info);
                    _countInfoSet.add(info);
                } else {
                    ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
                    deque.add(item);
                    CountInfo info = _countInfoMap.get(addr);
                    if (false == info.getAvailable()) {
                        _countInfoSet.remove(info);
                        info.setAvailable(true);
                        _countInfoSet.add(info);
                    }
                }
            }
        }

        // Initialize info from used connections.
        // Notice that, we won't keep the info of connections whose address had been remove
        // from the pool. So, when _idleConnItemMap does't contain the address of a connection,
        // we will ignore that kind of connections.
        if (_usedConnPairs != null) {
            itr2 = _usedConnPairs.iterator();
            while (itr2.hasNext()) {
                Pair pair = itr2.next();
                ConnItem item = pair.first();
                String addr = item.getAddr();
                if (_idleConnItemMap.containsKey(addr)) {
                    // should remove the original one then modify and insert again
                    CountInfo info = _countInfoMap.get(addr);
                    _countInfoSet.remove(info);
                    info.increaseCount(1);
                    _countInfoSet.add(info);
                } else {
                    continue;
                }
            }
        }

    }

    @Override
    public synchronized ConnItem pollConnItemForGetting() {
        return _pollConnItem(Operation.GET_CONN);
    }

    @Override
    public synchronized ConnItem pollConnItemForDeleting() {
        return _pollConnItem(Operation.DEL_CONN);
    }

    private ConnItem _pollConnItem(Operation operation) {
        ConnItem connItem = null;
        while (true) {
            CountInfo countInfo = null;
            String addr = null;
            if (operation == Operation.GET_CONN) {
                // get countInfo of connection which count is the least
                try {
                    countInfo = _countInfoSet.first();
                } catch (NoSuchElementException e) {
                    countInfo = null;
                }
            } else {
                // TODO: what's the meaning?
                countInfo = _countInfoSet.lower(_dumpCountInfo);
            }
            // if we have no countInfo or all the countInfo are unavailable, let's return
            if (countInfo == null || countInfo.getAvailable() == false) {
                return null;
            }
            addr = countInfo.getAddr();
            /// Now, let's get the ConnItem which associated with "addr".
            ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
            if (deque != null) {
                if (operation == Operation.GET_CONN) {
                    connItem = deque.pollFirst();
                } else {
                    connItem = deque.pollLast();
                }
            } else {
                // should never happen
                throw new BaseException(SDBError.SDB_SYS, "Invalid state in strategy");
            }
            /// Check the connItem can be use or not.
            if (connItem == null) {
                // When address "addr" has no idle connection, we get another one.
                // But, before this, let's mark the countInfo of address "addr" to be unavailable.
                // And update this countInfo
                countInfo = _countInfoMap.get(addr);
                _countInfoSet.remove(countInfo);
                countInfo.setAvailable(false);
                _countInfoSet.add(countInfo);
                continue;
            } else {
                // when we get it, let's stop
                break;
            }
        }
        // return
        return connItem;
    }

    @Override
    public synchronized String getAddress() {
        // TODO: what's the meaning
        CountInfo info = _countInfoSet.higher(_dumpCountInfo);
        if (info == null) {
            try {
                info = _countInfoSet.first();
            } catch (NoSuchElementException e) {
                // in this case, _countInfoSet is empty
                return null;
            }
        }
        return info.getAddr();
    }

    /*
     * only when the amount of connections in used pool or idle pool change,
     * we need to update
     * */
    @Override
    public synchronized void update(PoolType poolType, ConnItem connItem, int change) {
        String addr = connItem.getAddr();
        CountInfo countInfo = null;
        if (poolType == PoolType.IDLE_POOL) {
            if (!_idleConnItemMap.containsKey(addr)) {
                // maybe the information of this address was remove by "removeAddress()"
                // so let's rebuild those information
                _restoreIdleConnItemInfo(addr);
            }
            ArrayDeque<ConnItem> idleConnItemDeque = null;
            if (change > 0) {
                /// in this case, we are adding connections to idle pool
                countInfo = _countInfoMap.get(addr);
                if (countInfo == null) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point1: the pool has no information about address: " + addr);
                }
                // update the countInfo which is in the state of unavailable
                if (countInfo.getAvailable() == false) {
                    _countInfoSet.remove(countInfo);
                    countInfo.setAvailable(true);
                    _countInfoSet.add(countInfo);
                }

                // add the connItem at the head of deque
                idleConnItemDeque = _idleConnItemMap.get(addr);
                if (idleConnItemDeque == null) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point2: the pool has no information about address: " + addr);
                }
                idleConnItemDeque.addFirst(connItem);
            } else if (change < 0) {
                /// in this case, we are removing connections from idle pool
                /// when we come here, we the CLEAN TASK is working.
                idleConnItemDeque = _idleConnItemMap.get(addr);
                if (idleConnItemDeque == null) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point3: the pool has no information about address: " + addr);
                }
                if (idleConnItemDeque.size() == 0) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point4: the pool has no information about address: " + addr);
                }
                if (idleConnItemDeque.remove(connItem) == false) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point5: the pool has no information about address: " + addr);
                }
                // when current list has not connItem any more, let's set current address unusable.
                if (idleConnItemDeque.size() == 0) {
                    countInfo = _countInfoMap.get(addr);
                    _countInfoSet.remove(countInfo);
                    countInfo.setAvailable(false);
                    _countInfoSet.add(countInfo);
                }
            } else {
                throw new BaseException(SDBError.SDB_SYS, "Point1: invalid change in idle pool");
            }
        } else if (poolType == PoolType.USED_POOL) {
            // when _countInfoMap does not contain this address,
            // this address may be remove by user.
            // see "removeAddress" for more detail.
            if (_countInfoMap.containsKey(addr)) {
                countInfo = _countInfoMap.get(addr);
                // the info may be removed when strategy removed address
                if (countInfo == null) {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "Point6: the pool has no information about address: " + addr);
                }
                _countInfoSet.remove(countInfo);
                if (change > 0) {
                    countInfo.increaseCount(change);
                } else if (change < 0) {
                    countInfo.decreaseCount(change);
                } else {
                    throw new BaseException(SDBError.SDB_SYS, "Point2: invalid change in idle pool");
                }
                _countInfoSet.add(countInfo);
            }
        } else {
            // should never happen
            throw new BaseException(SDBError.SDB_SYS, "Invalid item status: " + poolType);
        }
    }

    @Override
    public synchronized void addAddress(String addr) {

        ArrayDeque<ConnItem> deque = _idleConnItemMap.get(addr);
        if (deque == null) {
            // when we have no info about address "addr", let't prepare
            _idleConnItemMap.put(addr, new ArrayDeque<ConnItem>());
            CountInfo info = new CountInfo(addr, 0, false);
            _countInfoMap.put(addr, info);
            _countInfoSet.add(info);
        }
    }

    @Override
    public synchronized List<ConnItem> removeAddress(String addr) {
        List<ConnItem> list = new ArrayList<ConnItem>();
        if (_idleConnItemMap.containsKey(addr)) {
            CountInfo obj = _countInfoMap.remove(addr);
            if (obj != null) {
                _countInfoSet.remove(obj);
            }
            ArrayDeque<ConnItem> deque = _idleConnItemMap.remove(addr);
            if (deque != null) {
                for (ConnItem item : deque) {
                    list.add(item);
                }
            }
        }
        return list;
    }

    private void _restoreIdleConnItemInfo(String addr) {
        addAddress(addr);
    }
}
