/************************************************************************
*@Description:  timestamp, valid range
*testcases:        seqDB-11115、seqDB-11116、seqDB-11117
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11115_1" ;
      
      var rawData = [ { a: 0, b: { $timestamp: "1902-01-01-00.00.00.000000" } }, 
                      { a: 1, b: { $timestamp: "1970-01-01-00.00.00.000000" } }, 
                      { a: 2, b: { $timestamp: "2037-12-31-23.59.59.999999" } }, 
                       
                      { a: 3, b: { $timestamp: "1901-12-31T15:54:03.000Z" } },  //"1902-01-01T00:00:00.000Z"
                      { a: 4, b: { $timestamp: "2037-12-31T15:59:59.999Z" } },  //"2037-12-31-23:59:59.999999"
                      
                      { a: 5, b: { $timestamp: "1902-01-01T00:00:00.000+0800" } }, 
                      { a: 6, b: { $timestamp: "2037-12-31T23:59:59.999+0800" } } ]  
      
      var cl = readyCL( clName );
      
      //crud  
      insertRecs( cl, rawData );
      findAndCheckResult( cl, rawData );
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

function findAndCheckResult( cl, rawData )
{
   println("\n---Begin to find and check result.");
   
   //find, exact matching each record
   var rcData = [];
   for( i = 0; i < rawData.length; i++ )
   {
      var cursor = cl.find( {$and:[ rawData[i], {a:{$ne:5}} ]}, {_id:{$include:0}} ).sort({a:1});
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   
   //check result
   var localtime1 = turnLocaltime( '1901-12-31T15:54:03.000Z', '%Y-%m-%d-%H.%M.%S.000000' );
   var expRecs = '[{"a":0,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":1,"b":{"$timestamp":"1970-01-01-00.00.00.000000"}},{"a":2,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"a":3,"b":{"$timestamp":"'+ localtime1 +'"}},{"a":4,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":6,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}}]' ;
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
   for( i = 0; i < rawData.length; i++ )
   {
      cl.update( {$set:{up: rawData[i]["b"] }}, rawData[i] );
   }
   
   //checkResult
   var rcData = [];
   for( i = 0; i < rawData.length; i++ )
   {
      var cursor = cl.find( {}, {_id:{$include:0}} ).sort({a:1});
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
   var expCnt = rawData.length;
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

