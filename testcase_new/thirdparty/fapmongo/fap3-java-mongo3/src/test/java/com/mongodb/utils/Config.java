package com.mongodb.utils;

import org.springframework.stereotype.Component;

/**
 * @Description 配置类
 * @author fanyu
 * @version 1.00
 * @Date 2020/4/22
 */
@Component
public class Config {
    private String javaMongoVersion;
    private String springMongoVersion;
    private String multiUrl;
    private String springDBName;
    private String javaDBName;
    private String username;
    private String password;

    public Config( String javaMongoVersion, String springMongoVersion,
            String multiUrl, String javaDBName, String springDBName,
            String username, String password ) {
        this.javaMongoVersion = javaMongoVersion;
        this.springMongoVersion = springMongoVersion;
        this.multiUrl = multiUrl;
        this.javaDBName = javaDBName;
        this.springDBName = springDBName;
        this.username = username;
        this.password = password;
    }

    public void setDbname2( String dbname2 ) {
        this.javaDBName = dbname2;
    }

    public String getJavaMongoVersion() {
        return javaMongoVersion;
    }

    public void setJavaMongoVersion( String javaMongoVersion ) {
        this.javaMongoVersion = javaMongoVersion;
    }

    public String getSpringMongoVersion() {
        return springMongoVersion;
    }

    public void setSpringMongoVersion( String springMongoVersion ) {
        this.springMongoVersion = springMongoVersion;
    }

    public String getMultiUrl() {
        return multiUrl;
    }

    public void setMultiUrl( String multiUrl ) {
        this.multiUrl = multiUrl;
    }

    public String getJavaDBName() {
        return javaDBName;
    }

    public void setJavaDBName( String javaDBName ) {
        this.javaDBName = javaDBName;
    }

    public String getSpringDBName() {
        return springDBName;
    }

    public void setSpringDBName( String springDBName ) {
        this.springDBName = springDBName;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername( String username ) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword( String password ) {
        this.password = password;
    }

    public String getHost() {
        return getMultiUrl().split( "," )[ 0 ].split( ":" )[ 0 ];
    }

    public Integer getPort() {
        return Integer
                .parseInt( getMultiUrl().split( "," )[ 0 ].split( ":" )[ 1 ] );
    }

    public String[] getUrls() {
        return getMultiUrl().split( "," );
    }

    @Override
    public String toString() {
        return "Config{" + "javaMongoVersion='" + javaMongoVersion + '\''
                + ", springMongoVersion='" + springMongoVersion + '\''
                + ", javaDBName='" + javaDBName + '\'' + ", springDBName='"
                + springDBName + '\'' + ", multiUrl='" + multiUrl + '\''
                + ", username='" + username + '\'' + ", password='" + password
                + '\'' + '}';
    }
}
