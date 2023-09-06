/******************************************** 
@description : 查询decimal数据
@testcase    : seqDB-33160
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

   // findAll
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) },
      { "_id": 4, "b": NumberDecimal( "4.234500000000000000000000000000000E+50" ) },
      { "_id": 5, "b": NumberDecimal( "5.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find();
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $eq
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $eq: NumberDecimal( "1.2345E+50" ) } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $gt
   var expDocs = [
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) },
      { "_id": 4, "b": NumberDecimal( "4.234500000000000000000000000000000E+50" ) },
      { "_id": 5, "b": NumberDecimal( "5.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $gt: NumberDecimal( "1.2345E+50" ) } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $lt
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $lt: NumberDecimal( "3.2345E+50" ) } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $ne
   var expDocs = [
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) },
      { "_id": 4, "b": NumberDecimal( "4.234500000000000000000000000000000E+50" ) },
      { "_id": 5, "b": NumberDecimal( "5.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $ne: NumberDecimal( "1.2345E+50" ) } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $in
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $in: [NumberDecimal( "1.2345E+50" ), NumberDecimal( "2.2345E+50" ), NumberDecimal( "3.2345E+50" )] } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   // $exists
   var expDocs = [
      { "_id": 1, "b": NumberDecimal( "1.234500000000000000000000000000000E+50" ) },
      { "_id": 2, "b": NumberDecimal( "2.234500000000000000000000000000000E+50" ) },
      { "_id": 3, "b": NumberDecimal( "3.234500000000000000000000000000000E+50" ) },
      { "_id": 4, "b": NumberDecimal( "4.234500000000000000000000000000000E+50" ) },
      { "_id": 5, "b": NumberDecimal( "5.234500000000000000000000000000000E+50" ) }
   ];
   var rc = cl.find( { "b": { $exists: true } } );
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   rc.close();
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );

   cl.drop();
}