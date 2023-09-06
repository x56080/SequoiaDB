/******************************************** 
@description : 删除decimal数据
@testcase    : seqDB-33161
@author      : Zejia Chen 2023-09-06
*********************************************/
main();

function main ()
{
   var clName = "cl33158";
   var cl = db.getCollection( clName );
   cl.drop();

   var doc = [{ "_id": 1, "b": NumberDecimal( "1.2345E+50" ) },
   { "_id": 2, "b": NumberDecimal( "2.2345E+50" ) },
   { "_id": 3, "b": NumberDecimal( "3.2345E+50" ) },
   { "_id": 4, "b": NumberDecimal( "4.2345E+50" ) },
   { "_id": 5, "b": NumberDecimal( "5.2345E+50" ) }
   ];
   cl.insert( doc );

   // remove by filter
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) }
   ];
   cl.remove( { "b": { $gt: NumberDecimal( "3.2345E+50" ) } } );
   var actDocs = [];
   var rc = cl.find();
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   rc.close();

   // remove all
   cl.remove( {} );
   assert.eq( cl.count(), 0 );

   cl.drop();
}