/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = SDBConfigOptionsTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
import org.junit.Test;
import org.junit.Assert;
import org.apache.flink.configuration.ConfigOptions;

import com.sequoiadb.flink.config.SDBConfigOptions;

public class SDBConfigOptionsTest {
    @Test
    public void testSDBOptions() {
        Assert.assertEquals(SDBConfigOptions.HOSTS, ConfigOptions.key("hosts").stringType().asList().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.COLLECTION_SPACE, ConfigOptions.key("collectionspace").stringType().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.COLLECTION, ConfigOptions.key("collection").stringType().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.FORMAT, ConfigOptions.key("format").stringType().defaultValue("bson"));
        Assert.assertEquals(SDBConfigOptions.USERNAME, ConfigOptions.key("username").stringType().defaultValue(""));
        Assert.assertEquals(SDBConfigOptions.PASSWORD_TYPE, ConfigOptions.key("passwordtype").stringType().defaultValue("cleartext"));
        Assert.assertEquals(SDBConfigOptions.PASSWORD, ConfigOptions.key("password").stringType().defaultValue(""));
        Assert.assertEquals(SDBConfigOptions.SPLIT_MODE, ConfigOptions.key("splitmode").stringType().defaultValue("auto"));
        Assert.assertEquals(SDBConfigOptions.SPLIT_BLOCK_NUM, ConfigOptions.key("splitblocknum").intType().defaultValue(4));
        Assert.assertEquals(SDBConfigOptions.PREFERRED_INSTANCE, ConfigOptions.key("preferredinstance").stringType().defaultValue("M"));
        Assert.assertEquals(SDBConfigOptions.PREFERRED_INSTANCE_MODE, ConfigOptions.key("preferredinstancemode").stringType().defaultValue("random"));
        Assert.assertEquals(SDBConfigOptions.PREFERRED_INSTANCE_STRICT, ConfigOptions.key("preferredinstancestrict").booleanType().defaultValue(false));
        Assert.assertEquals(SDBConfigOptions.IGNORE_NULL_FIELD, ConfigOptions.key("ignorenullfield").booleanType().defaultValue(false));
        Assert.assertEquals(SDBConfigOptions.BULK_SIZE, ConfigOptions.key("bulksize").intType().defaultValue(500));
        Assert.assertEquals(SDBConfigOptions.PAGE_SIZE, ConfigOptions.key("pagesize").intType().defaultValue(65536));
        Assert.assertEquals(SDBConfigOptions.DOMAIN, ConfigOptions.key("domain").stringType().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.SHARDING_KEY, ConfigOptions.key("shardingkey").stringType().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.SHARDING_TYPE, ConfigOptions.key("shardingtype").stringType().defaultValue("hash"));
        Assert.assertEquals(SDBConfigOptions.REPL_SIZE, ConfigOptions.key("replsize").intType().defaultValue(1));
        Assert.assertEquals(SDBConfigOptions.COMPRESSION_TYPE, ConfigOptions.key("compressiontype").stringType().defaultValue("lzw"));
        Assert.assertEquals(SDBConfigOptions.GROUP, ConfigOptions.key("group").stringType().noDefaultValue());
        Assert.assertEquals(SDBConfigOptions.AUTO_PARTITION, ConfigOptions.key("autopartition").booleanType().defaultValue(true));
        Assert.assertEquals(SDBConfigOptions.SINK_PARALLELISM, ConfigOptions.key("parallelism").intType().defaultValue(1));
    }
}