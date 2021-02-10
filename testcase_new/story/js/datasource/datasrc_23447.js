/* *****************************************************************************
@description: seqDB-23477:多个子表使用不同数据源，主表上执行LOB操作
@author: 2021-1-8 wuyan  Init
***************************************************************************** */
main( test );

function test( )
{
   var dataSrcName1 = "datasrc23477_a";
   var dataSrcName2 = "datasrc23477_b";
   var dataSrcName3 = "datasrc23477_c";
   var srcCSName = "cs_23477new";
   var csName = "cs_23477new";
   var clName = "cl_23477new";   
   var mainCLName = "mainCL_23477new";
   var subCLName1 = "subCL_23477_a"; 
   var subCLName2 = "subCL_23477_b";  
   var subCLName3 = "subCL_23477_c";     
   var datasrcDB1 = new Sdb(datasrcIp,datasrcPort,userName,passwd);
   var datasrcDB2 = new Sdb(other_datasrcIp1,datasrcPort,userName,passwd);
   var datasrcDB3 = new Sdb(other_datasrcIp2,datasrcPort,userName,passwd);
   commDropCS ( datasrcDB1, srcCSName );
   commDropCS ( datasrcDB2, srcCSName );
   commDropCS ( datasrcDB3, srcCSName );
   clearDataSource(csName, dataSrcName1);
   clearDataSource(csName, dataSrcName2);
   clearDataSource(csName, dataSrcName3);
   commCreateCS ( datasrcDB1, srcCSName);
   commCreateCL( datasrcDB1, srcCSName, clName);   
   commCreateCS ( datasrcDB2, srcCSName);
   commCreateCL( datasrcDB2, srcCSName, clName);   
   commCreateCS ( datasrcDB3, srcCSName);
   commCreateCL( datasrcDB3, srcCSName, clName);   
   db.createDataSource(dataSrcName1, datasrcUrl,userName,passwd);
   db.createDataSource(dataSrcName2, otherDSUrl1,userName,passwd);
   db.createDataSource(dataSrcName3, otherDSUrl2,userName,passwd);   
   var dsMarjorVersion1 = getDSMajorVersion( dataSrcName1 ); 
   var dsMarjorVersion2 = getDSMajorVersion( dataSrcName2 ); 
   var dsMarjorVersion3 = getDSMajorVersion( dataSrcName3 );    
   
   var cs = db.createCS(csName); 
   cs.createCL(clName,{DataSource:dataSrcName1}); 
   cs.createCL(subCLName1,{DataSource:dataSrcName2,Mapping: srcCSName + "." + clName}); 
   cs.createCL(subCLName2,{DataSource:dataSrcName3,Mapping: srcCSName + "." + clName});    
   cs.createCL(subCLName3);
   var maincl = createCLAndAttachCL(cs, csName, mainCLName, subCLName1,subCLName2,subCLName3,clName);
   putLobAndCheckResult( maincl, dsMarjorVersion1);           
   
   db.dropCS(csName);   
   datasrcDB1.dropCS(srcCSName);
   datasrcDB2.dropCS(srcCSName);
   datasrcDB3.dropCS(srcCSName);
   db.dropDataSource(dataSrcName1); 
   db.dropDataSource(dataSrcName2); 
   db.dropDataSource(dataSrcName3);    
   datasrcDB1.close(); 
   datasrcDB2.close(); 
   datasrcDB3.close();     
}

function createCLAndAttachCL(cs, csName, mainCLName, subCLName1,subCLName2,subCLName3,subCLName4)
{   
   //创建LobShardingKeyFormat为YYYYMMDD主表  
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { date: 1 }, "LobShardingKeyFormat": "YYYYMMDD", ShardingType: "range", IsMainCL: true } );
   var scope = 10;  
   var beginBound = new Date().getFullYear() * 10000 + 101;  

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "date": ( parseInt( beginBound ) ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + scope ) + '' } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "date": ( parseInt( beginBound ) + scope ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + 2 * scope ) + '' } } ); 
   mainCL.attachCL( csName + "." + subCLName3, { LowBound: { "date": ( parseInt( beginBound ) + 2*scope ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + 3 * scope ) + '' } } ); 
   println("-w---")
   return mainCL;
}

function putLobAndCheckResult( dbcl,dsMarjorVersion )
{   
   var filePath = WORKDIR + "/lob23477/";
   var fileName = "filelob_23477";
   var fileSize = 1024 * 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
    
   //putLobs
   var nameArr = dbcl.toString().split( "." );
   var mainCLFullName = nameArr[1] + "." + nameArr[2];
   var lobOids = insertLob( dbcl, filePath + fileName, "YYYYMMDD" );
   //get lobs and check md5
   checkLobMD5( dbcl, lobOids, fileMD5 );  
   //list lobs
   var lobNum = 20;
   listLobs(dbcl,fileSize, lobNum);
   
   //truncate lob
   var size = 1024 * 20;  
   var lobID = lobOids[0];   
   //只有3.0以上版本才支持truncateLob操作，2.8版本不支持报错-315（SDB_OPERATION_INCOMPATIBLE）
   try
   {
      dbcl.truncateLob( lobID, size );  
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      dbcl.getLob( lobID, filePath + "checktruncateLob23447" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob23447"  );   
      assert.equal( expMD5, actMD5  ); 
   }
   catch(e)
   {
      if(e != SDB_OPERATION_INCOMPATIBLE)
      {
         throw new Error(e); 
      }
   } 
   
  
   //deleteLob
   deleteLob ( dbcl, lobOids );   
   
   deleteTmpFile( filePath );
}

function listLobs(dbcl,fileSize, lobNum)
{
   //list Lob
   var rc = dbcl.listLobs( );
   var listlobNum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];      
      assert.equal( fileSize, listSize);
      listlobNum++
   }
   rc.close();
   assert.equal( lobNum, listlobNum);
}