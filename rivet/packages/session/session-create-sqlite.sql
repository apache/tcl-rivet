CREATE TABLE rivet_session (
    ip_address          varchar(16) default NULL,
    session_start_time  varchar(24) default NULL,
    session_update_time varchar(24) default NULL,
    session_id          varchar(64) NOT NULL default '',
    PRIMARY KEY (session_id)
);
CREATE TABLE rivet_session_cache (
    session_id      varchar(128)    default NULL PRIMARY KEY ON CONFLICT FAIL,
    package_        varchar(64)     default NULL,
    key_            varchar(128)    default NULL,
    data            varchar(255)    default NULL,

--  KEY rivet_session_cache_idx (session_id),
    CONSTRAINT session_cleanup FOREIGN KEY (session_id) REFERENCES rivet_session(session_id) ON DELETE CASCADE
);
CREATE UNIQUE INDEX rvt_sess_cache_idx ON rivet_session_cache ( session_id, key_ );
