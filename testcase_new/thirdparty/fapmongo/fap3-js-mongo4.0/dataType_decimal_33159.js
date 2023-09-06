/******************************************** 
@description : numberDecimal参数取值测试
@testcase    : seqDB-33159
@author      : Zejia Chen 2023-09-06
*********************************************/
main();

function main ()
{
   var clName = "cl33159";
   var cl = db.getCollection( clName );
   cl.drop();

   // 34位有效数字,指数为-1000
   cl.insert( { "_id": 0, "b": NumberDecimal( "1.595073542451439326734581868976865E-1000" ) } );
   // 34位有效数字,指数为1000
   cl.insert( { "_id": 1, "b": NumberDecimal( "-1.595073542451439326734581868976865E+1000" ) } );
   // 35位有效数字
   try
   {
      cl.insert( { "_id": 2, "b": NumberDecimal( "15950735424514393267345818689768651E-1000" ) } );
      throw new Error( "expect fail but actual success." );
   } catch( e )
   {
      if( e.message !== 'Input out of range of Decimal128 value (inexact).' )
      {
         throw new Error( e );
      }
   }
   // 指数小于-1000或大于1000
   var docs = [
      { "_id": 3, "b": NumberDecimal( "1.59507354245143932673458186E-1001" ) },
      { "_id": 4, "b": NumberDecimal( "-1.59507354245143932673458186E-1001" ) },
      { "_id": 5, "b": NumberDecimal( "1.005950733333335424514393267458186E+1034" ) },
      { "_id": 6, "b": NumberDecimal( "-1.005950733333335424514393267458186E+1034" ) },
      { "_id": 7, "b": NumberDecimal( "1005950733333335424514393267458186E-1034" ) },
      { "_id": 8, "b": NumberDecimal( "-1005950733333335424514393267458186E-1034" ) },
      { "_id": 9, "b": NumberDecimal( "-1E+1001" ) },
      { "_id": 10, "b": NumberDecimal( "1E+1001" ) },
   ];
   for( var i = 0; i < docs.length; i++ )
   {
      var rc = cl.insert( docs[i] );
      assert.eq( rc, {
         "nInserted": 0,
         "writeError": {
            "code": -6,
            "errmsg": "Invalid decimal"
         }
      } );
      assert.eq( db.getLastError(), "Invalid decimal" );
   }
   // 插入特殊值
   var docs = [
      { "_id": 11, "b": NumberDecimal( "0" ) },
      { "_id": 12, "b": NumberDecimal( "-0" ) },
      { "_id": 13, "b": NumberDecimal( "+0" ) },
      { "_id": 14, "b": NumberDecimal( "NaN" ) },
      { "_id": 15, "b": NumberDecimal( "-NaN" ) },
      { "_id": 16, "b": NumberDecimal( "+NaN" ) },
      { "_id": 17, "b": NumberDecimal( "Infinity" ) },
      { "_id": 18, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 19, "b": NumberDecimal( "+Infinity" ) },
      { "_id": 20, "b": NumberDecimal( "Inf" ) },
      { "_id": 21, "b": NumberDecimal( "-Inf" ) },
      { "_id": 22, "b": NumberDecimal( "+Inf" ) },
      { "_id": 23, "b": NumberDecimal() },
   ];
   cl.insert( docs );

   // check result
   var expDocs = [
      { "_id": 0, "b": NumberDecimal( "1.595073542451439326734581868976865E-1000" ) },
      { "_id": 1, "b": NumberDecimal( "-1.595073542451439326734581868976865E+1000" ) },
      { "_id": 11, "b": NumberDecimal( "0" ) },
      { "_id": 12, "b": NumberDecimal( "0" ) },
      { "_id": 13, "b": NumberDecimal( "0" ) },
      { "_id": 14, "b": NumberDecimal( "NaN" ) },
      { "_id": 15, "b": NumberDecimal( "NaN" ) },
      { "_id": 16, "b": NumberDecimal( "NaN" ) },
      { "_id": 17, "b": NumberDecimal( "Infinity" ) },
      { "_id": 18, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 19, "b": NumberDecimal( "Infinity" ) },
      { "_id": 20, "b": NumberDecimal( "Infinity" ) },
      { "_id": 21, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 22, "b": NumberDecimal( "Infinity" ) },
      { "_id": 23, "b": NumberDecimal( "0" ) },
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
   cl.drop();
}