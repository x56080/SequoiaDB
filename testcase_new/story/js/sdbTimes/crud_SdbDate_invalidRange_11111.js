/************************************************************************
*@Description:  SdbDate, invalid range
*testcases:        seqDB-11111、seqDB-11112、seqDB-11113、seqDB-11114
*@Author:  2017/2/28  huangxiaoni
*          2019/11/20  zhaoyu modify
************************************************************************/
try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
;

function main()
{  
   var clName = COMMCLNAME+"_11111_2" ;
      
   commDropCL( db, COMMCSNAME, clName);
   var cl = commCreateCL( db, COMMCSNAME, clName); 
   
   var rawData = [ "0000-01-00", 
                   "10000-01-01", 
                   "0000-00-31T15:00:00.000Z",
                   "10000-01-01T16:00:00.000Z",
                   "-9223372036854775809",
                   "9223372036854775808" ]
   testSdbDate( rawData );
   checkCount( cl );

   commDropCL( db, COMMCSNAME, clName);
}

function testSdbDate( rawData )
{
   for( i = 0; i< rawData.length; i++ )
   {
      try{
         SdbDate( rawData[i] );
         throw "Expected failure, but the record:" + JSON.stringify(rawData[i]) +" sucess.";
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw new Error("the record: " + JSON.stringify(rawData[i]) + " failed, actual e:" + e);
         }
      }
   }
}
