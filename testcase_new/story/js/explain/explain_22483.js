/******************************************************************************
*@Description : seqDB-22483:查询cond条件带$exists/$isnull，检查访问计划中扫描方式 
*@author      : Zhao Xiaoni
*@Date        : 2020.7.27
******************************************************************************/
main();

function main()
{
   var clName = CHANGEDPREFIX + "_22483";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
 
   cl.createIndex( "index_22483", { "a": 1 }, false );

   checkScanType( cl, { "a": { "$isnull": 0 } }, "tbscan" ); 
    
   checkScanType( cl, { "a": { "$isnull": 1 } }, "ixscan" );
   
   checkScanType( cl, { "a": { "$exists": 0 } }, "ixscan" );

   checkScanType( cl, { "a": { "$exists": 1 } }, "tbscan" );

   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkScanType( cl, cond, expResult )
{
   var scanType = cl.find( cond ).explain().current().toObj().ScanType;
   if( !commCompareObject( expResult, scanType ) )
   {
     throw new Error( "expResult: " + expResult + ", scanType: " + scanType ); 
   }   
}
