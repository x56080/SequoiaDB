/*
 * Copyright 2023 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.log.Log;
import com.sequoiadb.log.LogFactory;


import java.net.*;
import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

class AddressMgr {
    private final static String ADDRESS_SEPARATOR = ":";
    private final static Log log = LogFactory.getLog(AddressMgr.class);

    private final List<ServerAddress> normalList;
    private final List<ServerAddress> abnormalList;
    private final List<String> localIpList;
    private final ReentrantReadWriteLock rwLock = new ReentrantReadWriteLock();

    AddressMgr(List<String> addressList) {
        localIpList = getNetCardIPs();

        normalList = new ArrayList<>();
        abnormalList = new ArrayList<>();
        for (String address : addressList) {
            if (address != null && !address.isEmpty()) {
                String addr = parseAddress(address);

                ServerAddress serAddr = new ServerAddress(addr);
                serAddr.setLocal(isLocalAddress(addr));

                if (!normalList.contains(serAddr)) {
                    normalList.add(serAddr);
                }
            }
        }
        if (normalList.isEmpty()) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "No available address: " + addressList);
        }
    }

    List<ServerAddress> getNormalAddress() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            return new ArrayList<>(normalList);
        } finally {
            lock.unlock();
        }
    }

    List<ServerAddress> getAbnormalAddress() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            return new ArrayList<>(abnormalList);
        } finally {
            lock.unlock();
        }
    }

    int getLocalAddressSize() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            int result = 0;
            for (ServerAddress serAddr : normalList) {
                if (serAddr.isLocal()) {
                    result++;
                }
            }
            for (ServerAddress serAddr : abnormalList) {
                if (serAddr.isLocal()) {
                    result++;
                }
            }
            return result;
        } finally {
            lock.unlock();
        }
    }

    int getNormalAddressSize() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            return normalList.size();
        } finally {
            lock.unlock();
        }
    }

    int getAbnormalAddressSize() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            return abnormalList.size();
        } finally {
            lock.unlock();
        }
    }

    boolean isNormalAddress(String address) {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            for (ServerAddress serAddr : normalList) {
                if (address.equals(serAddr.getAddress())) {
                    return true;
                }
            }
            return false;
        } finally {
            lock.unlock();
        }
    }

    void addAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = new ServerAddress(address);
            serAddr.setLocal(isLocalAddress(address));

            if (normalList.contains(serAddr) || abnormalList.contains(serAddr)) {
                log.info(String.format("Already exist address: %s", address));
            } else {
                normalList.add(serAddr);
                log.info(String.format("Add address: %s", address));
            }
        } finally {
            lock.unlock();
        }
    }

    void removeAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            Iterator<ServerAddress> itr;

            itr = normalList.iterator();
            while (itr.hasNext()) {
                if (address.equals(itr.next().getAddress())) {
                    itr.remove();
                    break;
                }
            }

            itr = abnormalList.iterator();
            while (itr.hasNext()) {
                if (address.equals(itr.next().getAddress())) {
                    itr.remove();
                    break;
                }
            }
        } finally {
            lock.unlock();
        }
    }

    void enableAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = null;
            Iterator<ServerAddress> itr = abnormalList.iterator();
            while (itr.hasNext()) {
                serAddr = itr.next();
                if (address.equals(serAddr.getAddress())) {
                    itr.remove();
                    break;
                }
                serAddr = null;
            }
            if (serAddr != null) {
                normalList.add(serAddr);
            }
        } finally {
            lock.unlock();
        }
    }

    void disableAddress(String address) {
        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            ServerAddress serAddr = null;
            Iterator<ServerAddress> itr = normalList.iterator();
            while (itr.hasNext()) {
                serAddr = itr.next();
                if (address.equals(serAddr.getAddress())) {
                    itr.remove();
                    break;
                }
                serAddr = null;
            }
            if (serAddr != null) {
                abnormalList.add(serAddr);
            }
        } finally {
            lock.unlock();
        }
    }

    List<ServerAddress> updateAddress(List<String> addressList) {
        List<ServerAddress> sourceList = new ArrayList<>();
        for (String addr : addressList) {
            ServerAddress serAddr = new ServerAddress(addr);
            serAddr.setLocal(isLocalAddress(addr));

            if (!sourceList.contains(serAddr)) {
                sourceList.add(serAddr);
            }
        }

        Lock lock = rwLock.writeLock();
        lock.lock();
        try {
            List<ServerAddress> incList = new ArrayList<>();
            List<ServerAddress> decList = new ArrayList<>();

            for (ServerAddress serAddr : this.normalList) {
                if (!sourceList.contains(serAddr)) {
                    decList.add(serAddr);
                }
            }
            for (ServerAddress serAddr : this.abnormalList) {
                if (!sourceList.contains(serAddr)) {
                    decList.add(serAddr);
                }
            }

            for (ServerAddress serAddr : sourceList) {
                if (!this.normalList.contains(serAddr) && !this.abnormalList.contains(serAddr)) {
                    incList.add(serAddr);
                }
            }

            this.normalList.removeAll(decList);
            this.abnormalList.removeAll(decList);
            this.normalList.addAll(incList);

            if (incList.size() > 0 || decList.size() > 0) {
                log.info(String.format("update address success, increase address: %s, decrease address: %s",
                        incList, decList));
            }
            return decList;
        } finally {
            lock.unlock();
        }
    }

    String getAddressSnapshot() {
        Lock lock = rwLock.readLock();
        lock.lock();
        try {
            int localNum = 0;
            for (ServerAddress serAddr : normalList) {
                if (serAddr.isLocal()) {
                    localNum++;
                }
            }
            for (ServerAddress serAddr : abnormalList) {
                if (serAddr.isLocal()) {
                    localNum++;
                }
            }
            return String.format("normal address: %d, abnormal address: %d, local address: %d",
                    normalList.size(), abnormalList.size(), localNum);
        } finally {
            lock.unlock();
        }
    }

    private boolean isLocalAddress(String address) {
        return localIpList.contains(address.split(ADDRESS_SEPARATOR)[0]);
    }

    // hostname to ip
    static String parseHostName(String hostName) {
        try {
            return InetAddress.getByName(hostName).getHostAddress();
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to parse hostname: " + hostName, e);
        }
    }

    // hostname:port to ip:port
    static String parseAddress(String address) {
        String host ;
        int port;
        String[] tmp = address.split(ADDRESS_SEPARATOR);
        if (tmp.length < 2) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid address format: " + address);
        }

        try {
            host = parseHostName(tmp[0].trim());
            port = Integer.parseInt(tmp[1].trim());
            return host + ADDRESS_SEPARATOR + port;
        } catch (NumberFormatException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid address format: " + address, e);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to parse address: " + address, e);
        }
    }

    private static List<String> getNetCardIPs() {
        List<String> localIPs = new ArrayList<>();
        localIPs.add("127.0.0.1");
        try {
            Enumeration<NetworkInterface> netCards = NetworkInterface.getNetworkInterfaces();
            if (netCards == null) {
                return localIPs;
            }
            for (NetworkInterface netCard : Collections.list(netCards)) {
                if (netCard.getHardwareAddress() == null) {
                    continue;
                }
                List<InterfaceAddress> list = netCard.getInterfaceAddresses();
                for (InterfaceAddress interfaceAddress : list) {
                    String addr = interfaceAddress.getAddress().getHostAddress();
                    localIPs.add(addr);
                }
            }
        } catch (SocketException e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to get local ip address");
        }
        return localIPs;
    }
}
