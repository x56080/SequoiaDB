/************************************************************************
*@Description:      1、导入时指定字段分隔符与记录分隔符相等，检查结果；
                    2、导入时指定字符串分隔符与记录分隔符相等，检查结果；
                    3、导入时指定字符串分隔符与字段分隔符相等，检查结果。
                   seqDB-18264
*@Author:   2019-4-18  chensiqin
************************************************************************/

main() ;

function main()
{  
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME+"_18264" ;
      var cl = readyCL( csName, clName );
      
      var imprtFile = tmpFileDir +"18264.csv";
      readyData( imprtFile );
      importDataER( csName, clName, imprtFile , cl);
      importDataAR( csName, clName, imprtFile , cl);
      importDataEA( csName, clName, imprtFile , cl);
      cleanCL( csName, clName );
   }
   catch(e)
   {
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + imprtFile );
   }
}

function readyData( imprtFile )
{
   println("\n---Begin to ready data.");
   
   var file = fileInit( imprtFile );
   file.write( "cYdY1YYexprtTestYY" );
   var fileInfo = cmd.run( "cat "+ imprtFile );
   println( imprtFile +"\n" + fileInfo );
   file.close();
}

function importDataER( csName, clName, imprtFile, cl )
{
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -e 'Y' -r 'Y' --headerline true --fields='c int,d string'"
                     +" --file "+ imprtFile;
   
   testRunCommand(imprtOption);
   checkCLData( cl );
   cl.truncate();
}

function importDataAR( csName, clName, imprtFile, cl )
{
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -a 'Y' -r 'Y' --headerline true --fields='c int,d string'"
                     +" --file "+ imprtFile;
   
   testRunCommand(imprtOption);
   checkCLData( cl );
   cl.truncate();
}

function importDataEA( csName, clName, imprtFile, cl )
{
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -e 'Y' -a 'Y' --headerline true --fields='c int,d string'"
                     +" --file "+ imprtFile;
   
   testRunCommand(imprtOption);
   checkCLData( cl );
   cl.truncate();
}

function testRunCommand(command)
{
   println( command );
   try{
     cmd.run( command );
     throw buildException( "importData", null, "[sdbimprt results]", 
                        "expected thow exception", 
                        "actual success" );
   }
   catch(e)
   {
      if( e !== 127 )
      {
        throw buildException( "importData", null, "[sdbimprt results]", 
                           "expected thow exception", 
                           "actual success" );
      }
   }
   
}

function checkCLData( cl )
{
   println("\n---Begin to check cl data.");
   
   var rc = cl.find();
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   
   var expCnt  = 0;
   var actCnt  = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw buildException( "checkCLdata", null, "[find]", 
                        "[cnt:"+ expCnt +"]", 
                        "[cnt:"+ actCnt +"]" );
   }
   
}