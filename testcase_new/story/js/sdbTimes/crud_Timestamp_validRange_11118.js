/************************************************************************
*@Description:  Timestamp, valid range
*testcases:        seqDB-11118、seqDB-11119、seqDB-11120、seqDB-11121
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11118_1" ;
      
      //get local time or millisecond
      var t1 = cmdRun("date -d '1902-01-01' +%s");  
      var t2 = cmdRun("date -d '2037-12-31' +%s"); 
      var rawData = [ { a: 0, b: Timestamp() }, 
                      { a: 1, b: Timestamp( "1902-01-01-00.00.00.000000" ) }, 
                      { a: 2, b: Timestamp( "2037-12-31-23.59.59.999999" ) }, 
                      
                      { a: 3, b: Timestamp( "1901-12-31T15:54:03.000Z" ) },  
                      { a: 4, b: Timestamp( "2037-12-31T15:59:59.999Z" ) },  
                      
                      { a: 5, b: Timestamp( "1902-01-01T00:00:00.000+0800" ) },  
                      { a: 6, b: Timestamp( "2037-12-31T23:59:59.999+0800" ) },  
                      
                      { a: 7, b: Timestamp( Number( t1 ), 0 ) },  
                      { a: 8, b: Timestamp( Number( t2 ) + 86399, 0 ) },  //86399999=23:59:59
                      
                      { a: 9, b: Timestamp( -2147483648, 0 ) }, 
                      { a:10, b: Timestamp(  2147483647, 0 ) } ];  
      
      var cl = readyCL( clName );
      
      //crud  
      insertRecs( cl, rawData );
      findAndCheckResult( cl, rawData, t1 );
      updateAndCheckResult( cl, rawData );
      removeAndCheckResult( cl, rawData );
   
      cleanCL( clName );
   }
      catch(e)
   {
   	throw e;
   }
}

function insertRecs( cl, rawData )
{
   println("\n---Begin to insert records.");
   
   cl.insert( rawData );
}

function findAndCheckResult( cl, rawData, t1 )
{
   println("\n---Begin to find and check result.");
   
   //find, exact matching each record
   var rcData = [];
   for( i = 1; i < rawData.length; i++ )
   {
      var cursor = cl.find( {$and :[ rawData[i], {a:{$ne:5}} ] }, {_id:{$include:0}} ).sort({a:1});
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   
   //check result
   var localtime1 = turnLocaltime( '1901-12-31T15:54:03.000Z', '%Y-%m-%d-%H.%M.%S.000000' );
   var localtime2 = cmdRun( 'date -d@"'+ t1 +'" "+%Y-%m-%d-%H.%M.%S.000000"' );
   var localtime3 = cmdRun( 'date -d@"-2147483648" "+%Y-%m-%d-%H.%M.%S.000000"' );
   
   var expRecs = '[{"a":1,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":2,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"a":3,"b":{"$timestamp":"'+ localtime1 +'"}},{"a":4,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":6,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":7,"b":{"$timestamp":"'+ localtime2 +'"}},{"a":8,"b":{"$timestamp":"2037-12-31-23.59.59.000000"}},{"a":9,"b":{"$timestamp":"'+ localtime3 +'"}},{"a":10,"b":{"$timestamp":"2038-01-19-11.14.07.000000"}}]' ;
   var actRecs = JSON.stringify( rcData ) ;
   if( expRecs !== actRecs )
   {
      throw buildException("findAndCheckResult", null, "[find]", 
                          "[expRecs:"+ expRecs +"]\n",
                          "[actRecs:"+ actRecs +"]");
   }
   
   var cnt = cl.count({b:{$type:1,$et:17}});
   var expCnt = rawData.length;
   var actCnt = Number( cnt );
   if( expCnt !== actCnt )
   {
      throw buildException("findAndCheckResult", null, "[count]", 
                          "[expCnt:"+ expCnt +"]",
                          "[actCnt:"+ actCnt +"]");
   }
   
}

function updateAndCheckResult( cl, rawData )
{
   println("\n---Begin to update and check result.");
   
   //update
   for( i = 1; i < rawData.length; i++ )
   {
      cl.update( {$set:{up: rawData[i]["b"] }}, rawData[i] );
   }
   
   //checkResult
   var rcData = [];
   for( i = 1; i < rawData.length; i++ )
   {
      var cursor = cl.find( rawData[i], {_id:{$include:0}} ).sort({a:1});
      while( tmpRec = cursor.next() )
      {
         //check result
         var dt = JSON.stringify( tmpRec.toObj()["b"] );  //println( dt );
         var up = JSON.stringify( tmpRec.toObj()["up"] );  //println( up +"\n" );
         if( dt !== up )
         {
            throw buildException("updateAndCheckResult", null, "[update]", 
                                "[b:"+ dt +"]",
                                "[up:"+ up +"]");
         }
      }
   }
   
   var cnt = cl.count({up:{$type:1,$et:17}});
   var expCnt = rawData.length - 1;  //except '{ a: 0, b: Timestamp() }'
   var actCnt = Number( cnt );
   if( expCnt !== actCnt )
   {
      throw buildException("findAndCheckResult", null, "[count]", 
                          "[expCnt:"+ expCnt +"]",
                          "[actCnt:"+ actCnt +"]");
   }
   
}

function removeAndCheckResult( cl, rawData )
{
   println("\n---Begin to remove and check result.");
   
   //remove
   for( i = 0; i < rawData.length; i++ )
   {
      cl.remove( rawData[i] );
   }
   
   var cnt = cl.count();
   var expCnt = 0;
   var actCnt = Number( cnt );
   if( expCnt !== actCnt )
   {
      throw buildException("removeAndCheckResult", null, "[count]", 
                          "[expCnt:"+ expCnt +"]",
                          "[actCnt:"+ actCnt +"]");
   }
   
}