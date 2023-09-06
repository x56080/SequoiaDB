/******************************************** 
@description : 修改decimal数据
@testcase    : seqDB-33162
@author      : Zejia Chen 2023-09-06
*********************************************/
main();

function main ()
{
   var clName = "cl33158";
   var cl = db.getCollection( clName );
   cl.drop();

   var docs = [{ "_id": 1, "b": NumberDecimal( "1.2345E+50" ) },
   { "_id": 2, "b": NumberDecimal( "2.2345E+50" ) },
   { "_id": 3, "b": NumberDecimal( "3.2345E+50" ) },
   { "_id": 4, "b": NumberDecimal( "4.2345E+50" ) },
   { "_id": 5, "b": NumberDecimal( "5.2345E+50" ) }
   ];
   cl.insert( docs );

   // update $set
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "6.660000000000000000000000000000000E+50" ) }
   ];
   cl.update( { "_id": 1 }, { $set: { "b": NumberDecimal( "6.66E+50" ) } } );
   var rc = cl.find( { "_id": 1 } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );

   // update $inc
   var rc = cl.update( {}, { "$inc": { "b": NumberDecimal( 2147483647000000 ) } }, { "multi": true } );
   assert.eq( rc, { "nMatched": docs.length, "nUpserted": 0, "nModified": docs.length } );

   // check result for update
   expDocs = [
      { "_id": 1, "b": NumberDecimal( "6.660000000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) },
      { "_id": 4, "b": NumberDecimal( "4.234500000000000000000000000000000E+50" ) },
      { "_id": 5, "b": NumberDecimal( "5.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find();
   var actDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // 使用原先记录查找匹配不到记录，说明decimal数据已经被修改
   for( var i = 0; i < expDocs.length; i++ )
   {
      var rc = cl.count( expDocs[i] );
      assert.eq( rc, 0 );
   }

   cl.drop();
}