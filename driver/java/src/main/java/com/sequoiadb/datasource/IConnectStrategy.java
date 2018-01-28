package com.sequoiadb.datasource;

import java.util.List;


enum Operation {
    GET_CONN, DEL_CONN
}

enum PoolType {
    IDLE_POOL, USED_POOL
}

enum Action {
    CREATE_CONN, RELEASE_CONN
}

interface IConnectStrategy {
    public void init(List<String> addressList, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs);

    public ConnItem pollConnItemForGetting();

    public ConnItem pollConnItemForDeleting();
    public ConnItem peekConnItemForDeleting();

    /*
       PoolType    incDecItemCount      meaning
       IDLE_POOL          +             one connection had been add to idle pool,
                                        strategy need to record the info of that idle connection.
       IDLE_POOL          -             one connection had been removed from idle pool,
                                        strategy need to remove the info of that idle connection
       USED_POOL          +             one connection was filled to used pool,
                                        strategy need to increase amount of used connection with specified address
       USED_POOL          -             one connection was got out from the used pool,
                                        strategy need to decrease amount of used connection with specified address
     */
//    public void update(PoolType poolType, ConnItem connItem, int change);

    public void addConnItemAfterCreating(ConnItem connItem);

    public void addConnItemAfterReleasing(ConnItem connItem);

    public void removeConnItemAfterCleaning(ConnItem connItem);

    public void updateUsedConnItemCount(ConnItem connItem, int change);

    public String getAddress();

    public void addAddress(String addr);

    public List<ConnItem> removeAddress(String addr);
}
