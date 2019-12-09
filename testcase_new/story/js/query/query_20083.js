/******************************************************************************
*@Description : seqDB-20083:指定sel字段为$include:0且与排序字段不相同相同，执行查询
*@author      : Zhao xiaoni
*@Date        : 2019-10-24
******************************************************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw new Error( e );
}

function main ()
{
   var clName = "cl_20083";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //insert records and get expect result
   var insertNum = 100;
   var records = [];
   var expResult = [];
   for( var i = 0; i < insertNum; i++ )
   {
      records.push( { a: i, b: ( insertNum - i ) } );
      expResult.push( { b: ( i + 1 ) } );
   }
   cl.insert( records );

   //query
   var sel = { _id: { "$include": 0 }, a: { "$include": 0 } };
   var sort = { b: 1 };
   var cursor = cl.find( {}, sel ).sort( sort );

   //get actual result
   var actResult = [];
   while( cursor.next() )
   {
      actResult.push( cursor.current().toObj() );
   }

   //check Result
   if( actResult.length !== expResult.length )
   {
      throw new Error( "actResult.length: " + actResult.length + "is not equals to expResult.length: " + expResult.length );
   }
   for( var i = 0; i < actResult.length; i++ )
   {
      if( JSON.stringify( actResult[i] ) !== JSON.stringify( expResult[i] ) )
      {
         throw new Error( "actResult[" + i + "]: " + JSON.stringify( actResult[i] ) + "is not equals to expResult[" + i + "]: " + JSON.stringify( expResult[i] ) );
      }
   }
   commDropCL( db, COMMCSNAME, clName, false, false );
}
