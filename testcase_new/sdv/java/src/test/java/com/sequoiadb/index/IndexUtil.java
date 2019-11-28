package com.sequoiadb.index;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNotNull;

/**
 * Created by laojingtang on 18-1-12.
 */
public class IndexUtil {
    public static IndexEntity changeBson2IndexEntity( BSONObject o1 ) {
        BSONObject indexDef;
        if ( o1.containsField( "IndexDef" ) ) {
            indexDef = ( BSONObject ) o1.get( "IndexDef" );
        } else {
            indexDef = o1;
        }

        if ( indexDef.containsField( "name" ) && indexDef.containsField( "key" )
                && indexDef.containsField( "unique" )
                && indexDef.containsField( "enforced" ) ) {
            IndexEntity index = new IndexEntity();
            index.setIndexName( ( String ) indexDef.get( "name" ) );
            index.setKey( ( BasicBSONObject ) indexDef.get( "key" ) );
            index.setUnique( ( Boolean ) indexDef.get( "unique" ) );
            index.setEnforced( ( Boolean ) indexDef.get( "enforced" ) );
            return index;
        } else {
            return new IndexEntity();
        }
    }

    public static void assertIndexCreatedCorrect( DBCollection cl,
            IndexEntity entity ) {
        try ( DBCursor cursor = cl.getIndex( entity.getIndexName() )) {
            BSONObject object = cursor.getNext();
            assertNotNull( object, "index11413" );
            assertEquals( IndexUtil.changeBson2IndexEntity( object ), entity );
        }
    }
}
