--
--  Define SQL tables for session management code
--
--  Author: Arnulf  (minor changes by Massimo Manghi)
--  
--  02 May 2006 
--

DROP TABLE IF EXISTS `rivet_session`;
create table rivet_session (
    ip_address		varchar(16) default NULL,
    session_start_time	datetime    default NULL,
    session_update_time	datetime    default NULL,
    session_id		varchar(64) NOT NULL default '',
    PRIMARY KEY	(session_id)
) ENGINE=INNODB; 

DROP TABLE IF EXISTS `rivet_session_cache`;
create table rivet_session_cache(
    session_id		varchar(128)	default NULL,
    package_		varchar(64)	default NULL,
    key_		varchar(128)	default NULL,
    data                varchar(255)	default NULL,

    UNIQUE KEY riv_sess_cache_ix( session_id, key_ ),
    KEY rivet_session_cache_idx (session_id),
    FOREIGN KEY (session_id) REFERENCES rivet_session(session_id) ON DELETE CASCADE
) ENGINE=INNODB;
-- create index rivet_session_cache_idx ON rivet_session_cache( session_id );

