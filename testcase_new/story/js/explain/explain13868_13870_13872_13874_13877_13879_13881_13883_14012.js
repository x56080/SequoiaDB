/************************************
*@Description: seqDB-13868:Ö÷×Ó±íÊ¹ÓÃSearch²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13870:Ö÷×Ó±íÊ¹ÓÃEvaluate²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13872:Ö÷×Ó±íÊ¹ÓÃEstimate²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13874:Ö÷×Ó±íÊ¹ÓÃExpand²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13877:Ö÷×Ó±íÊ¹ÓÃSubCollections²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13879:Ö÷×Ó±íÊ¹ÓÃFilter²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13881:Ö÷×Ó±íÊ¹ÓÃDetail²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-13883:Ö÷×Ó±íÊ¹ÓÃRun²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
seqDB-14012:Ö÷×Ó±íÊ¹ÓÃFlatten²ÎÊýÕ¹Ê¾·ÃÎÊ¼Æ»®
*@author:      zhaoyu
*@createdate:  2019.7.13
*@testlinkCase: seqDB-13867
**************************************/
function main()
{
   if( commIsStandalone( db ) )
   {
      println( "------Deploy is standalone" ); 
      return; 
   }
   
   if( commGetGroupsNum( db )< 2 )
   {
      println( "Deploy is only one group!" ); 
      return; 
   }
   
   var configPath = "./config.txt"; 
   var mainCLName = COMMCLNAME + "_maincl_13868"; 
   var subCLName1 = COMMCLNAME + "_subcl_13868_1"; 
   var subCLName2 = COMMCLNAME + "_subcl_13868_2"; 
   commDropCL( db, COMMCSNAME, mainCLName, true ); 
   commDropCL( db, COMMCSNAME, subCLName1, true ); 
   commDropCL( db, COMMCSNAME, subCLName2, true ); 
   var dbcl = commCreateCLByOption( db, COMMCSNAME, mainCLName, {ShardingType:"range", ShardingKey:{a:1}, IsMainCL:true} ); 
   commCreateCL( db, COMMCSNAME, subCLName1 ); 
   commCreateCL( db, COMMCSNAME, subCLName2 ); 
   dbcl.attachCL( COMMCSNAME + "." + subCLName1, {LowBound:{a:0}, UpBound:{a:10000}} ); 
   dbcl.attachCL( COMMCSNAME + "." + subCLName2, {LowBound:{a:10000}, UpBound:{a:20000}} ); 
   
   var doc = []; 
   for( var i = 0; i < 20000; i++ )
   {
      doc.push( {a:i, b:i, c:i, d:"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" + i} )
   }
   dbcl.insert( doc ); 
   
   var file = new File( configPath ); 
   while( true )
   {
      try
      {
         var explainObj = JSON.parse( file.readLine().split( "\n" )[0] ); 
         var explainCursor = dbcl.find( {a:{$in:[1, 10000]}} ).explain( explainObj ); 
         while( explainCursor.next() ){}; 
         
      }
      catch( e )
      {
         if( e === -9 )
         {
            break; 
         }
         else
         {
            throw e; 
         }
      }
   }
   
   //Ê¹ÓÃSubCollectionsÕ¹Ê¾·ÃÎÊ¼Æ»®
   var explainCursor = dbcl.find( {a:{$in:[1, 10000]}} ).explain( {SubCollections: COMMCSNAME + "." + subCLName1} ); 
   while( explainCursor.next() ){}; 
   
   commDropCL( db, COMMCSNAME, mainCLName, true ); 
   commDropCL( db, COMMCSNAME, subCLName1, true ); 
   commDropCL( db, COMMCSNAME, subCLName2, true ); 
   
}
main(); 
