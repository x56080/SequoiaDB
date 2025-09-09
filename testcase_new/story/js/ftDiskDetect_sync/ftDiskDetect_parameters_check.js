/******************************************************************************
 * @Description   : ftmask,detectdisk,ftdiskslowthreshold,ftdiskslowincrement 参数测试
 * @Author        : fangjiabin
 * @CreateTime    : 2025.08.29
 * @LastEditTime  : 2025.08.29
 * @LastEditors   : fangjiabin
 ******************************************************************************/
testConf.skipStandAlone = true;

var ftmaskDef = "NOSPC|DEADSYNC|DISKFAULT" ;
var detectdiskDef = 5 ;
var ftdiskslowthresholdDef = 1000 ;
var ftdiskslowincrementDef = 500 ;

main( test );

function test ()
{
   db.deleteConf( {"ftmask":'',"detectdisk":'',"ftdiskslowthreshold":'',"ftdiskslowincrement":''} );

   try
   {
      // ftmask
      checkParameter( "ftmask", ftmaskDef );
      checkUpdateParameter( "ftmask", "NOSPC|DEADSYNC|DISKFAULT|ABC", ftmaskDef );
      checkUpdateParameter( "ftmask", "DISKFAULT|ABC", ftmaskDef );
      checkUpdateParameter( "ftmask", "DISKFAULT", "DISKFAULT" );
      checkUpdateParameter( "ftmask", 123, ftmaskDef );

      // detectdisk
      checkParameter( "detectdisk", detectdiskDef );
      checkUpdateParameter( "detectdisk", 0, "5" );
      checkUpdateParameter( "detectdisk", 10, "10" );
      checkUpdateParameter( "detectdisk", 300, "300" );
      checkUpdateParameter( "detectdisk", 301, "300" );
      checkUpdateParameter( "detectdisk", -1, "300" );
      checkUpdateParameter( "detectdisk", "0", "5" );
      checkUpdateParameter( "detectdisk", "10", "10" );
      checkUpdateParameter( "detectdisk", "300", "300" );
      checkUpdateParameter( "detectdisk", "301", "300" );
      checkUpdateParameter( "detectdisk", "-1", "300" );

      checkUpdateParameter( "detectdisk", "345abc", "5" );

      // ftdiskslowthreshold
      checkParameter( "ftdiskslowthreshold", ftdiskslowthresholdDef );
      checkUpdateParameter( "ftdiskslowthreshold", 0, 1000 );
      checkUpdateParameter( "ftdiskslowthreshold", 1001, 1001 );
      checkUpdateParameter( "ftdiskslowthreshold", 600000, 600000 );
      checkUpdateParameter( "ftdiskslowthreshold", 600001, 600000 );
      checkUpdateParameter( "ftdiskslowthreshold", -1, 1000, -6 );
      checkUpdateParameter( "ftdiskslowthreshold", "abc", 1000, -6 );

      // ftdiskslowincrement
      checkParameter( "ftdiskslowincrement", ftdiskslowincrementDef );
      checkUpdateParameter( "ftdiskslowincrement", 0, 0 );
      checkUpdateParameter( "ftdiskslowincrement", 200, 200 );
      checkUpdateParameter( "ftdiskslowincrement", 600000, 600000 );
      checkUpdateParameter( "ftdiskslowincrement", 600001, 600000 );
      checkUpdateParameter( "ftdiskslowincrement", -1, 500, -6 );
      checkUpdateParameter( "ftdiskslowincrement", "abc", 500, -6 );
   }
   finally
   {
      db.deleteConf( {"ftmask":'',"detectdisk":'',"ftdiskslowthreshold":'',"ftdiskslowincrement":''} );
   }
}