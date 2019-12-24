package com.sequoias3.testcommon;

public interface StorageInterface {
    void envPrePare( String url );

    void envRestore( String url );

    String getUrls( String url );

    String getClusterInfo( String url );

}
