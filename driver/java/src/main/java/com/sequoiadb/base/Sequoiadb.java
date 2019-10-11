/**
 * Copyright (C) 2018 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @package com.sequoiadb.base;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
package com.sequoiadb.base;

import com.sequoiadb.base.SequoiadbConstants.Operation;
import com.sequoiadb.base.SequoiadbConstants.PreferInstanceType;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.net.ConnectionTCPImpl;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.util.SDBMessageHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.Code;
import org.bson.util.JSON;

import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;

/**
 * @class Sequoiadb
 * @brief Database operation interfaces of admin.
 */
public class Sequoiadb {
    private ServerAddress serverAddress;
    private IConnection connection;
    private String userName;
    private String password;
    boolean endianConvert;
    private long requestID = 0;

    /// for caching cs/cl name
    private Map<String, Long> nameCache = new HashMap<String, Long>();
    private static boolean enableCache = true;
    private static long cacheInterval = 300 * 1000;
    private BSONObject attributeCache = null;

    /**
     * specified the package size of the collections in current collection space to be 4K
     */
    public final static int SDB_PAGESIZE_4K = 4096;
    /**
     * specified the package size of the collections in current collection space to be 8K
     */
    public final static int SDB_PAGESIZE_8K = 8192;
    /**
     * specified the package size of the collections in current collection space to be 16K
     */
    public final static int SDB_PAGESIZE_16K = 16384;
    /**
     * specified the package size of the collections in current collection space to be 32K
     */
    public final static int SDB_PAGESIZE_32K = 32768;
    /**
     * specified the package size of the collections in current collection space to be 64K
     */
    public final static int SDB_PAGESIZE_64K = 65536;
    /** 0 means using database's default pagesize, it 64k now */
    public final static int SDB_PAGESIZE_DEFAULT = 0;

    public final static int SDB_LIST_CONTEXTS = 0;
    public final static int SDB_LIST_CONTEXTS_CURRENT = 1;
    public final static int SDB_LIST_SESSIONS = 2;
    public final static int SDB_LIST_SESSIONS_CURRENT = 3;
    public final static int SDB_LIST_COLLECTIONS = 4;
    public final static int SDB_LIST_COLLECTIONSPACES = 5;
    public final static int SDB_LIST_STORAGEUNITS = 6;
    public final static int SDB_LIST_GROUPS = 7;
    public final static int SDB_LIST_STOREPROCEDURES = 8;
    public final static int SDB_LIST_DOMAINS = 9;
    public final static int SDB_LIST_TASKS = 10;
    public final static int SDB_LIST_TRANSACTIONS = 11;
    public final static int SDB_LIST_TRANSACTIONS_CURRENT = 12;
    public final static int SDB_LIST_USERS = 16;
    public final static int SDB_LIST_CL_IN_DOMAIN = 129;
    public final static int SDB_LIST_CS_IN_DOMAIN = 130;

    public final static int SDB_SNAP_CONTEXTS = 0;
    public final static int SDB_SNAP_CONTEXTS_CURRENT = 1;
    public final static int SDB_SNAP_SESSIONS = 2;
    public final static int SDB_SNAP_SESSIONS_CURRENT = 3;
    public final static int SDB_SNAP_COLLECTIONS = 4;
    public final static int SDB_SNAP_COLLECTIONSPACES = 5;
    public final static int SDB_SNAP_DATABASE = 6;
    public final static int SDB_SNAP_SYSTEM = 7;
    public final static int SDB_SNAP_CATALOG = 8;
    public final static int SDB_SNAP_TRANSACTIONS = 9;
    public final static int SDB_SNAP_TRANSACTIONS_CURRENT = 10;

    public final static int FMP_FUNC_TYPE_INVALID = -1;
    public final static int FMP_FUNC_TYPE_JS = 0;
    public final static int FMP_FUNC_TYPE_C = 1;
    public final static int FMP_FUNC_TYPE_JAVA = 2;

    public final static String CATALOG_GROUP_NAME = "SYSCatalogGroup";

    void upsertCache(String name) {
        if (name == null)
            return;
        if (enableCache) {
            long current = System.currentTimeMillis();
            nameCache.put(name, current);
            String[] arr = name.split("\\.");
            if (arr.length > 1) {
                // extract cs name from cl full name and that
                // upsert cs name
                nameCache.put(arr[0], current);
            }
        }
    }

    void removeCache(String name) {
        if (name == null)
            return;
        String[] arr = name.split("\\.");
        if (arr.length == 1) {
            // when we come here, "name" is a cs name, so
            // we are going to remove the cache of the cs
            // and the cache of the cls

            // remove cs cache
            // name may be "foo.", it's a invalid name,
            // we don't want to remove anything,
            // so we use "name" but not "arr[0]" here
            nameCache.remove(name);
            Set<String> keySet = nameCache.keySet();
            List<String> list = new ArrayList<String>();
            for (String str : keySet) {
                String[] nameArr = str.split("\\.");
                if (nameArr.length > 1 && nameArr[0].equals(name))
                    list.add(str);
            }
            if (list.size() != 0) {
                for (String str : list)
                    nameCache.remove(str);
            }
        } else {
            // we are going to remove the cache of the cl
            nameCache.remove(name);
        }
    }

    boolean fetchCache(String name) {
        if (enableCache) {
            if (nameCache.containsKey(name)) {
                long lastUpdatedTime = nameCache.get(name);
                if ((System.currentTimeMillis() - lastUpdatedTime) >= cacheInterval) {
                    nameCache.remove(name);
                    return false;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    /**
     * @fn initClient(ClientOptions options)
     * @brief Initialize the configuration options for client.
     * @param options the configuration options for client
     * @return void
     */
    public static void initClient(ClientOptions options) {
        enableCache = (options != null) ? options.getEnableCache() : true;
        cacheInterval = (options != null && options.getCacheInterval() >= 0) ? options.getCacheInterval() : 300 * 1000;
    }

    /**
     * @fn IConnection getConnection()
     * @brief Get the current connection to remote server.
     * @return IConnection
     */
    public IConnection getConnection() {
        return connection;
    }

    /**
     * @fn ServerAddress getServerAddress()
     * @brief Get the address of remote server.
     * @return ServerAddress
     */
    public ServerAddress getServerAddress() {
        return serverAddress;
    }

    /**
     * @return Host name of SequoiaDB server.
     */
    public String getHost() {
        return serverAddress.getHost();
    }

    /**
     * @return Service port of SequoiaDB server.
     */
    public int getPort() {
        return serverAddress.getPort();
    }

    /**
     * @fn void setServerAddress(ServerAddress serverAddress)
     * @brief Set the address of remote server.
     * @param serverAddress
     *            the serverAddress object of remote server
     */
    public void setServerAddress(ServerAddress serverAddress) {
        this.serverAddress = serverAddress;
    }

    /**
     * @fn boolean isEndianConvert()
     * @brief Judge the endian of the physical computer
     * @return Big-Endian for true while Little-Endian for false
     */
    public boolean isEndianConvert() {
        return endianConvert;
    }

    /**
     * @fn Sequoiadb(String username, String password)
     * @brief Constructor. The server address is "127.0.0.1 : 11810".
     * @param username the user's name of the account
     * @param password the password of the account
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String username, String password) throws BaseException {
        // connect used default address
        serverAddress = new ServerAddress();
        ConfigOptions opts = new ConfigOptions();
        initConnection(opts);
        // authentication
        this.userName = username;
        this.password = password;
        auth();
    }

    /**
     * @fn Sequoiadb(String connString, String username, String password)
     * @brief Constructor.
     * @param connString
     *            remote server address "IP : Port" or "IP"(port is 50000)
     * @param username the user's name of the account
     * @param password the password of the account
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String connString, String username, String password)
            throws BaseException {
        this(connString, username, password, null);
    }

    /**
     * @fn Sequoiadb(String connString, String username,
     *String password, ConfigOptions options)
     * @brief Constructor.
     * @param connString
     *            remote server address "IP : Port" or "IP"(port is 11810)
     * @param username the user's name of the account
     * @param password the password of the account
     * @param options the options for connection
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String connString, String username, String password,
                     ConfigOptions options) throws BaseException {
        ConfigOptions opts = options;
        if (null == options)
            opts = new ConfigOptions();
        try {
            // connect
            serverAddress = new ServerAddress(connString);
            initConnection(opts);
        } catch (UnknownHostException e) {
            throw new BaseException(SDBError.SDB_NETWORK, connString, e);
        }
        // authentication
        this.userName = username;
        this.password = password;
        auth();
    }

    /**
     * @fn Sequoiadb(List<String> connStrings, String username, String password,
     *ConfigOptions options)
     * @brief Constructor, use a random valid address to connect to database.
     * @param connStrings The array of the coord's address
     * @param username the user's name of the account
     * @param password the password  of the account
     * @param options the options for connection
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table in local computer
     */
    public Sequoiadb(List<String> connStrings, String username, String password,
                     ConfigOptions options) throws BaseException {
        ConfigOptions opts = options;
        if (options == null)
            opts = new ConfigOptions();

        Iterator<String> tmpIter = connStrings.iterator();
        while (tmpIter.hasNext()) {
            String tmpStr = tmpIter.next();
            if (null == tmpStr) {
                tmpIter.remove();
            }
        }

        int size = connStrings.size();
        if (0 == size) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Address list is empty");
        }
        Random random = new Random();
        int count = random.nextInt(size);
        int mark = count;
        do {
            count = ++count % size;
            String str = connStrings.get(count);
            try {
                // connect
                try {
                    serverAddress = new ServerAddress(str);
                    initConnection(opts);
                } catch (UnknownHostException e) {
                    throw new BaseException(SDBError.SDB_NETWORK, str);
                }
                // authentication
                this.userName = username;
                this.password = password;
                auth();
            } catch (BaseException e) {
                if (e.getErrorCode() == SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode()) {
                    throw e;
                }
                if (mark == count) {
                    throw new BaseException(SDBError.SDB_NET_CANNOT_CONNECT);
                }
                continue;
            }
            break;
        } while (mark != count);
    }

    /**
     * @fn Sequoiadb(String addr, int port, String username, String password)
     * @brief Constructor.
     * @param addr the address of coord
     * @param port the port of coord
     * @param username the user's name of the account
     * @param password the password  of the account
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String addr, int port, String username, String password)
            throws BaseException {
        /*
        try {
			// connect
			serverAddress = new ServerAddress(addr, port);
			ConfigOptions opts = new ConfigOptions();
			initConnection(opts);
		} catch (UnknownHostException e) {
			throw new BaseException("SDB_NETWORK", addr, port);
		}
		// authentication
		this.userName = username;
		this.password = password;
		auth();
		*/
        this(addr, port, username, password, null);
    }

    /**
     * @fn Sequoiadb(String addr, int port, String username,
     *String password, ConfigOptions options)
     * @brief Constructor.
     * @param addr the address of coord
     * @param port the port of coord
     * @param username the user's name of the account
     * @param password the password of the account
     * @exception com.sequoiadb.exception.BaseException
     *            "SDB_NETWORK" means network error,
     *            "SDB_INVALIDARG" means wrong address or the address don't map to the hosts table
     */
    public Sequoiadb(String addr, int port,
                     String username, String password,
                     ConfigOptions options) throws BaseException {
        ConfigOptions opts = options;
        if (options == null)
            opts = new ConfigOptions();
        try {
            // connect
            serverAddress = new ServerAddress(addr, port);
            initConnection(opts);
        } catch (UnknownHostException e) {
            throw new BaseException(SDBError.SDB_NETWORK, addr + ":" + port, e);
        }
        // authentication
        this.userName = username;
        this.password = password;
        auth();
    }

    /**
     * @fn auth()
     * @brief authentication
     */
    private void auth() {
        endianConvert = requestSysInfo();
        byte[] request = SDBMessageHelper.buildAuthMsg(userName, password,
                getNextRequstID(), (byte) 0, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.MSG_AUTH_VERIFY_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0) {
            connection.close();
            throw new BaseException(flags, "failed to auth, user is " + userName);
        }
    }

    /**
     * @fn void createUser(String username, String password)
     * @brief Add an user in current database.
     * @param username
     *            The connection user name
     * @param password
     *            The connection password
     */
    public void createUser(String username, String password) throws BaseException {
        if (username == null || password == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        byte[] request = SDBMessageHelper.buildAuthMsg(username, password,
                getNextRequstID(), (byte) 1, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.MSG_AUTH_CRTUSR_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, "failed to create user " + username);
        }
    }

    /**
     * @fn void removeUser(String username, String password)
     * @brief Remove the spacified user from current database.
     * @param username
     *            The connection user name
     * @param password
     *            The connection password
     */
    public void removeUser(String username, String password) throws BaseException {
        byte[] request = SDBMessageHelper.buildAuthMsg(username, password,
                getNextRequstID(), (byte) 2, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.MSG_AUTH_DELUSR_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, "failed to remove user " + username);
        }
    }

    /**
     * @fn void disconnect()
     * @brief Disconnect from the remote server.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void disconnect() throws BaseException {
        if (connection == null || connection.isClosed()) {
            return;
        }
        try {
            byte[] request = SDBMessageHelper.buildDisconnectRequest(endianConvert);
            releaseResource();
            connection.sendMessage(request);
        } finally {
            connection.close();
        }
    }

    /**
     * @fn void releaseResource()
     * @brief Release the resource of the connection.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     * @since v1.2.6 && v2.2
     */
    public void releaseResource() {
        // let the receive buffer shrink to default value
        closeAllCursors();
        connection.shrinkBuffer();
    }

    /**
     * @fn boolean isClosed()
     * @brief Whether the socket has been closed or not.
     * @return return true when the socket has been
     * @since v1.2.6 && v2.2
     */
    public boolean isClosed() {
        if (connection == null)
            return true;
        return connection.isClosed();
    }

    /**
     * @fn boolean isValid()
     * @brief Send a test message to database to test whether the connection is valid or not.
     * @return if the connection is valid, return true
     * @exception com.sequoiadb.exception.BaseException
     */
    public boolean isValid() throws BaseException {
        // client not connect to database or client
        // disconnect from database
        if (connection == null || connection.isClosed())
            return false;
        try {
            sendKillContextMsg();
        } catch (BaseException e) {
            return false;
        }
        return true;
    }

    /**
     * @fn void changeConnectionOptions(ConfigOptions opts)
     * @brief Change the connection options.
     * @param opts
     *            The connection options
     * @exception com.sequoiadb.exception.BaseException
     */
    public void changeConnectionOptions(ConfigOptions opts)
            throws BaseException {
        connection.changeConfigOptions(opts);
        auth();
    }

    /**
     * @fn CollectionSpace createCollectionSpace(String collectionSpaceName)
     * @brief Create the named collection space with default SDB_PAGESIZE_4K.
     * @param csName
     *            The collection space name
     * @return the newly created collection space object
     * @exception com.sequoiadb.exception.BaseException
     */
    public CollectionSpace createCollectionSpace(String csName)
            throws BaseException {
        return createCollectionSpace(csName, SDB_PAGESIZE_DEFAULT);
    }

    /**
     * @fn CollectionSpace createCollectionSpace(String collectionSpaceName, int pageSize)
     * @brief Create collection space.
     * @param csName The name of collection space
     * @param pageSize The Page Size as below:
     * <ul>
     * <li> SDB_PAGESIZE_4K
     * <li> SDB_PAGESIZE_8K
     * <li> SDB_PAGESIZE_16K
     * <li> SDB_PAGESIZE_32K
     * <li> SDB_PAGESIZE_64K
     * <li> SDB_PAGESIZE_DEFAULT
     * </ul>
     * @return the newly created collection space object
     * @exception com.sequoiadb.exception.BaseException
     */
    public CollectionSpace createCollectionSpace(String csName, int pageSize)
            throws BaseException {
        BSONObject options = new BasicBSONObject();
        options.put("PageSize", pageSize);
        return createCollectionSpace(csName, options);
    }

    /**
     * @fn CollectionSpace createCollectionSpace(String csName, BSONObject options)
     * @brief Create collection space.
     * @param csName The name of collection space
     * @param options Contains configuration informations for create collection space. The options are as below:
     * <ul>
     * <li>PageSize    : Assign how large the page size is for the collection created in this collection space, default to be 64K
     * <li>Domain    : Assign which domain does current collection space belong to, it will belongs to the system domain if not assign this option
     * </ul>
     * @return the newly created collection space object
     * @exception com.sequoiadb.exception.BaseException
     */
    public CollectionSpace createCollectionSpace(String csName, BSONObject options)
            throws BaseException {
        if (csName == null || csName == "")
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        if (isCollectionSpaceExist(csName))
            throw new BaseException(SDBError.SDB_DMS_CS_EXIST, csName);
        SDBMessage rtnSDBMessage = createCS(csName, options);
        int flags = rtnSDBMessage.getFlags();
        if (flags != 0)
            throw new BaseException(flags);
        upsertCache(csName);
        return new CollectionSpace(this, csName);
    }

    /**
     * @fn void dropCollectionSpace(String collectionSpaceName)
     * @brief Remove the named collection space.
     * @param csName
     *            The collection space name
     * @exception com.sequoiadb.exception.BaseException
     */
    public void dropCollectionSpace(String csName) throws BaseException {
        if (!isCollectionSpaceExist(csName)) {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        }
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_NAME, csName);
        String commandString = SequoiadbConstants.DROP_CMD + " "
                + SequoiadbConstants.COLSPACE;
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
        // remove cache
        removeCache(csName);
    }

    /*
     * @param csName The collection space name
     * @param options The control options:(Only take effect in coordinate nodes, can be null)
     *                <ul>
     *                <li>GroupID:int</li>
     *                <li>GroupName:String</li>
     *                <li>NodeID:int</li>
     *                <li>HostName:String</li>
     *                <li>svcname:String</li>
     *                <li>...</li>
     *                </ul>
     * @throws BaseException SDB_INVALIDARG, SDB_DMS_CS_NOTEXIST...
     * @since 2.8
     */
    public void loadCollectionSpace(String csName, BSONObject options) throws BaseException {
        if (csName == null || csName.length() == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        if (isCollectionSpaceExist(csName))
            throw new BaseException(SDBError.SDB_DMS_CS_EXIST, csName);
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_NAME, csName);
        if (options != null) {
            matcher.putAll(options);
        }
        String commandString = SequoiadbConstants.LOAD_CMD + " "
                + SequoiadbConstants.COLSPACE;
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
        upsertCache(csName);
    }

    /*
     * @param csName The collection space name
     * @param options The control options:(Only take effect in coordinate nodes, can be null)
     *                <ul>
     *                <li>GroupID:int</li>
     *                <li>GroupName:String</li>
     *                <li>NodeID:int</li>
     *                <li>HostName:String</li>
     *                <li>svcname:String</li>
     *                <li>...</li>
     *                </ul>
     * @throws BaseException SDB_INVALIDARG, SDB_DMS_CS_NOTEXIST...
     * @since 2.8
     */
    public void unloadCollectionSpace(String csName, BSONObject options) throws BaseException {
        if (csName == null || csName.length() == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, csName);
        if (!isCollectionSpaceExist(csName))
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_NAME, csName);
        if (options != null) {
            matcher.putAll(options);
        }
        String commandString = SequoiadbConstants.UNLOAD_CMD + " "
                + SequoiadbConstants.COLSPACE;
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
        removeCache(csName);
    }

    /*
     * @param oldName The old collection space name
     * @param newName The new collection space name
     * @throws BaseException SDB_INVALIDARG, SDB_DMS_CS_NOTEXIST...
     * @since 2.8
     */
    public void renameCollectionSpace(String oldName, String newName) throws BaseException {
        if (oldName == null || oldName.length() == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, oldName);
        if (newName == null || newName.length() == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, newName);
        if (!isCollectionSpaceExist(oldName))
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, oldName);
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_OLDNAME, oldName);
        matcher.put(SequoiadbConstants.FIELD_NAME_NEWNAME, newName);
        String commandString = SequoiadbConstants.RENAME_CMD + " "
                + SequoiadbConstants.COLSPACE;
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
        removeCache(oldName);
        upsertCache(newName);
    }

    /**
     * @fn void sync(BSONObject options)
     * @brief sync the database
     * @param options The control options:(can be null)
     *                <ul>
     *                <li>
     *                    Deep:int
    Flush with deep mode or not. 1 in default.
    0 for non-deep mode,1 for deep mode,-1 means use the configuration with server
     *                </li>
     *                <li>
     *                    Block:boolean
    Flush with block mode or not. false in default.
     *                </li>
     *                <li>
     *                    CollectionSpace:String
    Specify the collectionspace to sync.
    If not set, will sync all the collection spaces and logs,
    otherwise, will only sync the collection space specified.
     *                </li>
     *                <li>
     *                Others:(Only take effect in coordinate nodes)
    GroupID:int,
    GroupName:String,
    NodeID:int,
    HostName:String,
    svcname:String
    ...
     *                </li>
     *                </ul>
     * @throws BaseException
     * @since 2.8
     */
    public void sync(BSONObject options) throws BaseException {
        SDBMessage rtn = adminCommand(SequoiadbConstants.SYNC_DB_CMD, 0, 0, -1, -1,
                options, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn void sync()
     * @brief sync the whole database
     * @throws BaseException
     * @since 2.8
     */
    public void sync() throws BaseException {
        sync(null);
    }

    /**
     * @fn CollectionSpace getCollectionSpace(String csName)
     * @brief Get the named collection space.
     * @param csName
     *            The collection space name.
     * @return the object of the specified collection space, or an exception when the collection space does not exist.
     * @exception com.sequoiadb.exception.BaseException
     */
    public CollectionSpace getCollectionSpace(String csName)
            throws BaseException {
        // get cs object from cache
        if (fetchCache(csName)) {
            return new CollectionSpace(this, csName);
        }
        // get cs object from database
        // we don't need to update or remove cache here,
        // for "isCollectionSpaceExist" has do that
        if (isCollectionSpaceExist(csName)) {
            return new CollectionSpace(this, csName);
        } else {
            throw new BaseException(SDBError.SDB_DMS_CS_NOTEXIST, csName);
        }
    }

    /**
     * @fn boolean isCollectionSpaceExist(String csName)
     * @brief Verify the existence of collection space.
     * @param csName
     *            The collecion space name
     * @return True if existed or False if not existed
     * @exception com.sequoiadb.exception.BaseException
     */
    public boolean isCollectionSpaceExist(String csName) throws BaseException {
        String commandString = SequoiadbConstants.TEST_CMD + " "
                + SequoiadbConstants.COLSPACE;
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_NAME, csName);
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags == 0) {
            upsertCache(csName);
            return true;
        } else if (flags == SDBError.SDB_DMS_CS_NOTEXIST.getErrorCode()) {
            removeCache(csName);
            return false;
        } else {
            throw new BaseException(flags, csName);
        }
    }

    /**
     * @fn DBCursor listCollectionSpaces()
     * @brief Get all the collecionspaces.
     * @return cursor of all collecionspace names
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listCollectionSpaces() throws BaseException {
        return getList(SDB_LIST_COLLECTIONSPACES, 0, 0, -1, -1, null, null,
                null, null);
    }

    /**
     * @fn ArrayList<String> getCollectionSpaceNames()
     * @brief Get all the collecion space names
     * @return A list of all collecion space names
     * @exception com.sequoiadb.exception.BaseException
     */
    public ArrayList<String> getCollectionSpaceNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_COLLECTIONSPACES, 0, 0, -1, -1,
                null, null, null, null);
        if (cursor == null)
            return null;
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * @fn DBCursor listCollections()
     * @brief Get all the collections
     * @return dbCursor of all collecions
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listCollections() throws BaseException {
        return getList(SDB_LIST_COLLECTIONS, 0, 0, 0, -1, null, null, null,
                null);
    }

    /**
     * @fn ArrayList<String> getCollectionNames()
     * @brief Get all the collection names
     * @return A list of all collecion names
     * @exception com.sequoiadb.exception.BaseException
     */
    public ArrayList<String> getCollectionNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_COLLECTIONS, 0, 0, 0, -1, null,
                null, null, null);
        if (cursor == null)
            return null;
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * @fn List<BSONObject> getStorageUnits()
     * @brief Get all the storage units
     * @return A list of all storage units
     * @exception com.sequoiadb.exception.BaseException
     */
    public ArrayList<String> getStorageUnits() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_STORAGEUNITS, 0, 0, -1, -1, null,
                null, null, null);
        if (cursor == null) {
            return null;
        }
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("Name").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * @fn void resetSnapshot()
     * @brief Reset the snapshot.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void resetSnapshot() throws BaseException {
        resetSnapshot(null);
    }

    /**
     * @param options The control options:(can be null)
     *                <ul>
     *                <li>
     *                Type: (String) Specify the snapshot type to be reset (default is "all"):
     *                <ul>
     *                <li>"sessions"</li>
     *                <li>"sessions current"</li>
     *                <li>"database"</li>
     *                <li>"health"</li>
     *                <li>"all"</li>
     *                </ul>
     *                </li>
     *                <li>
     *                SessionID: (Int32) Specify the session ID to be reset.
     *                </li>
     *                <li>
     *                Other options: Some of other options are as below:(please visit the official website to
     *                search "Location Elements" for more detail.)
     *                <ul>
     *                <li>GroupID:int,</li>
     *                <li>GroupName:String,</li>
     *                <li>NodeID:int,</li>
     *                <li>HostName:String,</li>
     *                <li>svcname:String,</li>
     *                <li>...</li>
     *                </ul>
     *                </li>
     *                </ul>
     * @return void
     * @throws BaseException If error happens.
     * @fn void resetSnapshot(BSONObject options)
     * @brief Reset the snapshot.
     */
    public void resetSnapshot(BSONObject options) throws BaseException {
        String commandString = SequoiadbConstants.SNAP_CMD + " "
                + SequoiadbConstants.RESET;
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1,
                options, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy, BSONObject hint,
     *                      long skipRows, long returnRows)
     * @brief Get the informations of specified type.
     * @param listType The list type as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS   : Get all contexts list
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS_CURRENT        : Get contexts list for the current session
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS        : Get all sessions list
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS_CURRENT        : Get the current session
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONS        : Get all collections list
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONSPACES        : Get all collection spaces list
     *                 <dt>Sequoiadb.SDB_LIST_STORAGEUNITS        : Get storage units list
     *                 <dt>Sequoiadb.SDB_LIST_GROUPS        : Get replica group list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_STOREPROCEDURES           : Get stored procedure list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_DOMAINS        : Get all the domains list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TASKS        : Get all the running split tasks ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS        : Get all the transactions information.
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT        : Get the transactions information of current session.
     *                 <dt>Sequoiadb.SDB_LIST_USERS                : Get all the user information.
     *                 </dl>
     * @param query    The matching rule, match all the documents if null.
     * @param selector The selective rule, return the whole document if null.
     * @param orderBy The ordered rule, never sort if null.
     * @param hint The options provided for specific list type. Reserved.
     * @param skipRows Skip the first skipRows documents.
     * @param returnRows Only return returnRows documents. -1 means return all matched results.
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy, BSONObject hint,
                            long skipRows, long returnRows) throws BaseException {
        return getList(listType, 0, 0, skipRows, returnRows, query, selector, orderBy, hint);
    }

    /**
     * @fn DBCursor getList(int listType, BSONObject query, BSONObject selector,
    BSONObject orderBy)
     * @brief Get the informations of specified type.
     * @param listType The list type as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS   : Get all contexts list
     *                 <dt>Sequoiadb.SDB_LIST_CONTEXTS_CURRENT        : Get contexts list for the current session
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS        : Get all sessions list
     *                 <dt>Sequoiadb.SDB_LIST_SESSIONS_CURRENT        : Get the current session
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONS        : Get all collections list
     *                 <dt>Sequoiadb.SDB_LIST_COLLECTIONSPACES        : Get all collection spaces list
     *                 <dt>Sequoiadb.SDB_LIST_STORAGEUNITS        : Get storage units list
     *                 <dt>Sequoiadb.SDB_LIST_GROUPS        : Get replica group list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_STOREPROCEDURES           : Get stored procedure list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_DOMAINS        : Get all the domains list ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TASKS        : Get all the running split tasks ( only applicable in sharding env )
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS        : Get all the transactions information.
     *                 <dt>Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT        : Get the transactions information of current session.
     *                 <dt>Sequoiadb.SDB_LIST_USERS                : Get all the user information.
     *                 </dl>
     * @param query    The matching rule, match all the documents if null.
     * @param selector The selective rule, return the whole document if null.
     * @param orderBy The ordered rule, never sort if null.
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor getList(int listType, BSONObject query, BSONObject selector, BSONObject orderBy) throws BaseException {
        return getList(listType, query, selector, orderBy, null, 0, -1);
    }

    /**
     * @fn void flushConfigure(BSONObject param)
     * @brief Flush the options to configuration file
     * @param param
     *            The param of flush, pass {"Global":true} or {"Global":false}
     *            In cluster environment, passing {"Global":true} will flush data's and catalog's configuration file,
     *            while passing {"Global":false} will flush coord's configuration file
     *            In stand-alone environment, both them have the same behaviour
     * @exception com.sequoiadb.exception.BaseException
     */
    public void flushConfigure(BSONObject param) throws BaseException {
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_EXPORT_CONFIG, 0, 0, 0, -1, param, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn void execUpdate(String sql)
     * @brief Execute sql in database.
     * @param sql the SQL command.
     * @exception com.sequoiadb.exception.BaseException
     */
    public void execUpdate(String sql) throws BaseException {
        SDBMessage sdb = new SDBMessage();
        sdb.setRequestID(getNextRequstID());
        sdb.setNodeID(SequoiadbConstants.ZERO_NODEID);
        byte[] request = SDBMessageHelper.buildSqlMsg(sdb, sql, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.MSG_BS_SQL_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, sql);
        }
    }

    /**
     * @fn DBCursor exec(String sql)
     * @brief Execute sql in database.
     * @param sql the SQL command
     * @return the DBCursor of the result
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor exec(String sql) throws BaseException {
        SDBMessage sdb = new SDBMessage();
        sdb.setRequestID(getNextRequstID());
        sdb.setNodeID(SequoiadbConstants.ZERO_NODEID);
        byte[] request = SDBMessageHelper.buildSqlMsg(sdb, sql, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.MSG_BS_SQL_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SequoiadbConstants.SDB_DMS_EOC)
                return null;
            else {
                throw new BaseException(flags, sql);
            }
        }
        return new DBCursor(rtn, this);
    }

    /**
     * @fn DBCursor getSnapshot(int snapType, String matcher, String selector,
     *     String orderBy)
     * @brief Get snapshot of the database.
     * @param snapType The snapshot types are as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS   : Get all contexts' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT        : Get the current context's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS        : Get all sessions' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS_CURRENT        : Get the current session's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONS        : Get the collections' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONSPACES        : Get the collection spaces' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_DATABASE        : Get database's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SYSTEM        : Get system's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CATALOG        : Get catalog's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS           : Get the snapshot of all the transactions
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT        : Get the snapshot of current transactions
     *                 </dl>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor instance of the result
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor getSnapshot(int snapType, String matcher, String selector,
                                String orderBy) throws BaseException {
        BSONObject ma = null;
        BSONObject se = null;
        BSONObject or = null;
        if (matcher != null)
            ma = (BSONObject) JSON.parse(matcher);
        if (selector != null)
            se = (BSONObject) JSON.parse(selector);
        if (orderBy != null)
            or = (BSONObject) JSON.parse(orderBy);

        return getSnapshot(snapType, ma, se, or);
    }

    /**
     * @fn DBCursor getSnapshot(int snapType, BSONObject matcher,
     *                           BSONObject selector, BSONObject orderBy, BSONObject hint,
     *                           long skipRows, long returnRows)
     * @brief Get snapshot of the database.
     * @param snapType The snapshot types are as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS   : Get all contexts' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT        : Get the current context's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS        : Get all sessions' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS_CURRENT        : Get the current session's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONS        : Get the collections' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONSPACES        : Get the collection spaces' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_DATABASE        : Get database's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SYSTEM        : Get system's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CATALOG        : Get catalog's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS        : Get snapshot of transactions in current session
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT           : Get snapshot of all the transactions
     *                 </dl>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @param hint     Reserved.
     * @param skipRows   Reserved.
     * @param returnRows Reserved.
     * @return the DBCursor instance of the result
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor getSnapshot(int snapType, BSONObject matcher,
                                BSONObject selector, BSONObject orderBy, BSONObject hint,
                                long skipRows, long returnRows) throws BaseException {
        String command = SequoiadbConstants.SNAP_CMD;
        switch (snapType) {
            case SDB_SNAP_CONTEXTS:
                command += " " + SequoiadbConstants.CONTEXTS;
                break;
            case SDB_SNAP_CONTEXTS_CURRENT:
                command += " " + SequoiadbConstants.CONTEXTS_CUR;
                break;
            case SDB_SNAP_SESSIONS:
                command += " " + SequoiadbConstants.SESSIONS;
                break;
            case SDB_SNAP_SESSIONS_CURRENT:
                command += " " + SequoiadbConstants.SESSIONS_CUR;
                break;
            case SDB_SNAP_COLLECTIONS:
                command += " " + SequoiadbConstants.COLLECTIONS;
                break;
            case SDB_SNAP_COLLECTIONSPACES:
                command += " " + SequoiadbConstants.COLSPACES;
                break;
            case SDB_SNAP_DATABASE:
                command += " " + SequoiadbConstants.DATABASE;
                break;
            case SDB_SNAP_SYSTEM:
                command += " " + SequoiadbConstants.SYSTEM;
                break;
            case SDB_SNAP_CATALOG:
                command += " " + SequoiadbConstants.CATA;
                break;
            case SDB_SNAP_TRANSACTIONS:
                command += " " + SequoiadbConstants.TRANSACTIONS;
                break;
            case SDB_SNAP_TRANSACTIONS_CURRENT:
                command += " " + SequoiadbConstants.TRANSACTIONS_CURRENT;
                break;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        SDBMessage rtn = adminCommand(command, 0, 0,
                                      skipRows, returnRows,
                                      matcher, selector, orderBy, hint);
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SequoiadbConstants.SDB_DMS_EOC) {
                return null;
            } else {
                String msg = "matcher = " + matcher +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy;
                throw new BaseException(flags, msg);
            }
        }
        return new DBCursor(rtn, this);

    }

    /**
     * @fn DBCursor getSnapshot(int snapType, BSONObject matcher, BSONObject
     *     selector, BSONObject orderBy)
     * @brief Get snapshot of the database.
     * @param snapType The snapshot types are as below:
     *                 <dl>
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS   : Get all contexts' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT        : Get the current context's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS        : Get all sessions' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SESSIONS_CURRENT        : Get the current session's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONS        : Get the collections' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_COLLECTIONSPACES        : Get the collection spaces' snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_DATABASE        : Get database's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_SYSTEM        : Get system's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_CATALOG        : Get catalog's snapshot
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS        : Get snapshot of transactions in current session
     *                 <dt>Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT           : Get snapshot of all the transactions
     *                 </dl>
     * @param matcher  the matching rule, match all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy  the ordered rule, never sort if null
     * @return the DBCursor instance of the result
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor getSnapshot(int snapType, BSONObject matcher,
                                BSONObject selector, BSONObject orderBy) throws BaseException {
        return getSnapshot(snapType, matcher, selector, orderBy, null, 0, -1);
    }

    /**
     * @fn void beginTransaction()
     * @brief Begin the transaction.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void beginTransaction() throws BaseException {
        byte[] request = SDBMessageHelper.buildTransactionRequest(
                SequoiadbConstants.Operation.TRANS_BEGIN_REQ, getNextRequstID(),
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.TRANS_BEGIN_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0)
            throw new BaseException(flags);
    }

    /**
     * @fn void commit()
     * @brief Commit the transaction.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void commit() throws BaseException {
        byte[] request = SDBMessageHelper.buildTransactionRequest(
                SequoiadbConstants.Operation.TRANS_COMMIT_REQ,
                getNextRequstID(), endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.TRANS_COMMIT_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0)
            throw new BaseException(flags);
    }

    /**
     * @fn void rollback()
     * @brief Rollback the transaction.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void rollback() throws BaseException {
        byte[] request = SDBMessageHelper.buildTransactionRequest(
                SequoiadbConstants.Operation.TRANS_ROLLBACK_REQ,
                getNextRequstID(), endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtn = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtn.getOperationCode() != Operation.TRANS_ROLLBACK_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtn.getOperationCode().toString());
        }
        int flags = rtn.getFlags();
        if (flags != 0)
            throw new BaseException(flags);
    }

    /**
     * @fn void crtJSProcedure ( String code )
     * @brief Create a store procedure.
     * @param code The code of store procedure
     * @exception com.sequoiadb.exception.BaseException
     */
    public void crtJSProcedure(String code) throws BaseException {
        // check the argument
        if (null == code || code.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, code);
        }
        // build code type bson
        BSONObject newobj = new BasicBSONObject();
        Code codeObj = new Code(code);
        newobj.put(SequoiadbConstants.FIELD_NAME_FUNC, codeObj);
        newobj.put(SequoiadbConstants.FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS);

        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_CRT_PROCEDURE,
                0, 0, 0, -1, newobj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn void rmProcedure ( String name )
     * @brief Remove a store procedure.
     * @param name The name of store procedure to be removed
     * @exception com.sequoiadb.exception.BaseException
     */
    public void rmProcedure(String name) throws BaseException {
        // check the argument
        if (null == name || name.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, name);
        }
        // append the name to a bson
        BSONObject newobj = new BasicBSONObject();
        newobj.put(SequoiadbConstants.FIELD_NAME_FUNC, name);

        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_RM_PROCEDURE,
                0, 0, 0, -1, newobj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0)
            throw new BaseException(flags);
    }

    /**
     * @fn DBCursor listProcedures ( BSONObject condition )
     * @brief List the store procedures.
     * @param condition The condition of list eg: {"name":"sum"}. return all if null
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listProcedures(BSONObject condition) throws BaseException {
        return getList(SDB_LIST_STOREPROCEDURES, 0, 0, 0,
                -1, condition, null, null, null);
    }

    /**
     * @fn Sequoiadb.SptEvalResult evalJS ( String code )
     * @brief Eval javascript code.
     * @param code The javasript code
     * @return The result of the eval operation, including the return value type,
     *         the return data and the error message. If succeed to eval, error message is null,
     *         and we can extract the eval result from the return cursor and return type,
     *         if not, the return cursor and the return type are null, we can extract
     *         the error mssage for more detail. 
     * @exception com.sequoiadb.exception.BaseException
     */
    public Sequoiadb.SptEvalResult evalJS(String code) throws BaseException {
        // check the argument
        if (code == null || code.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }
        SptEvalResult evalResult = new Sequoiadb.SptEvalResult();
        // build code type bson
        BSONObject newObj = new BasicBSONObject();
        Code codeObj = new Code(code);
        newObj.put(SequoiadbConstants.FIELD_NAME_FUNC, codeObj);
        newObj.put(SequoiadbConstants.FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS);

        SDBMessage rtn = adminCommandEval(SequoiadbConstants.CMD_NAME_EVAL,
                0, 0, 0, -1, newObj,
                null, null, null);
        // get error code
        int flags = rtn.getFlags();
        // if something wrong with the eval operation, not throws exception here
        if (flags != 0) {
            // get error message
            List<BSONObject> objList = rtn.getObjectList();
            if (objList.size() > 0) {
                evalResult.errmsg = rtn.getObjectList().get(0);
            }
            return evalResult;
        } else {
            // get the return type of eval result
            List<BSONObject> objList = rtn.getObjectList();
            int typeValue = (Integer) objList.get(0).get(SequoiadbConstants.FIELD_NAME_RETYE);
            evalResult.returnType = Sequoiadb.SptReturnType.getTypeByValue(typeValue);
            // set the return cursor
            evalResult.cursor = new DBCursor(rtn, this);
            return evalResult;
        }
    }

    /**
     * @fn void backupOffline ( BSONObject options )
     * @brief Backup the whole database or specifed replica group.
     * @param options Contains a series of backup configuration infomations. 
     *        Backup the whole cluster if null. The "options" contains 5 options as below. 
     *        All the elements in options are optional. 
     *        eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup", 
     *             "Name":"backupName", "Description":description, "EnsureInc":true, "OverWrite":true}
     *<ul>
     *<li>GroupID     : The id(s) of replica group(s) which to be backuped
     *<li>GroupName   : The name(s) of replica group(s) which to be backuped
     *<li>Name        : The name for the backup
     *<li>Path        : The backup path, if not assign, use the backup path assigned in the configuration file,
     *                  the path support to use wildcard(%g/%G:group name, %h/%H:host name, %s/%S:service name).
     *                  e.g.  {Path:"/opt/sequoiadb/backup/%g"}
     *<li>isSubDir    : Whether the path specified by paramer "Path" is a subdirectory of
     *                  the path specified in the configuration file, default to be false
     *<li>Prefix      : The prefix of name for the backup, default to be null. e.g. {Prefix:"%g_bk_"}
     *<li>EnableDateDir : Whether turn on the feature which will create subdirectory named to
     *                    current date like "YYYY-MM-DD" automatically, default to be false             
     *<li>Description : The description for the backup
     *<li>EnsureInc   : Whether turn on increment synchronization, default to be false
     *<li>OverWrite   : Whether overwrite the old backup file with the same name, default to be false
     *</ul>
     * @exception com.sequoiadb.exception.BaseException
     */
    public void backupOffline(BSONObject options) throws BaseException {
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_BACKUP_OFFLINE,
                0, 0, 0, -1, options,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn DBCursor listBackup ( BSONObject options, BSONObject matcher,
    BSONObject selector, BSONObject orderBy )
     * @brief List the backups.
     * @param options  Contains configuration information for listing backups, list all the backups in the default backup path if null.
     *                 The "options" contains several options as below. All the elements in options are optional.
     *                 eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                 <ul>
     *                 <li>GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
     *                 <li>GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
     *                 <li>Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
     *                 <li>Name        : Specified the name of backup, default to list all the backups.
     *                 <li>IsSubDir    : Specified the "Path" is a subdirectory of the backup path asigned in the configuration file or not, default to be false.
     *                 <li>Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
     *                 <li>Detail      : Display the detail of the backups or not, default to be false.
     *                 </ul>
     * @param matcher The matching rule, return all the documents if null
     * @param selector The selective rule, return the whole document if null
     * @param orderBy The ordered rule, never sort if null
     * @return the DBCursor of the backup or null while having no backup infonation. 
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listBackup(BSONObject options, BSONObject matcher,
                               BSONObject selector, BSONObject orderBy) throws BaseException {
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_LIST_BACKUP,
                0, 0, 0, -1, matcher,
                selector, orderBy, options);
        DBCursor cursor = null;
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SequoiadbConstants.SDB_DMS_EOC) {
                return cursor;
            } else {
                String msg = "matcher = " + matcher +
                        ", selector = " + selector +
                        ", orderBy = " + orderBy +
                        ", options = " + options;
                throw new BaseException(flags, msg);
            }
        }
        cursor = new DBCursor(rtn, this);
        return cursor;
    }

    /**
     * @fn void removeBackup ( BSONObject options )
     * @brief Remove the backups.
     * @param options Contains configuration information for removing backups, remove all the backups in the default backup path if null.
     *                The "options" contains several options as below. All the elements in options are optional.
     *                eg: {"GroupName":["rgName1", "rgName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}
     *                 <ul>
     *                 <li>GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
     *                 <li>GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
     *                 <li>Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
     *                 <li>Name        : Specified the name of backup, default to list all the backups.
     *                 <li>IsSubDir    : Specified the "Path" is a subdirectory of the backup path assigned in the configuration file or not, default to be false.
     *                 <li>Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
     *                 <li>Detail      : Display the detail of the backups or not, default to be false.
     *                 </ul>
     * @exception com.sequoiadb.exception.BaseException
     */
    public void removeBackup(BSONObject options) throws BaseException {
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_REMOVE_BACKUP,
                0, 0, 0, -1, options,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn DBCursor listTasks ( BSONObject matcher, BSONObject selector,
     *		                    BSONObject orderBy, BSONObject hint )
     * @brief List the tasks.
     * @param matcher The matching rule, return all the documents if null
     * @param selector The selective rule, return the whole document if null
     * @param orderBy The ordered rule, never sort if null
     * @param hint
     *            Specified the index used to scan data. e.g. {"":"ageIndex"} means 
     *            using index "ageIndex" to scan data(index scan); 
     *            {"":null} means table scan. when hint is null, 
     *            database automatically match the optimal index to scan data.
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listTasks(BSONObject matcher, BSONObject selector,
                              BSONObject orderBy, BSONObject hint) throws BaseException {

        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_LIST_TASK,
                0, 0, 0, -1, matcher,
                selector, orderBy, hint);
        int flags = rtn.getFlags();
        if (flags != 0) {
            String msg = "matcher = " + matcher +
                    ", selector = " + selector +
                    ", orderBy = " + orderBy +
                    ", hint = " + hint;
            throw new BaseException(flags, msg);
        }
        // return the result by cursor
        DBCursor cursor = null;
        cursor = new DBCursor(rtn, this);
        return cursor;
    }

    /**
     * @fn DBCursor waitTasks (long[] taskIDs)
     * @brief Wait the tasks to finish.
     * @param taskIDs The array of task id
     * @exception com.sequoiadb.exception.BaseException
     */
    public void waitTasks(long[] taskIDs) throws BaseException {
        // check argument
        if (taskIDs == null || taskIDs.length == 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "taskIDs is empty or null");
        // append argument:{ "TaskID": { "$in": [ 1, 2, 3 ] } }
        BSONObject newObj = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        BSONObject list = new BasicBSONList();
        for (int i = 0; i < taskIDs.length; i++) {
            list.put(Integer.toString(i), taskIDs[i]);
        }
        subObj.put("$in", list);
        newObj.put(SequoiadbConstants.FIELD_NAME_TASKID, subObj);

        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_WAITTASK,
                0, 0, 0, -1, newObj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn DBCursor cancelTask ( long taskID, boolean isAsync )
     * @brief Cancel the specified task.
     * @param taskID The task id
     * @param isAsync The operation "cancel task" is async or not,
     *                "true" for async, "false" for sync. Default sync.
     * @exception com.sequoiadb.exception.BaseException
     */
    public void cancelTask(long taskID, boolean isAsync) throws BaseException {
        // check argument
        if (taskID <= 0) {
            String msg = "taskID = " + taskID + ", isAsync = " + isAsync;
            throw new BaseException(SDBError.SDB_INVALIDARG, msg);
        }
        // append argument
        BSONObject newObj = new BasicBSONObject();
        newObj.put(SequoiadbConstants.FIELD_NAME_TASKID, taskID);
        newObj.put(SequoiadbConstants.FIELD_NAME_ASYNC, isAsync);
        // run command
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_CANCEL_TASK,
                0, 0, 0, -1, newObj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    private void clearSessionAttrCache() {
        attributeCache = null;
    }

    private BSONObject getSessionAttrCache() {
        return attributeCache;
    }

    private void setSessionAttrCache(BSONObject attribute) {
        attributeCache = attribute;
    }

    /**
     * @fn void setSessionAttr( BSONObject options )
     * @brief Set the attributes of the current session.
     * @param options The configuration options for the current session.The options are as below:
     *                <ul>
     *                <li>PreferedInstance : Preferred instance for read request in the current session. Could be single value in "M", "m", "S", "s", "A", "a", 1-255, or BSON Array to include multiple values. e.g. { "PreferedInstance" : [ 1, 7 ] }.
     *                    <ul>
     *                        <li>"M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.</li>
     *                        <li>"S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.</li>
     *                        <li>"A", "a": any instance.</li>
     *                        <li>1-255: the instance with specified instance ID.</li>
     *                        <li>If multiple alphabet instances are given, only first one will be used.</li>
     *                        <li>If matched instance is not found, will choose instance by random.</li>
     *                    </ul>
     *                </li>
     *                <li>PreferedInstanceMode : The mode to choose query instance when multiple preferred instances are found in the current session. e.g. { "PreferedInstanceMode : "random" }.
     *                    <ul>
     *                        <li>"random": choose the instance from matched instances by random.</li>
     *                        <li>"ordered": choose the instance from matched instances by the order of "PreferedInstance".</li>
     *                    </ul>
     *                </li>
     *                <li>Timeout : The timeout (in ms) for operations in the current session. -1 means no timeout for operations. e.g. { "Timeout" : 10000 }.
     *                </li>
     *                </ul>
     * @exception com.sequoiadb.exception.BaseException
     */
    public void setSessionAttr(BSONObject options) throws BaseException {
        // check argument
        if (null == options || options.isEmpty()) {
            return;
        }

        BSONObject newObj = new BasicBSONObject();

        newObj.putAll(options);

        if (options.containsField(SequoiadbConstants.FIELD_NAME_PREFERED_INSTANCE)) {
            // Add old version of preferred instance
            Object value = options.get(SequoiadbConstants.FIELD_NAME_PREFERED_INSTANCE);
            if (value instanceof String) {
                int v = PreferInstanceType.INS_MASTER.getCode();
                if (value.equals("M") || value.equals("m")) {
                    v = PreferInstanceType.INS_MASTER.getCode();
                } else if (value.equals("S") || value.equals("s")) {
                    v = PreferInstanceType.INS_SLAVE.getCode();
                } else if (value.equals("A") || value.equals("a")) {
                    v = PreferInstanceType.INS_ANYONE.getCode();
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG, options.toString());
                }
                newObj.put(SequoiadbConstants.FIELD_NAME_PREFERED_INSTANCE, v);
            } else if (value instanceof Integer) {
                newObj.put(SequoiadbConstants.FIELD_NAME_PREFERED_INSTANCE, value);
            }
            // Add new version of preferred instance
            newObj.put(SequoiadbConstants.FIELD_NAME_PREFERED_INSTANCE_V1, value);
        }

        clearSessionAttrCache();
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_SETSESS_ATTR,
                0, 0, 0, -1, newObj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn BSONObject getSessionAttr()
     * @brief Get the attributes of the current session.
     * @return the BSONObject of the session attribute.
     * @exception com.sequoiadb.exception.BaseException
     * @since 2.8.5
     */
    public BSONObject getSessionAttr() throws BaseException {
        BSONObject result = getSessionAttrCache();
        if (null != result) {
            return result;
        }
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_GETSESS_ATTR,
                0, 0, 0, -1, null, null, null, null);
        List<BSONObject> resultList = rtn.getObjectList();
        if (null != resultList && resultList.size() > 0) {
            result = resultList.get(0);
            if (null == result) {
                clearSessionAttrCache();
            } else {
                setSessionAttrCache(result);
            }
        } else {
            clearSessionAttrCache();
        }
        return result;
    }

    /**
     * @fn void closeAllCursors()
     * @brief Close all the cursors created in current connection, we can't use those cursors to get
     *        data again.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void closeAllCursors() throws BaseException {
        byte[] request = SDBMessageHelper.buildTransactionRequest(
                SequoiadbConstants.Operation.MSG_BS_INTERRUPTE,
                getNextRequstID(), endianConvert);
        connection.sendMessage(request);
    }

    /**
     * @fn DBCursor listReplicaGroups()
     * @brief List all the replica group.
     * @return information of all replica groups.
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listReplicaGroups() throws BaseException {
        return getList(SDB_LIST_GROUPS, 0, 0, -1, -1, null, null,
                null, null);
    }

    /**
     * @fn boolean isDomainExist(String domainName)
     * @brief Verify the existence of domain.
     * @param domainName the name of domain
     * @return True if existed or False if not existed
     * @exception com.sequoiadb.exception.BaseException
     */
    public boolean isDomainExist(String domainName) throws BaseException {
        if (null == domainName || domainName.equals(""))
            throw new BaseException(SDBError.SDB_INVALIDARG, domainName);
        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_NAME, domainName);
        DBCursor cursor = getList(SDB_LIST_DOMAINS, matcher, null, null);
        try {
            if (cursor != null && cursor.hasNext())
                return true;
            else
                return false;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    /**
     * @fn Domain createDomain(String domainName, BSONObject options)
     * @brief Create a domain.
     * @param domainName The name of the creating domain
     * @param options The options for the domain. The options are as below:
     * <ul>
     * <li>Groups    : the list of the replica groups' names which the domain is going to contain.
     *                 eg: { "Groups": [ "group1", "group2", "group3" ] }
     *                 If this argument is not included, the domain will contain all replica groups in the cluster. 
     * <li>AutoSplit    : If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
     *                    the data of this collection will be split(hash split) into all the groups in this domain automatically.
     *                    However, it won't automatically split data into those groups which were add into this domain later.
     *                    eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
     * </ul>
     * @return the newly created collection space object
     * @exception com.sequoiadb.exception.BaseException
     */
    public Domain createDomain(String domainName, BSONObject options) throws BaseException {
        if (null == domainName || domainName.equals("")) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "domain name is empty or null");
        }
        if (isDomainExist(domainName))
            throw new BaseException(SDBError.SDB_CAT_DOMAIN_EXIST, domainName);

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SequoiadbConstants.FIELD_NAME_NAME, domainName);
        if (null != options) {
            newObj.put(SequoiadbConstants.FIELD_NAME_OPTIONS, options);
        }
        // command
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_CREATE_DOMAIN,
                0, 0, 0, -1, newObj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
        return new Domain(this, domainName);

    }

    /**
     * @fn void dropDomain(String domainName)
     * @brief Drop a domain.
     * @param domainName the name of the domain
     * @exception com.sequoiadb.exception.BaseException
     */
    public void dropDomain(String domainName) throws BaseException {
        if (null == domainName || domainName.equals(""))
            throw new BaseException(SDBError.SDB_INVALIDARG, "domain name is empty or null");

        BSONObject newObj = new BasicBSONObject();
        newObj.put(SequoiadbConstants.FIELD_NAME_NAME, domainName);
        // command
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_DROP_DOMAIN,
                0, 0, 0, -1, newObj,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @fn Domain getDomain(String domainName)
     * @brief Get the specified domain.
     * @param domainName the name of the domain
     * @return the Domain instance
     * @exception com.sequoiadb.exception.BaseException
     *            If the domain not exit, throw BaseException with the error type "SDB_CAT_DOMAIN_NOT_EXIST"
     */
    public Domain getDomain(String domainName)
            throws BaseException {
        if (isDomainExist(domainName)) {
            return new Domain(this, domainName);
        } else {
            throw new BaseException(SDBError.SDB_CAT_DOMAIN_NOT_EXIST, domainName);
        }
    }

    /**
     * @fn DBCursor listDomains(BSONObject matcher, BSONObject selector,
    BSONObject orderBy, BSONObject hint)
     * @brief List domains.
     * @param matcher the matching rule, return all the documents if null
     * @param selector the selective rule, return the whole document if null
     * @param orderBy the ordered rule, never sort if null
     * @param hint
     *            Specified the index used to scan data. e.g. {"":"ageIndex"} means 
     *            using index "ageIndex" to scan data(index scan); 
     *            {"":null} means table scan. when hint is null, 
     *            database automatically match the optimal index to scan data.
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBCursor listDomains(BSONObject matcher, BSONObject selector,
                                BSONObject orderBy, BSONObject hint) throws BaseException {
        return getList(SDB_LIST_DOMAINS, 0, 0, 0, -1, matcher, selector, orderBy, hint);
    }

    /**
     * @fn ArrayList<String> getReplicaGroupNames()
     * @brief Get all the replica groups' name.
     * @return A list of all the replica groups' names.
     * @exception com.sequoiadb.exception.BaseException
     */
    public ArrayList<String> getReplicaGroupNames() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_GROUPS, 0, 0, -1, -1,
                null, null, null, null);
        if (cursor == null)
            return null;
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().get("GroupName").toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * @fn List<String> getReplicaGroupsInfo()
     * @brief Get the infomations of the replica groups.
     * @return A list of informations of the replica groups.
     * @exception com.sequoiadb.exception.BaseException
     */
    public ArrayList<String> getReplicaGroupsInfo() throws BaseException {
        DBCursor cursor = getList(SDB_LIST_GROUPS, 0, 0, -1, -1, null, null,
                null, null);
        if (cursor == null)
            return null;
        ArrayList<String> colList = new ArrayList<String>();
        try {
            while (cursor.hasNext()) {
                colList.add(cursor.getNext().toString());
            }
        } finally {
            cursor.close();
        }
        return colList;
    }

    /**
     * @fn boolean isReplicaGroupExist(String rgName)
     * @brief whether the replica group exists in the database or not
     * @param rgName replica group's name
     * @return true or false
     */
    public boolean isRelicaGroupExist(String rgName) {
        BSONObject rg = getDetailByName(rgName);
        if (rg == null) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * @fn boolean isReplicaGroupExist(int rgId)
     * @brief whether the replica group exists in the database or not
     * @param rgId id of replica group
     * @return true or false
     */
    public boolean isReplicaGroupExist(int rgId) {
        BSONObject rg = getDetailById(rgId);
        if (rg == null) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * @fn ReplicaGroup getReplicaGroup(String rgName)
     * @brief Get replica group by name.
     * @param rgName
     *            replica group's name
     * @return A replica group object or null for not exit.
     * @exception com.sequoiadb.exception.BaseException
     */
    public ReplicaGroup getReplicaGroup(String rgName)
            throws BaseException {
        BSONObject rg = getDetailByName(rgName);
        if (rg == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST,
                    String.format("Group with the name[%s] does not exist", rgName));
        }
        return new ReplicaGroup(this, rgName);
    }

    /**
     * @fn ReplicaGroup getReplicaGroup(int rgId)
     * @brief Get replica group by id.
     * @param rgId
     *            replica group id
     * @return A replica group object or null for not exit.
     * @exception com.sequoiadb.exception.BaseException
     */
    public ReplicaGroup getReplicaGroup(int rgId) throws BaseException {
        BSONObject rg = getDetailById(rgId);
        if (rg == null) {
            throw new BaseException(SDBError.SDB_CLS_GRP_NOT_EXIST,
                    String.format("Group with the id[%d] does not exist", rgId));
        }
        return new ReplicaGroup(this, rgId);
    }

    /**
     * @fn ReplicaGroup createReplicaGroup(String rgName)
     * @brief Create replica group by name.
     * @param rgName
     *            replica group's name
     * @return A replica group object.
     * @exception com.sequoiadb.exception.BaseException
     */
    public ReplicaGroup createReplicaGroup(String rgName)
            throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, rgName);
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_CREATE_GROUP,
                0, 0, -1, -1, rg, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, rgName);
        }
        return new ReplicaGroup(this, rgName);
    }

    /**
     * @fn void removeReplicaGroup(String rgName)
     * @brief Remove replica group by name.
     * @param rgName
     *            replica group's name
     * @exception com.sequoiadb.exception.BaseException
     */
    public void removeReplicaGroup(String rgName)
            throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, rgName);
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_REMOVE_GROUP,
                0, 0, -1, -1, rg, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, rgName);
        }
    }

    long getNextRequstID() {
        return requestID++;
    }

    /**
     * @fn void activateReplicaGroup(String rgName)
     * @brief Active replica group by name.
     * @param rgName
     *            replica group name
     * @exception com.sequoiadb.exception.BaseException
     */
    public void activateReplicaGroup(String rgName)
            throws BaseException {
        BSONObject rg = new BasicBSONObject();
        rg.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, rgName);
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_ACTIVE_GROUP,
                0, 0, -1, -1, rg, null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, rgName);
        }
    }

    /**
     * @fn void createReplicaCataGroup(String hostName, int port, String dbPath,
     *     BSONObject configuration)
     * @brief Create the replica Catalog group with the given options.
     * @param hostName
     *            The host name
     * @param port
     *            The port
     * @param dbPath
     *            The database path
     * @param configure
     *            The configure options
     * @exception com.sequoiadb.exception.BaseException
     */
    public void createReplicaCataGroup(String hostName, int port,
                                       String dbPath, Map<String, String> configure) {
        String commandString = SequoiadbConstants.CMD_NAME_CREATE_CATA_GROUP;
        BSONObject obj = new BasicBSONObject();
        obj.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        obj.put(SequoiadbConstants.PMD_OPTION_SVCNAME, Integer.toString(port));
        obj.put(SequoiadbConstants.PMD_OPTION_DBPATH, dbPath);
        if (configure != null) {
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME)
                        || key.equals(SequoiadbConstants.PMD_OPTION_DBPATH)) {
                    continue;
                }
                obj.put(key, configure.get(key).toString());
            }
        }
        SDBMessage rtn = adminCommand(commandString, 0, 0, -1, -1, obj, null,
                null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * Stop the specified session's current operation and terminate it.
     *
     * @param sessionID
     *            The ID of the session.
     */
    public void forceSession(long sessionID){
        forceSession(sessionID, null);
    }

    /**
     * Stop the specified session's current operation and terminate it.
     *
     * @param sessionID
     *            The ID of the session.
     * @param option
     *            The control options, Please reference
     *            {@see <a
     *            href=http://doc.sequoiadb.com/cn/SequoiaDB-cat_id-1482314609-edition_id-208>here</a>}
     *            for more detail.
     */
    public void forceSession(long sessionID,BSONObject option ){

        BSONObject matcher = new BasicBSONObject();
        matcher.put(SequoiadbConstants.FIELD_NAME_SESSION_ID, sessionID);
        if (option != null) {
            matcher.putAllUnique(option);
        }
        SDBMessage rtn = adminCommand(SequoiadbConstants.CMD_NAME_FORCE_SESSION, 0, 0, -1, -1, matcher,
                null, null, null);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    DBCursor getList(int listType, int flag, long reqID, long skipNum,
                     long returnNum, BSONObject query, BSONObject selector,
                     BSONObject order, BSONObject hint) throws BaseException {
        String command = "";
        switch (listType) {
            case SDB_LIST_CONTEXTS:
                command = SequoiadbConstants.CMD_NAME_LIST_CONTEXTS;
                break;
            case SDB_LIST_CONTEXTS_CURRENT:
                command = SequoiadbConstants.CMD_NAME_LIST_CONTEXTS_CURRENT;
                break;
            case SDB_LIST_SESSIONS:
                command = SequoiadbConstants.CMD_NAME_LIST_SESSIONS;
                break;
            case SDB_LIST_SESSIONS_CURRENT:
                command = SequoiadbConstants.CMD_NAME_LIST_SESSIONS_CURRENT;
                break;
            case SDB_LIST_COLLECTIONS:
                command = SequoiadbConstants.CMD_NAME_LIST_COLLECTIONS;
                break;
            case SDB_LIST_COLLECTIONSPACES:
                command = SequoiadbConstants.CMD_NAME_LIST_COLLECTIONSPACES;
                break;
            case SDB_LIST_STORAGEUNITS:
                command = SequoiadbConstants.CMD_NAME_LIST_STORAGEUNITS;
                break;
            case SDB_LIST_GROUPS:
                command = SequoiadbConstants.CMD_NAME_LIST_GROUPS;
                break;
            case SDB_LIST_STOREPROCEDURES:
                command = SequoiadbConstants.CMD_NAME_LIST_PROCEDURES;
                break;
            case SDB_LIST_DOMAINS:
                command = SequoiadbConstants.CMD_NAME_LIST_DOMAINS;
                break;
            case SDB_LIST_TASKS:
                command = SequoiadbConstants.CMD_NAME_LIST_TASKS;
                break;
            case SDB_LIST_TRANSACTIONS:
                command = SequoiadbConstants.CMD_NAME_LIST_TRANSACTIONS;
                break;
            case SDB_LIST_TRANSACTIONS_CURRENT:
                command = SequoiadbConstants.CMD_NAME_LIST_TRANSACTIONS_CURRENT;
                break;
            case SDB_LIST_USERS:
                command = SequoiadbConstants.CMD_NAME_LIST_USERS;
                break;
            case SDB_LIST_CL_IN_DOMAIN:
                command = SequoiadbConstants.CMD_NAME_LIST_CL_IN_DOMAIN;
                break;
            case SDB_LIST_CS_IN_DOMAIN:
                command = SequoiadbConstants.CMD_NAME_LIST_CS_IN_DOMAIN;
                break;
            default:
                throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        SDBMessage rtn = adminCommand(command, flag, reqID, skipNum, returnNum,
                query, selector, order, hint);
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SequoiadbConstants.SDB_DMS_EOC) {
                return null;
            } else {
                String msg = "query = " + query +
                        ", selector = " + selector +
                        ", order = " + order +
                        ", hint = " + hint;
                throw new BaseException(flags, msg);
            }
        }
        return new DBCursor(rtn, this);
    }

    String getUserName() {
        return userName;
    }

    String getPassword() {
        return password;
    }

    BSONObject getDetailByName(String name) throws BaseException {
        BSONObject condition = new BasicBSONObject();
        condition.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        BSONObject obj;
        DBCursor shardsCursor = getList(Sequoiadb.SDB_LIST_GROUPS, 0, 0, -1,
                -1, condition, null, null, null);
        try {
            if (shardsCursor == null || !shardsCursor.hasNext()) {
                return null;
            }
            obj = shardsCursor.getNext();
        } finally {
            if (shardsCursor != null) {
                shardsCursor.close();
            }
        }
        return obj;
    }

    BSONObject getDetailById(int id) throws BaseException {
        BSONObject condition = new BasicBSONObject();
        condition.put(SequoiadbConstants.FIELD_NAME_GROUPID, id);
        DBCursor shardsCursor = getList(Sequoiadb.SDB_LIST_GROUPS, 0, 0, -1,
                -1, condition, null, null, null);
        BSONObject obj;
        try {
            if (shardsCursor == null || !shardsCursor.hasNext())
                return null;
            obj = shardsCursor.getNext();
        } finally {
            if (shardsCursor != null) {
                shardsCursor.close();
            }
        }
        return obj;
    }

    private void initConnection(ConfigOptions options) throws BaseException {
        if (options == null)
            throw new BaseException(SDBError.SDB_INVALIDARG);
        connection = new ConnectionTCPImpl(serverAddress, options);
        connection.initialize();
    }

    SDBMessage adminCommand(String commandString, int flag, long reqID,
                            long skipNum, long returnNum, BSONObject query,
                            BSONObject selector, BSONObject order, BSONObject hint)
            throws BaseException {
        // Admin command request
        // int reqId = 0;
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage sdbMessage = new SDBMessage();

        if (query == null)
            sdbMessage.setMatcher(dummyObj);
        else
            sdbMessage.setMatcher(query);
        if (selector == null)
            sdbMessage.setSelector(dummyObj);
        else
            sdbMessage.setSelector(selector);
        if (order == null)
            sdbMessage.setOrderBy(dummyObj);
        else
            sdbMessage.setOrderBy(order);
        if (hint == null)
            sdbMessage.setHint(dummyObj);
        else
            sdbMessage.setHint(hint);

        sdbMessage.setCollectionFullName(SequoiadbConstants.ADMIN_PROMPT
                + commandString);

        sdbMessage.setVersion(1);
        sdbMessage.setW((short) 0);
        sdbMessage.setPadding((short) 0);
        sdbMessage.setFlags(flag);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        // sdbMessage.setResponseTo(reqId);
        // reqId++;
        if (0 == reqID) {
            reqID = getNextRequstID();
        }
        sdbMessage.setRequestID(reqID);
        sdbMessage.setSkipRowsCount(skipNum);
        sdbMessage.setReturnRowsCount(returnNum);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage,
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);

        return rtnSDBMessage;
    }

    private SDBMessage adminCommandEval(String commandString, int flag, long reqID,
                                        long skipNum, long returnNum, BSONObject query,
                                        BSONObject selector, BSONObject order, BSONObject hint)
            throws BaseException {
        // Admin command request
        // int reqId = 0;
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage sdbMessage = new SDBMessage();

        if (query == null)
            sdbMessage.setMatcher(dummyObj);
        else
            sdbMessage.setMatcher(query);
        if (selector == null)
            sdbMessage.setSelector(dummyObj);
        else
            sdbMessage.setSelector(selector);
        if (order == null)
            sdbMessage.setOrderBy(dummyObj);
        else
            sdbMessage.setOrderBy(order);
        if (hint == null)
            sdbMessage.setHint(dummyObj);
        else
            sdbMessage.setHint(hint);

        sdbMessage.setCollectionFullName(SequoiadbConstants.ADMIN_PROMPT
                + commandString);

        sdbMessage.setVersion(1);
        sdbMessage.setW((short) 0);
        sdbMessage.setPadding((short) 0);
        sdbMessage.setFlags(flag);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        // sdbMessage.setResponseTo(reqId);
        // reqId++;
        if (0 == reqID) {
            reqID = getNextRequstID();
        }
        sdbMessage.setRequestID(reqID);
        sdbMessage.setSkipRowsCount(skipNum);
        sdbMessage.setReturnRowsCount(returnNum);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage,
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        //SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractEvalReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);

        return rtnSDBMessage;
    }

    private SDBMessage createCS(String csName, BSONObject options)
            throws BaseException {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CREATE_CMD + " "
                + SequoiadbConstants.COLSPACE;
        BSONObject cObj = new BasicBSONObject();
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage sdbMessage = new SDBMessage();

        cObj.put(SequoiadbConstants.FIELD_NAME_NAME, csName);
        if (null != options)
            cObj.putAll(options);
        sdbMessage.setMatcher(cObj);
        sdbMessage.setCollectionFullName(commandString);

        sdbMessage.setVersion(1);
        sdbMessage.setW((short) 0);
        sdbMessage.setPadding((short) 0);
        sdbMessage.setFlags(0);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        sdbMessage.setRequestID(getNextRequstID());
        sdbMessage.setSkipRowsCount(-1);
        sdbMessage.setReturnRowsCount(-1);
        sdbMessage.setSelector(dummyObj);
        sdbMessage.setOrderBy(dummyObj);
        sdbMessage.setHint(dummyObj);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage,
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        if (endianConvert) {
            byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            byteBuffer.order(ByteOrder.BIG_ENDIAN);
        }
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);

        return rtnSDBMessage;
    }

    private boolean requestSysInfo() {
        byte[] request = SDBMessageHelper.buildSysInfoRequest();
        connection.sendMessage(request);
        boolean endianConvert = SDBMessageHelper
                .msgExtractSysInfoReply(connection.receiveSysInfoMsg(128));
        return endianConvert;
    }

    private void sendKillContextMsg() {
        if (connection == null)
            throw new BaseException(SDBError.SDB_NETWORK);
        long[] contextIds = new long[]{-1};
        byte[] request = SDBMessageHelper.buildKillCursorMsg(0, contextIds,
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        if (rtnSDBMessage.getOperationCode() != Operation.OP_KILL_CONTEXT_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtnSDBMessage.getOperationCode().toString());
        }
        int flags = rtnSDBMessage.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    /**
     * @class SptEvalResult
     * @brief Class for executing stored procedure result.
     */
    public static class SptEvalResult {
        private SptReturnType returnType;
        private BSONObject errmsg;
        private DBCursor cursor;

        /**
         * @fn SptEvalResult ()
         * @brief Constructor.
         */
        public SptEvalResult() {
            returnType = null;
            errmsg = null;
            cursor = null;
        }

        /**
         * @fn setReturnType ()
         * @brief Set return type.
         */
        public void setReturnType(SptReturnType returnType) {
            this.returnType = returnType;
        }

        /**
         * @fn SptReturnType getReturnType ()
         * @brief Get return type.
         */
        public SptReturnType getReturnType() {
            return returnType;
        }

        /**
         * @fn setErrMsg ()
         * @brief Set error type.
         */
        public void setErrMsg(BSONObject errmsg) {
            this.errmsg = errmsg;
        }

        /**
         * @fn BSONObject getErrMsg ()
         * @brief Get error type.
         */
        public BSONObject getErrMsg() {
            return errmsg;
        }

        /**
         * @fn setCursor ()
         * @brief Set result cursor.
         */
        public void setCursor(DBCursor cursor) {
            if (this.cursor != null) {
                this.cursor.close();
            }
            this.cursor = cursor;
        }

        /**
         * @fn DBCursor getCursor ()
         * @brief Get result cursor.
         */
        public DBCursor getCursor() {
            return cursor;
        }
    }

    public enum SptReturnType {
        TYPE_VOID(0),
        TYPE_STR(1),
        TYPE_NUMBER(2),
        TYPE_OBJ(3),
        TYPE_BOOL(4),
        TYPE_RECORDSET(5),
        TYPE_CS(6),
        TYPE_CL(7),
        TYPE_RG(8),
        TYPE_RN(9);

        private int typeValue;

        private SptReturnType(int typeValue) {
            this.typeValue = typeValue;
        }

        public int getTypeValue() {
            return typeValue;
        }

        public static SptReturnType getTypeByValue(int typeValue) {
            SptReturnType retType = null;
            for (SptReturnType rt : values()) {
                if (rt.getTypeValue() == typeValue) {
                    retType = rt;
                    break;
                }
            }
            return retType;
        }
    }

    /**
     * @fn DBDataCenter getDataCenter()
     * @brief get the datacenter
     * @return DBDataCenter
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBDataCenter getDataCenter() throws BaseException {
        DBDataCenter dc = new DBDataCenterConcrete(this);
        return dc;
    }
}
