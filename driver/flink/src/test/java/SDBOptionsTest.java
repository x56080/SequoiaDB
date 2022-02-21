import org.junit.Test;
import org.junit.Assert;
import org.apache.flink.configuration.ConfigOptions;

import com.sequoiadb.flink.config.SDBOptions;

public class SDBOptionsTest {
    @Test
    public void testSDBOptions() {
        Assert.assertEquals(SDBOptions.HOSTS, ConfigOptions.key("hosts").stringType().asList().noDefaultValue());
        Assert.assertEquals(SDBOptions.COLLECTION_SPACE, ConfigOptions.key("collectionspace").stringType().noDefaultValue());
        Assert.assertEquals(SDBOptions.COLLECTION, ConfigOptions.key("collection").stringType().noDefaultValue());
        Assert.assertEquals(SDBOptions.FORMAT, ConfigOptions.key("format").stringType().defaultValue("bson")); 
        Assert.assertEquals(SDBOptions.USERNAME, ConfigOptions.key("username").stringType().defaultValue(""));
        Assert.assertEquals(SDBOptions.PASSWORD_TYPE, ConfigOptions.key("passwordtype").stringType().defaultValue("cleartext"));
        Assert.assertEquals(SDBOptions.PASSWORD, ConfigOptions.key("password").stringType().defaultValue(""));
        Assert.assertEquals(SDBOptions.SPLIT_MODE, ConfigOptions.key("splitmode").stringType().defaultValue("auto"));
        Assert.assertEquals(SDBOptions.SPLIT_BLOCK_NUM, ConfigOptions.key("splitblocknum").intType().defaultValue(4));
        Assert.assertEquals(SDBOptions.PREFERRED_INSTANCE, ConfigOptions.key("preferredinstance").stringType().defaultValue("M"));
        Assert.assertEquals(SDBOptions.PREFERRED_INSTANCE_MODE, ConfigOptions.key("preferredinstancemode").stringType().defaultValue("random"));
        Assert.assertEquals(SDBOptions.PREFERRED_INSTANCE_STRICT, ConfigOptions.key("preferredinstancestrict").booleanType().defaultValue(false));
        Assert.assertEquals(SDBOptions.IGNORE_NULL_FIELD, ConfigOptions.key("ignorenullfield").booleanType().defaultValue(false));
        Assert.assertEquals(SDBOptions.BULK_SIZE, ConfigOptions.key("bulksize").intType().defaultValue(500));
        Assert.assertEquals(SDBOptions.PAGE_SIZE, ConfigOptions.key("pagesize").intType().defaultValue(65536));
        Assert.assertEquals(SDBOptions.DOMAIN, ConfigOptions.key("domain").stringType().noDefaultValue());
        Assert.assertEquals(SDBOptions.SHARDING_KEY, ConfigOptions.key("shardingkey").stringType().noDefaultValue());
        Assert.assertEquals(SDBOptions.SHARDING_TYPE, ConfigOptions.key("shardingtype").stringType().defaultValue("hash"));
        Assert.assertEquals(SDBOptions.REPL_SIZE, ConfigOptions.key("replsize").intType().defaultValue(1));
        Assert.assertEquals(SDBOptions.COMPRESSION_TYPE, ConfigOptions.key("compressiontype").stringType().defaultValue("lzw"));
        Assert.assertEquals(SDBOptions.GROUP, ConfigOptions.key("group").stringType().noDefaultValue());
        Assert.assertEquals(SDBOptions.AUTO_SPLIT, ConfigOptions.key("autosplit").booleanType().defaultValue(false));
        Assert.assertEquals(SDBOptions.SINK_PARALLELISM, ConfigOptions.key("parallelism").intType().defaultValue(1));
    }
}