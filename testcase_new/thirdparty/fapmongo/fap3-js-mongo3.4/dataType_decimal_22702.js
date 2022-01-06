/******************************************** 
@description : crud all dataType
@testcase    : seqDB-22702
@author      : XiaoNi Huang 2020-08-28
*********************************************/
main();

function main ()
{
   var clName = "cl22702";
   var cl = db.getCollection( clName );
   cl.drop();

   // insert
   var docs = [
      { "_id": 0, "b": NumberDecimal() },
      { "_id": 1, "b": NumberDecimal( 0 ) },
      { "_id": 2, "b": NumberDecimal( -0 ) },
      { "_id": 3, "b": NumberDecimal( -2147483648 ) },
      { "_id": 4, "b": NumberDecimal( 2147483647 ) },
      { "_id": 5, "b": NumberDecimal( -9223372036854775808 ) },
      { "_id": 6, "b": NumberDecimal( 9223372036854775807 ) },
      { "_id": 7, "b": NumberDecimal( -1.78E-308 ) },
      { "_id": 8, "b": NumberDecimal( 1.78E-308 ) },
      { "_id": 9, "b": NumberDecimal( -1.78E+308 ) },
      { "_id": 10, "b": NumberDecimal( 1.78E+308 ) },
      { "_id": 11, "b": NumberDecimal( 1.88888E+308 ) },
      { "_id": 12, "b": NumberDecimal( -1.59507354245143932673458186E+6150 ) },
      { "_id": 13, "b": NumberDecimal( -1.59507354245143932673458186E-6150 ) },
      { "_id": 14, "b": NumberDecimal( 1.59507354245143932673458186E+6150 ) },

      { "_id": 20, "b": NumberDecimal( "0" ) },
      { "_id": 21, "b": NumberDecimal( "-0" ) },
      { "_id": 22, "b": NumberDecimal( "00" ) },
      { "_id": 23, "b": NumberDecimal( "-00" ) },
      { "_id": 24, "b": NumberDecimal( "00.00" ) },
      { "_id": 25, "b": NumberDecimal( "-00.00" ) },
      { "_id": 26, "b": NumberDecimal( "0.000000000000000" ) },
      { "_id": 27, "b": NumberDecimal( "-0.000000000000000" ) },
      { "_id": 28, "b": NumberDecimal( "-2147483648.00000" ) },
      { "_id": 29, "b": NumberDecimal( "2147483647.00000" ) },
      { "_id": 30, "b": NumberDecimal( "-9223372036854775808.000" ) },
      { "_id": 31, "b": NumberDecimal( "9223372036854775807.000" ) },
      { "_id": 32, "b": NumberDecimal( "-1.78E-308" ) },
      { "_id": 33, "b": NumberDecimal( "1.78E-308" ) },
      { "_id": 34, "b": NumberDecimal( "-1.78E+308" ) },
      { "_id": 35, "b": NumberDecimal( "1.78E+308" ) },
      { "_id": 36, "b": NumberDecimal( "1.88888E+308" ) },
      { "_id": 37, "b": NumberDecimal( "-1.59507354245143932673458186E+974" ) },
      { "_id": 38, "b": NumberDecimal( "-1.59507354245143932673458186E-974" ) },
      { "_id": 39, "b": NumberDecimal( "1.59507354245143932673458186E+974" ) },

      { "_id": 50, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 51, "b": NumberDecimal( "Infinity" ) },
      { "_id": 52, "b": NumberDecimal( "-Inf" ) },
      { "_id": 53, "b": NumberDecimal( "Inf" ) },
      { "_id": 54, "b": NumberDecimal( "NaN" ) },
      { "_id": 55, "b": NumberDecimal( "+NaN" ) },
      { "_id": 56, "b": NumberDecimal( "-NaN" ) },

      // 总精度：1000，小数精度：999
      { "_id": 60, "b": NumberDecimal( "9.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" ) },
      // 总精度：1001，小数精度：1000
      { "_id": 61, "b": NumberDecimal( "9.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" ) }
   ];
   cl.insert( docs );

   // find
   var expDocs = [
      { "_id": 0, "b": NumberDecimal( "0" ) },
      { "_id": 1, "b": NumberDecimal( "0" ) },
      { "_id": 2, "b": NumberDecimal( "0" ) },
      { "_id": 3, "b": NumberDecimal( "-2147483648.00000" ) },
      { "_id": 4, "b": NumberDecimal( "2147483647.00000" ) },
      { "_id": 5, "b": NumberDecimal( "-9223372036854780000" ) },
      { "_id": 6, "b": NumberDecimal( "9223372036854780000" ) },
      { "_id": 7, "b": NumberDecimal( "-1.78000000000000E-308" ) },
      { "_id": 8, "b": NumberDecimal( "1.78000000000000E-308" ) },
      { "_id": 9, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 10, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 11, "b": NumberDecimal( "Infinity" ) },
      { "_id": 12, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 13, "b": NumberDecimal( "0" ) },
      { "_id": 14, "b": NumberDecimal( "Infinity" ) },

      { "_id": 20, "b": NumberDecimal( "0" ) },
      { "_id": 21, "b": NumberDecimal( "0" ) },
      { "_id": 22, "b": NumberDecimal( "0" ) },
      { "_id": 23, "b": NumberDecimal( "0" ) },
      { "_id": 24, "b": NumberDecimal( "0.00" ) },
      { "_id": 25, "b": NumberDecimal( "0.00" ) },
      { "_id": 26, "b": NumberDecimal( "0E-15" ) },
      { "_id": 27, "b": NumberDecimal( "0E-15" ) },
      { "_id": 28, "b": NumberDecimal( "-2147483648.00000" ) },
      { "_id": 29, "b": NumberDecimal( "2147483647.00000" ) },
      { "_id": 30, "b": NumberDecimal( "-9223372036854775808.000" ) },
      { "_id": 31, "b": NumberDecimal( "9223372036854775807.000" ) },
      { "_id": 32, "b": NumberDecimal( "-1.78E-308" ) },
      { "_id": 33, "b": NumberDecimal( "1.78E-308" ) },
      { "_id": 34, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 35, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 36, "b": NumberDecimal( "1.888880000000000000000000000000000E+308" ) },
      { "_id": 37, "b": NumberDecimal( "-1.595073542451439326734581860000000E+974" ) },
      { "_id": 38, "b": NumberDecimal( "-1.59507354245143932673458186E-974" ) },
      { "_id": 39, "b": NumberDecimal( "1.595073542451439326734581860000000E+974" ) },

      { "_id": 50, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 51, "b": NumberDecimal( "Infinity" ) },
      { "_id": 52, "b": NumberDecimal( "-Infinity" ) },
      { "_id": 53, "b": NumberDecimal( "Infinity" ) },
      { "_id": 54, "b": NumberDecimal( "NaN" ) },
      { "_id": 55, "b": NumberDecimal( "NaN" ) },
      { "_id": 56, "b": NumberDecimal( "NaN" ) },

      { "_id": 60, "b": NumberDecimal( "9.000000000000000000000000000000000" ) },
      { "_id": 61, "b": NumberDecimal( "9.000000000000000000000000000000000" ) }
   ];
   var actDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      assert.eq( cl.count( docs[i] ), 1 );

      var rc = cl.find( docs[i] );
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );


   // remove
   // update后有一次remove，但是数据覆盖不全，在insert后remove，update前重新准备数据  
   for( var i = 0; i < docs.length; i++ )
   {
      var rc = cl.remove( expDocs[i] );
      assert.eq( rc, { "nRemoved": 1 } );
   }
   assert.eq( cl.count(), 0 );


   // update
   cl.remove( {} );
   cl.insert( docs );

   var expDocs = [
      { "_id": 0, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 1, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 2, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 3, "b": NumberDecimal( "2147481499516352.00000" ) },
      { "_id": 4, "b": NumberDecimal( "2147485794483647.00000" ) },
      { "_id": 5, "b": NumberDecimal( "-9221224553207780000" ) },
      { "_id": 6, "b": NumberDecimal( "9225519520501780000" ) },
      { "_id": 7, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 8, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 9, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 10, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 11, "b": NumberDecimal( "NaN" ) },
      { "_id": 12, "b": NumberDecimal( "NaN" ) },
      { "_id": 13, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 14, "b": NumberDecimal( "NaN" ) },

      { "_id": 20, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 21, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 22, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 23, "b": NumberDecimal( "2147483647000000" ) },
      { "_id": 24, "b": NumberDecimal( "2147483647000000.00" ) },
      { "_id": 25, "b": NumberDecimal( "2147483647000000.00" ) },
      { "_id": 26, "b": NumberDecimal( "2147483647000000.000000000000000" ) },
      { "_id": 27, "b": NumberDecimal( "2147483647000000.000000000000000" ) },
      { "_id": 28, "b": NumberDecimal( "2147481499516352.00000" ) },
      { "_id": 29, "b": NumberDecimal( "2147485794483647.00000" ) },
      { "_id": 30, "b": NumberDecimal( "-9221224553207775808.000" ) },
      { "_id": 31, "b": NumberDecimal( "9225519520501775807.000" ) },
      { "_id": 32, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 33, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 34, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 35, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 36, "b": NumberDecimal( "1.888880000000000000000000000000000E+308" ) },
      { "_id": 37, "b": NumberDecimal( "-1.595073542451439326734581860000000E+974" ) },
      { "_id": 38, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 39, "b": NumberDecimal( "1.595073542451439326734581860000000E+974" ) },

      { "_id": 50, "b": NumberDecimal( "NaN" ) },
      { "_id": 51, "b": NumberDecimal( "NaN" ) },
      { "_id": 52, "b": NumberDecimal( "NaN" ) },
      { "_id": 53, "b": NumberDecimal( "NaN" ) },
      { "_id": 54, "b": NumberDecimal( "NaN" ) },
      { "_id": 55, "b": NumberDecimal( "NaN" ) },
      { "_id": 56, "b": NumberDecimal( "NaN" ) },

      { "_id": 60, "b": NumberDecimal( "2147483647000009.000000000000000000" ) },
      { "_id": 61, "b": NumberDecimal( "2147483647000009.000000000000000000" ) }
   ];

   var rc = cl.update( {}, { "$inc": { "b": NumberDecimal( 2147483647000000 ) } }, { "multi": true } );
   assert.eq( rc, { "nMatched": docs.length, "nUpserted": 0, "nModified": docs.length } );

   // check result for update
   var rc = cl.find();
   var actDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );


   // remove
   for( var i = 0; i < expDocs.length; i++ )
   {
      var rc = cl.remove( expDocs[i] );
   }

   // check result for remove
   var expCnt = 12;
   assert.eq( cl.count(), expCnt );

   var expDocs = [
      { "_id": 7, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 8, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 9, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 10, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 32, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 33, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 34, "b": NumberDecimal( "-1.780000000000000000000000000000000E+308" ) },
      { "_id": 35, "b": NumberDecimal( "1.780000000000000000000000000000000E+308" ) },
      { "_id": 36, "b": NumberDecimal( "1.888880000000000000000000000000000E+308" ) },
      { "_id": 37, "b": NumberDecimal( "-1.595073542451439326734581860000000E+974" ) },
      { "_id": 38, "b": NumberDecimal( "2147483647000000.000000000000000000" ) },
      { "_id": 39, "b": NumberDecimal( "1.595073542451439326734581860000000E+974" ) },
   ];
   var rc = cl.find();
   var actDocs = [];
   for( var i = 0; i < expCnt; i++ )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );


   // 精度小于等于1000
   var docs = [
      { "_id": 0, "b": NumberDecimal( "1.59507354245143932673458186E-999" ) },
      { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E-999" ) },
      { "_id": 2, "b": NumberDecimal( "1.59507354245143932673458186E-1000" ) },
      { "_id": 3, "b": NumberDecimal( "-1.59507354245143932673458186E-1000" ) },
      { "_id": 4, "b": NumberDecimal( "1005950733333335424514393267458186E-1033" ) },
      { "_id": 5, "b": NumberDecimal( "-1005950733333335424514393267458186E-1033" ) },
      { "_id": 6, "b": NumberDecimal( "1.005950733333335424514393267458186E+1033" ) },
      { "_id": 7, "b": NumberDecimal( "-1.005950733333335424514393267458186E+1033" ) }
   ];
   cl.remove( {} );
   cl.insert( docs );

   // check result
   var expDocs = [
      { "_id": 0, "b": NumberDecimal( "1.59507354245143932673458186E-999" ) },
      { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E-999" ) },
      { "_id": 2, "b": NumberDecimal( "1.59507354245143932673458186E-1000" ) },
      { "_id": 3, "b": NumberDecimal( "-1.59507354245143932673458186E-1000" ) },
      { "_id": 4, "b": NumberDecimal( "1.005950733333335424514393267458186E-1000" ) },
      { "_id": 5, "b": NumberDecimal( "-1.005950733333335424514393267458186E-1000" ) },
      { "_id": 6, "b": NumberDecimal( "1.005950733333335424514393267458186E+1033" ) },
      { "_id": 7, "b": NumberDecimal( "-1.005950733333335424514393267458186E+1033" ) }
   ];
   var rc = cl.find();
   var actDocs = [];
   for( var i = 0; i < docs.length; i++ )
   {
      var doc = rc.next();
      actDocs.push( doc );
   }
   assert.eq( JSON.stringify( actDocs ), JSON.stringify( expDocs ) );


   // 精度大于1000
   cl.remove( {} );
   var docs = [
      { "_id": 0, "b": NumberDecimal( "1.59507354245143932673458186E-1001" ) },
      { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E-1001" ) },
      { "_id": 3, "b": NumberDecimal( "1.005950733333335424514393267458186E+1034" ) },
      { "_id": 4, "b": NumberDecimal( "-1.005950733333335424514393267458186E+1034" ) },
      { "_id": 5, "b": NumberDecimal( "1005950733333335424514393267458186E-1034" ) },
      { "_id": 6, "b": NumberDecimal( "-1005950733333335424514393267458186E-1034" ) },
      { "_id": 7, "b": NumberDecimal( "-1.59507354245143932673458186E+6144" ) }
   ];

   for( var i = 0; i < docs.length; i++ )
   {
      var rc = cl.insert( docs[i] );
      assert.eq( rc, { "writeError": { "code": -6, "errmsg": "Invalid decimal" } } );
      assert.eq( db.getLastError(), "Invalid decimal" );
   }
   assert.eq( cl.count(), 0 );


   // out of range of decimal128
   cl.remove( {} );
   try
   {
      cl.insert( { "_id": 1, "b": NumberDecimal( "-1.59507354245143932673458186E+6145" ) } );
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      if( e.message !== 'Input out of range of Decimal128 value (inexact).' )
      {
         throw new Error( e );
      }
   }
   assert.eq( cl.count(), 0 );

   cl.drop();
}