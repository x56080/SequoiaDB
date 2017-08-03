/************************************************************************
*@Description:  date, invalid range
*testcases:        seqDB-11108、seqDB-11109、seqDB-11110
*@Author:  2017/2/28  huangxiaoni
************************************************************************/
main();

function main()
{  
   try
   {
      var clName = COMMCLNAME+"_11108_1" ;
      
      //get local time or millisecond
      var rawData = [ { a: 0, b: { $date: "0000-01-00"  } }, 
                      { a: 1, b: { $date: "10000-01-01" } }, 
                      
                      { a: 2, b: { $date: "0000-01-00T15:00:00.000Z"  } },  
                      { a: 3, b: { $date: "10000-01-01T16:00:00.000Z" } },  
                      
                      { a: 4, b: { $date: "0000-00-31T15:50:00.000+0800"  } }, 
                      { a: 5, b: { $date: "10000-01-01T00:00:00.000+0800" } } ] 
      
      var cl = readyCL( clName );
      
      //crud  
      insertRecs( cl, rawData );
      checkResult( cl, rawData );
   
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
   
   for( i = 0; i < rawData.length; i++ )
   {
      try
      {
         cl.insert( rawData[i] );
         throw "Expected failure, but actual success."
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw buildException("checkResult", e, 'insert( '+ JSON.stringify( rawData[i] ) +" )", 
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