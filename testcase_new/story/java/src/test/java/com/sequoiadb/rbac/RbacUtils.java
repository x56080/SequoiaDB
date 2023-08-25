package com.sequoiadb.rbac;

import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.*;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;

public class RbacUtils {

    public static void findActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean skipNotSupported ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );

        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行权限支持的操作
        DBCursor cursor = null;
        cursor = dbcl.query();
        cursor.getNext();
        cursor.close();

        dbcl.queryOne();

        dbcl.getCount();

        cursor = dbcl.listLobs();
        cursor.getNext();
        cursor.close();

        dbcl.getIndexInfo( indexName );

        cursor = dbcl.getIndexes();
        cursor.getNext();
        cursor.close();

        sdb.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        dbcl.getIndexStat( indexName );

        DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ );
        byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
        rLob.read( rbuff );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.createLob();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 20000 ) ),
                        0, -1, 0, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // try {
            // cursor = dbcl.queryAndRemove( new BasicBSONObject( "a", 1 ),
            // null, null, null, -1, -1, 0 );
            // cursor.getNext();
            // cursor.close();
            // Assert.fail( "should error but success" );
            // } catch ( BaseException e ) {
            // if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
            // .getErrorCode() ) {
            // throw e;
            // }
            // }
        }

        rootCL.dropIndex( indexName );
        rootCL.truncate();
    }

    public static void insertActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean skipNotSupported ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = dbcl.createLob();
        lob.write( wlobBuff );
        lob.close();
        lob.getID();

        dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                String indexName = "index_" + clName;
                dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.updateRecords( new BasicBSONObject( "a", 2 ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 3 ) ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void updateActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean skipNotSupported ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行权限支持的操作
        // upsert插入数据报错
        // dbcl.upsertRecords( new BasicBSONObject( "a", 3 ),
        // new BasicBSONObject( "$set", new BasicBSONObject( "a", 1 ) ) );
        dbcl.updateRecords( new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 2 ) ) );
        dbcl.updateRecords( new BasicBSONObject( "a", 2 ),
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 1 ) ) );

        // 执行lob偏移写操作

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void removeActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean skipNotSupported ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行权限支持的操作
        dbcl.removeLob( oid );
        dbcl.deleteRecords( new BasicBSONObject( "a", 1 ) );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                DBCursor cursor = dbcl.queryAndRemove(
                        new BasicBSONObject( "a", 1 ), null, null, null, -1, -1,
                        0 );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void getDetailActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean skipNotSupported ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );
        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );

        // 执行权限支持的操作
        dbcl.getCount();

        dbcl.getIndexInfo( indexName );

        dbcl.getIndexes();

        sdb.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        dbcl.getIndexStat( indexName );

        DBCursor cursor = dbcl.listLobs();
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.createLob();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.dropIndex( indexName );
        rootCL.truncate();
    }

    public static void createIndexActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean skipNotSupported ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        String indexName = "index_" + clName;
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        rootCL.dropIndex( indexName );
        long taskId = dbcl.createIndexAsync( indexName,
                new BasicBSONObject( "a", 1 ), null, null );
        long[] taskIds = new long[ 1 ];
        taskIds[ 0 ] = taskId;
        sdb.waitTasks( taskIds );
        rootCL.dropIndex( indexName );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            // 重建一个集合不包含_id索引
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.alterCollection( new BasicBSONObject( "ReplSize", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropIndexActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean skipNotSupported ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        dbcl.dropIndex( indexName );

        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        long taskId = dbcl.dropIndexAsync( indexName );
        long[] taskIds = new long[ 1 ];
        taskIds[ 0 ] = taskId;
        sdb.waitTasks( taskIds );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.dropIdIndex();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                BasicBSONObject options = new BasicBSONObject();
                options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
                options.put( "ShardingType", "hash" );
                dbcl.enableSharding( options );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean skipNotSupported ) {
        // 执行权限支持的操作
        String indexName = "index_" + clName;

        dbcl.alterCollection( new BasicBSONObject( "ReplSize", -1 ) );

        BasicBSONObject options = new BasicBSONObject();
        String fieldName = "field_" + clName;
        options.put( "Field", fieldName );
        options.put( "AcquireSize", 1 );
        dbcl.createAutoIncrement( options );
        dbcl.dropAutoIncrement( fieldName );
        dbcl.dropIdIndex();
        dbcl.createIdIndex( null );
        dbcl.disableCompression();
        options.clear();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "hash" );
        dbcl.enableSharding( options );
        dbcl.disableSharding();
        dbcl.setAttributes( new BasicBSONObject( "ReplSize", 1 ) );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterCSActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean skipNotSupported ) {
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        // 执行权限支持的操作
        String domainName1 = "domain_1_" + csName;
        String domainName2 = "domain_2_" + csName;

        if ( sdb.isDomainExist( domainName1 ) ) {
            sdb.dropDomain( domainName1 );
        }

        if ( sdb.isDomainExist( domainName2 ) ) {
            sdb.dropDomain( domainName2 );
        }

        sdb.createDomain( domainName1,
                new BasicBSONObject( "Groups", groupsName ) );
        sdb.createDomain( domainName2,
                new BasicBSONObject( "Groups", groupsName ) );

        dbcs.alterCollectionSpace(
                new BasicBSONObject( "Domain", domainName1 ) );
        dbcs.alterCollectionSpace(
                new BasicBSONObject( "Domain", domainName2 ) );
        dbcs.removeDomain();
        dbcs.setDomain( new BasicBSONObject( "Domain", domainName1 ) );
        dbcs.removeDomain();

        DBCollection dbcl = dbcs.getCollection( clName );
        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropDomain( domainName1 );
        sdb.dropDomain( domainName2 );
    }

    public static void createCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean skipNotSupported ) {
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );

        // 执行权限支持的操作
        String testCLName = clName + "test_create_cl";
        dbcs.createCollection( testCLName );

        DBCollection dbcl = dbcs.getCollection( testCLName );
        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.dropCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.dropCollection( testCLName );
    }

    public static void dropCLActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, CollectionSpace dbcs, boolean skipNotSupported ) {
        String testCLName = clName + "test_create_cl";
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( testCLName );

        // 执行权限支持的操作
        dbcs.dropCollection( testCLName );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void renameCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean skipNotSupported ) {
        String testCLName = clName + "test_create_cl";
        String testCLNameNew = clName + "test_create_cl_new";
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( testCLName );

        // 执行权限支持的操作
        dbcs.renameCollection( testCLName, testCLNameNew );

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.createCollection( testCLNameNew );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.dropCollection( testCLNameNew );
    }

    public static void findActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, CollectionSpace dbcs, boolean skipNotSupported ) {
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        String domainName = "domain_" + csName;
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }

        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupsName ) );

        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.setDomain( new BasicBSONObject( "Domain", domainName ) );

        // 执行权限支持的操作
        dbcs.getDomainName();

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.removeDomain();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.removeDomain();
        sdb.dropDomain( domainName );
    }

    public static void getDetailActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean skipNotSupported ) {
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        String domainName = "domain_" + csName;
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }

        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupsName ) );

        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.setDomain( new BasicBSONObject( "Domain", domainName ) );

        // 执行权限支持的操作
        // dbcs.getDomainName();
        // String domain = dbcs.getDomainName();
        // System.out.println( "domain -- " + domain );
        // List< String > test = dbcs.getCollectionNames();

        // 执行部分不支持的操作
        if ( skipNotSupported ) {
            try {
                dbcs.removeDomain();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.removeDomain();
        sdb.dropDomain( domainName );
    }

    public static void removeUser( Sequoiadb sdb, String user,
            String password ) {
        try {
            sdb.removeUser( user, password );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_AUTH_USER_NOT_EXIST.getErrorCode() );
        }
    }

    public static void dropRole( Sequoiadb sdb, String roleName ) {
        try {
            sdb.dropRole( roleName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_AUTH_ROLE_NOT_EXIST.getErrorCode() );
        }
    }

    /**
     * generating byte to write lob
     *
     * @param length
     *            generating byte stream size
     * @return byte[] bytes
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    public static String[] getRandomActions( String[] actions, int count ) {
        if ( count <= 0 || count > actions.length ) {
            throw new IllegalArgumentException( "Invalid count value" );
        }

        Random random = new Random();
        String[] randomActions = new String[ count ];

        for ( int i = 0; i < count; i++ ) {
            int randomIndex;
            do {
                randomIndex = random.nextInt( actions.length );
            } while ( contains( randomActions, actions[ randomIndex ] ) );

            randomActions[ i ] = actions[ randomIndex ];
        }

        return randomActions;
    }

    public static boolean contains( String[] array, String value ) {
        for ( String element : array ) {
            if ( element != null && element.equals( value ) ) {
                return true;
            }
        }
        return false;
    }

    public static String arrayToCommaSeparatedString( String[] array ) {
        StringBuilder sb = new StringBuilder();

        for ( String element : array ) {
            if ( sb.length() > 0 ) {
                sb.append( ", " );
            }
            sb.append( "'" ).append( element ).append( "'" );
        }

        return sb.toString();
    }

    public static boolean compareBSONListsIgnoreOrder( BasicBSONList list1,
            BasicBSONList list2 ) {
        if ( list1.size() != list2.size() )
            return false;

        Set< Object > set1 = new HashSet<>( list1 );
        Set< Object > set2 = new HashSet<>( list2 );

        return set1.equals( set2 );
    }
}
