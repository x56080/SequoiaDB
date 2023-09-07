/******************************************************************************
 * @Description   : seqDB-33204:aggregate() / aggregate([]) / aggregate({})
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.09.07
 * @LastEditTime  : 2023.09.07
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var clName = "cl_33204";
   var cl = db.getCollection( clName );
   cl.drop();
   var docs = [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }];
   cl.insert( docs );

   // aggregate()
   var rc = cl.aggregate();
   checkResults( rc, JSON.stringify( docs ) );

   // aggregate([])
   var rc = cl.aggregate( [] );
   checkResults( rc, JSON.stringify( docs ) );

   // aggregate({})
   try
   {
      cl.aggregate( {} );
      assert.eq( "success", "fail" );
   }
   catch( e )
   {
      assert.eq( e.code, -6 );
   }
   assert.eq( db.getLastError(), "Invalid Argument" );

   cl.drop();
}

function checkResults ( rc, expDocs )
{
   var docs = new Array();
   while( rc.hasNext() )
   {
      var doc = rc.next();
      docs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( docs.sort() ), expDocs );
}