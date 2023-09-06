/******************************************** 
@description : NumberDecimal参数类型测试
@testcase    : seqDB-33158
@author      : Zejia Chen 2023-09-06
*********************************************/
main();

function main ()
{
   var clName = "cl33158";
   var cl = db.getCollection( clName );
   cl.drop();

   cl.insert( { "_id": 0, "b": NumberDecimal( "1.59507354245143932673458186E-999" ) } );
   cl.insertOne( { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E-999" ) } );
   cl.bulkWrite( [{ insertOne: { document: { "_id": 2, "b": NumberDecimal( "1.59507354245143932673458186E-1000" ) } } }] );
   var docs = [
      { "_id": 3, "b": NumberDecimal( "-1.59507354245143932673458186E-1000" ) },
      { "_id": 4, "b": NumberDecimal( "1005950733333335424514393267458186E-1033" ) },
      { "_id": 5, "b": NumberDecimal( "-1005950733333335424514393267458186E-1033" ) },
      { "_id": 6, "b": NumberDecimal( 9223372036854775807 ) },
      { "_id": 7, "b": NumberDecimal( -1.78E-308 ) }
   ];
   cl.insertMany( docs );

   // check result
   var expDocs = [
      { "_id": 0, "b": NumberDecimal( "1.59507354245143932673458186E-999" ) },
      { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E-999" ) },
      { "_id": 2, "b": NumberDecimal( "1.59507354245143932673458186E-1000" ) },
      { "_id": 3, "b": NumberDecimal( "-1.59507354245143932673458186E-1000" ) },
      { "_id": 4, "b": NumberDecimal( "1.005950733333335424514393267458186E-1000" ) },
      { "_id": 5, "b": NumberDecimal( "-1.005950733333335424514393267458186E-1000" ) },
      { "_id": 6, "b": NumberDecimal( "9223372036854780000" ) },
      { "_id": 7, "b": NumberDecimal( "-1.78000000000000E-308" ) }
   ];
   var rc = cl.find();
   var actDocs = [];
   while( rc.hasNext() )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );
   rc.close();

   // invalid parameter
   try
   {
      cl.insert( { "_id": 1, "b": NumberDecimal( null ) } );
      throw new Error( "expect fail but actual success." );
   } catch( e )
   {
      if( e.message !== 'Unable to write Decimal128 value.' )
      {
         throw new Error( e );
      }
   }
   try
   {
      cl.insert( { "_id": 1, "b": NumberDecimal( { a: 1, b: 1 } ) } );
      throw new Error( "expect fail but actual success." );
   } catch( e )
   {
      if( e.message !== 'Unable to write Decimal128 value.' )
      {
         throw new Error( e );
      }
   }
   try
   {
      cl.insert( { "_id": 1, "b": NumberDecimal( [1, 2] ) } );
      throw new Error( "expect fail but actual success." );
   } catch( e )
   {
      if( e.message !== 'Unable to write Decimal128 value.' )
      {
         throw new Error( e );
      }
   }
   cl.drop();
}