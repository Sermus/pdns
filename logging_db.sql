CREATE TABLE `hitcountlog` (
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `qdomain` varchar(255) NOT NULL,
  `hitcount` bigint(20) NOT NULL DEFAULT '0',
  PRIMARY KEY (`qdomain`),
  KEY `timestamp` (`timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8

CREATE TABLE `hitcountlog_refs` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `name` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=2996 DEFAULT CHARSET=latin1

CREATE TABLE `querylog` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `qdomain` varchar(255) DEFAULT NULL,
  `source_ip` varchar(255) DEFAULT NULL,
  `rcode` smallint(6) NOT NULL,
  `answer` blob,
  PRIMARY KEY (`id`),
  KEY `qdomain` (`qdomain`),
  KEY `timestamp` (`timestamp`),
  KEY `rcode` (`rcode`)
) ENGINE=InnoDB AUTO_INCREMENT=129001 DEFAULT CHARSET=utf8

CREATE TABLE `querylog_refs` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `name` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=3053 DEFAULT CHARSET=latin1

SET GLOBAL event_scheduler = ON;

DELIMITER //
CREATE PROCEDURE archive_querylog()
 BEGIN
 insert into querylog_refs (name) values(CONCAT('querylog_', REPLACE(CURRENT_TIMESTAMP, ' ','_')));
 drop table if exists `querylog_temp`;
 create table querylog_temp like querylog;
 select concat('rename table querylog to `', name, '`, querylog_temp to querylog;') from querylog_refs order by id desc limit 1 into @statement;
 PREPARE queryexec FROM @statement;
 EXECUTE queryexec;
 END //
DELIMITER ;

create event archive_querylog on schedule every 1 day starts CURRENT_TIMESTAMP do call archive_querylog();

DELIMITER //
CREATE PROCEDURE drop_querylog_archives(in ts timestamp)
BEGIN
  SET SESSION group_concat_max_len = 1000000;
  IF EXISTS (select * from querylog_refs where timestamp < ts) THEN
    select concat('drop table ', GROUP_CONCAT(concat('`',name,'`')) , ';') from querylog_refs where timestamp < ts INTO @statement;
    PREPARE queryexec FROM @statement;
    EXECUTE queryexec;
    delete from querylog_refs where timestamp < ts;
    END IF;
END //
DELIMITER ;

create event drop_querylog_archives on schedule every 1 day starts CURRENT_TIMESTAMP do call drop_querylog_archives(DATE_SUB(NOW(), INTERVAL 1 DAY));




DELIMITER //
CREATE PROCEDURE archive_hitcountlog()
 BEGIN
 insert into hitcountlog_refs (name) values(CONCAT('hitcountlog_', REPLACE(CURRENT_TIMESTAMP, ' ','_')));
 drop table if exists `hitcountlog_temp`;
 create table hitcountlog_temp like hitcountlog;
 select concat('rename table hitcountlog to `', name, '`, hitcountlog_temp to hitcountlog;') from hitcountlog_refs order by id desc limit 1 into @statement;
 PREPARE queryexec FROM @statement;
 EXECUTE queryexec;
 END //
DELIMITER ;

create event archive_hitcountlog on schedule every 1 day starts CURRENT_TIMESTAMP do call archive_hitcountlog();

DELIMITER //
CREATE PROCEDURE drop_hitcountlog_archives(in ts timestamp)
BEGIN
  SET SESSION group_concat_max_len = 1000000;
  IF EXISTS (select * from hitcountlog_refs where timestamp < ts) THEN
    select concat('drop table ', GROUP_CONCAT(concat('`',name,'`')) , ';') from hitcountlog_refs where timestamp < ts INTO @statement;
    PREPARE queryexec FROM @statement;
    EXECUTE queryexec;
    delete from hitcountlog_refs where timestamp < ts;
    END IF;
END //
DELIMITER ;

create event drop_hitcountlog_archives on schedule every 1 day starts CURRENT_TIMESTAMP do call drop_hitcountlog_archives(DATE_SUB(NOW(), INTERVAL 1 DAY));


DELIMITER //
CREATE PROCEDURE get_querylog(in _from timestamp, in _to timestamp, in _rcode smallint)
BEGIN
  DECLARE done INT DEFAULT 0;
  DECLARE table_name VARCHAR(255);
  DECLARE cur CURSOR FOR
    select name from querylog_refs where timestamp >= DATE_SUB(_from, INTERVAL 1 DAY) and timestamp <= DATE_ADD(_to, INTERVAL 1 DAY);
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = 1;
    OPEN cur;

    read_loop: LOOP
    FETCH cur INTO table_name;
    IF done THEN
        LEAVE read_loop;
    END IF;
    select concat('select * from `', table_name, '` where timestamp >=''', _from, ''' and timestamp <= ''', _to, ''' and rcode=', _rcode,';') into @statement;
    PREPARE queryexec FROM @statement;
    EXECUTE queryexec;
    END LOOP;
    CLOSE cur;
END //
DELIMITER ;

DELIMITER //
CREATE PROCEDURE count_querylog(in _from timestamp, in _to timestamp, in _rcode smallint)
BEGIN
  DECLARE done INT DEFAULT 0;
  DECLARE table_name VARCHAR(255);
  DECLARE total_num BIGINT DEFAULT 0;
  DECLARE num BIGINT DEFAULT 0;
  DECLARE cur CURSOR FOR
    select name from querylog_refs where timestamp >= DATE_SUB(_from, INTERVAL 1 DAY) and timestamp <= DATE_ADD(_to, INTERVAL 1 DAY);
  DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = 1;
    OPEN cur;

    read_loop: LOOP
    FETCH cur INTO table_name;
    IF done THEN
        LEAVE read_loop;
    END IF;
    select concat('select count(*) from `', table_name, '` where timestamp >=''', _from, ''' and timestamp <= ''', _to, ''' and rcode=', _rcode,' into @num;') into @statement;
    PREPARE queryexec FROM @statement;
    EXECUTE queryexec;
    SET total_num = total_num + @num;
    END LOOP;
    CLOSE cur;
  select total_num;
END //
DELIMITER ;
