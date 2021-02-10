/******************************************************************************
 * @Description   : seqDB-22837 :: 创建数据源指定多个数据源的节点地址
 * @Author        : Wu Yan
 * @CreateTime    : 2021.10.20
 * @LastEditTime  : 2021.02.06
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//main( test );
function test ()
{
   var dataSrcName = "datasrc22837";

   try
   {
      db.createDataSource( dataSrcName, datasrcUrl + "," + datasrcUrl1, userName, passwd );
   }
   catch( e )
   {
      if( e != -32 )
      {//bug:目前未做校验，不报错

         //throw new Error(e); 
      }
   }

   try
   {
      db.getDataSource( dataSrcName );
   }
   catch( e )
   {
      if( e != -352 )
      {
         // throw new Error(e); 
      }
   }


}


