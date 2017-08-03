/************************************************************************
*@Description:  SdbDate, invalid range
*testcases:        seqDB-11111、seqDB-11112、seqDB-11113、seqDB-11114
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11111_2" ;
      
      var cl = readyCL( clName );
      
      var rawData = [ "0000-01-00", 
                      "10000-01-01", 
                      "0000-00-31T15:00:00.000Z",
                      "10000-01-01T16:00:00.000Z",
                      "-9223372036854775809",
                      "9223372036854775808" ]
      testSdbDate( rawData );
      checkResult( cl, rawData );
   
      cleanCL( clName );
   }
      catch(e)
   {
   	throw e;
   }
}

function testSdbDate( rawData )
{
   println("\n---Begin to test SdbDate.");
   
   for( i = 0; i< rawData.length; i++ )
   {
      try{
            SdbDate( rawData[i] );
            throw "Expected failure, but actual success."
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw buildException("generateData", e, "i:"+i, 
                                 "[e:-6]", 
                                 "[e:"+ e +"]");
         }
      }
   }
}

function checkResult( cl, rawData )
{
   println("\n---Begin to check result.");
   
   var cnt = cl.count();
   var expCnt = 0;
   var actCnt = Number( cnt );
   if( expCnt !== actCnt )
   {
      throw buildException("checkResult", null, "[count]", 
                          "[expCnt:"+ expCnt +"]",
                          "[actCnt:"+ actCnt +"]");
   }
   
}