-- 
--  Session management database creation for Oracle
-- 
--  Arnulf 
--
CREATE TABLE rivet_session
    (ip_address                    VARCHAR2(23) DEFAULT NULL,
    session_start_time             DATE DEFAULT NULL,
    session_update_time            DATE DEFAULT NULL,
    session_id                     VARCHAR2(64) NOT NULL
    )
/

ALTER TABLE rivet_session ADD PRIMARY KEY (session_id)

/

CREATE TABLE rivet_session_cache
    (session_id                    VARCHAR2(128) DEFAULT NULL,
    package_                       VARCHAR2(128) DEFAULT NULL,
    key_                           VARCHAR2(256) DEFAULT NULL,
    data                           VARCHAR2(4096) DEFAULT NULL
  )
/

CREATE UNIQUE INDEX riv_sess_cache_ix ON rivet_session_cache
  (
    session_id,
    package_,
    key_
  )
/

CREATE INDEX rivet_session_cache_idx ON rivet_session_cache
  (
    session_id
  )
