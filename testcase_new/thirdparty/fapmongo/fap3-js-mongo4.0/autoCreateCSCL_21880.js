/******************************************************************************
 * @Description   : seqDB-21880:自动创建CS/CL
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2020.02.24
 * @LastEditTime  : 2021.04.22
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
main();

function main ()
{
   var clName = "cl21880";
   var cl = db.getCollection( clName );
   db.dropDatabase();

   // test
   dbNotExist_dataOper( db, cl, clName );
   clNotExist_dataOper( cl, clName );
   repeatCreateCL( db, cl, clName );

   db.dropDatabase();
}

function dbNotExist_dataOper ( db, cl, clName )
{
   // createCollection
   db.createCollection( clName );
   var cl2 = db.getCollection( clName );
   assert.eq( cl2.count(), 0 );

   // insert
   db.dropDatabase();
   cl.insert( { "a": 1 } );
   assert.eq( cl.count(), 1 );

   // update
   db.dropDatabase();
   cl.update( {}, { "$set": { "a": 1 } }, { "upsert": true } );
   assert.eq( cl.count(), 1 );

   // createIndex
   db.dropDatabase();
   cl.createIndex( { a: 1 } );
   assert.eq( cl.getIndexes().length, 2 );

   // find
   cl.drop();
   assert.eq( cl.find().hasNext(), false )

   // count
   cl.drop();
   assert.eq( cl.count(), 0 )

   // aggregate
   cl.drop();
   assert.eq( cl.aggregate( { "$project": { "a": 1 } } ).hasNext(), false )

   // distinct
   cl.drop();
   assert.eq( cl.distinct( "b" ), [] );
}

function clNotExist_dataOper ( cl, clName )
{
   // db exist, cl not exist, auto create cl
   // insert
   cl.drop();
   cl.insert( { "a": 1 } );
   assert.eq( cl.count(), 1 );

   // update
   cl.drop();
   cl.update( {}, { "$set": { "a": 1 } }, { "upsert": true } );
   assert.eq( cl.count(), 1 );

   // createIndex
   cl.drop();
   var cl = db.getCollection( clName );
   cl.createIndex( { a: 1 } );
   assert.eq( cl.getIndexes().length, 2 );

   // find
   cl.drop();
   assert.eq( cl.find().hasNext(), false )

   // count
   cl.drop();
   assert.eq( cl.count(), 0 )

   // aggregate
   cl.drop();
   assert.eq( cl.aggregate( { "$project": { "a": 1 } } ).hasNext(), false )

   // distinct
   cl.drop();
   assert.eq( cl.distinct( "b" ), [] );
}

function repeatCreateCL ( db, cl, clName )
{
   cl.insert( { "a": 1 } );
   var rc = db.createCollection( clName );
   assert.eq( JSON.stringify( rc ), ["{\"ok\":0,\"code\":-22,\"errmsg\":\"Collection already exists\"}"] );
   var rc = db.getLastError();
   assert.eq( rc, "Collection already exists" );
}