CREATE TABLE `querylog` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `qdomain` varchar(255) DEFAULT NULL,
  `source_ip` varchar(255) DEFAULT NULL,
  `rcode` smallint(6) NOT NULL,
  `answer` blob,
  PRIMARY KEY (`id`),
  KEY `qdomain` (`qdomain`),
  KEY `timestamp` (`timestamp`)
) ENGINE=InnoDB AUTO_INCREMENT=46116 DEFAULT CHARSET=utf8

CREATE TABLE `hitcountlog` (
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `qdomain` varchar(255) NOT NULL,
  `hitcount` bigint(20) NOT NULL DEFAULT '0',
  PRIMARY KEY (`qdomain`),
  KEY `timestamp` (`timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8