/************************************************************************
*@Description:  Timestamp, invalid range
*testcases:        seqDB-11118、seqDB-11119、seqDB-11120、seqDB-11121
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11118_2" ;
      
      var cl = readyCL( clName );
      
      var rawData = [ "1901-12-13-00.00.00.000000",  //0
                      "2038-01-20-00.00.00.000000", 
                      "1901-12-13T00:00:00.000Z",
                      "2038-01-19T15:00:00.000Z",
                      "1901-12-14T00:00:00.000+0800",
                      "2038-01-19T15:00:00.000+0800",  //5
                      -2147483649,
                       2147483648 ]
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
            if( i < rawData.length - 2 )
            {
               Timestamp( rawData[i] );
            }
            else
            {
               Timestamp( rawData[i], 0 );
            }
            throw "Expected failure, but actual success."
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw buildException("generateData", e, "[i:"+ i +"]", 
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