package com.sequoiadb.lob;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.OperateTask;
import org.bson.types.ObjectId;

import java.util.List;
import java.util.Map;

import static com.sequoiadb.metaopr.commons.MyUtil.getSdb;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public abstract class LobTask extends OperateTask {


    @Override
    public void exec() {
        try (Sequoiadb db = getSdb()) {
            DBCollection cl = db.getCollectionSpace(LobUtil.csName).getCollection(LobUtil.clName);
            lobOperate(cl);
        }
    }


    abstract void lobOperate(DBCollection cl);

    public static LobTask getCreateLobsTask(final int num, final byte[] bytes, final List<ObjectId> createLobIds) {
        return new LobTask() {
            @Override
            void lobOperate(DBCollection cl) {
                if (createLobIds == null)
                    throw new IllegalArgumentException("lobIds can not be null");
                for (int i = 0; i < num; i++) {
                    DBLob lob = cl.createLob();
                    lob.write(bytes);
                    lob.close();
                    createLobIds.add(lob.getID());
                }
            }
        };
    }

    public static LobTask getDeleteLobsTask(final List<ObjectId> lobIds, final Map<ObjectId,String> deletedIdMap) {
        return new LobTask() {
            @Override
            void lobOperate(DBCollection cl) {
                for (int i = 0; i < lobIds.size(); i++) {
                    ObjectId id = lobIds.get(i);
                    cl.removeLob(id);
                    deletedIdMap.put(id,"");
                }
            }
        };
    }

    public static LobTask getReadLobsTask(final List<ObjectId> lobIds) {
        return new LobTask() {
            @Override
            void lobOperate(DBCollection cl) {
                for (ObjectId lobId : lobIds) {
                    MyUtil.readLob(cl.getCSName(),cl.getName(),lobId);
                }
            }
        };
    }
}
