/************************************************************************
*@Description:  timestamp, separator is colon
*testcases:        seqDB-11308
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11308" ;
      
      var rawData = [ { a: 0, b: { $timestamp: "1902-01-01-00:00:00.000000" } }, 
                      { a: 1, b: { $timestamp: "1970-01-01-00:00:00.000000" } }, 
                      { a: 2, b: { $timestamp: "2037-12-31-23:59:59.999999" } }, 
                      
                      { a: 3, b: Timestamp( "1902-01-01-00:00:00.000000" ) }, 
                      { a: 4, b: Timestamp( "2037-12-31-23:59:59.999999" ) } ]
      
      var cl = readyCL( clName );
      
      insertRecs( cl, rawData );
      findAndCheckResult( cl, rawData );
   
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
      var cursor = cl.find( rawData[i], {_id:{$include:0}} ).sort({a:1});
      while( tmpRec = cursor.next() )
      {
         rcData.push( tmpRec.toObj() );
      }
   }
   
   //check result
   var expRecs = '[{"a":0,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":1,"b":{"$timestamp":"1970-01-01-00.00.00.000000"}},{"a":2,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"a":3,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":4,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}}]' ;
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

