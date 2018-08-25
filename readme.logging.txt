BASICS
------
PDNS now supports logging all coming queries and answers to a database.
Two things are logged: 
- raw requests/answers with timestamps and response codes
- requests statistics for qdomains with total request count per domain and last request timestamps

The logging can be arranged to the same database as the regular records reside. However, this is not recommended as logging can affect DB performance and therefore PDNS performance.
A better way is setting up a separate MySQL instance preferrable in a separate dedicated server.
PDNS supports logging into a separate DB engine.

LOGGING DB
----------
The logging is done into tables named `querylog` and `hitcount` [refer to the BASICS].
Every [configured period] the tables are archived into tables named `querylog_{TIMESTAMP}` and `hitcount_{TIMESTAMP}` and the new `querylog` and `hitcount` are created.
This is an efficient approach for maintaining the current logging tables at reasonable size to support high logging rates (tested at 14000 requests per second).
The mechanism is implemented purely inside MySQL basing on stored procedures and events and doesn't require setting up any external facilities.

The backside of the approach is that querying the database becomes a complicated thing to do. To make the user's life easier a number of query tools created [refer to QUERYING LOGGING DATABASE]

The typical table set in the database after some period of time is [the mechanism was configured to archive every minute, not every day]:
mysql> show tables;
+---------------------------------+
| Tables_in_pdns                  |
+---------------------------------+
| hitcountlog                     |
| hitcountlog_2018-08-25_07:30:09 |
| hitcountlog_2018-08-25_07:31:09 |
| hitcountlog_2018-08-25_07:32:09 |
| hitcountlog_2018-08-25_07:33:09 |
| hitcountlog_2018-08-25_07:34:09 |
| hitcountlog_2018-08-25_07:35:09 |
| hitcountlog_2018-08-25_07:36:09 |
| hitcountlog_2018-08-25_07:37:09 |
| hitcountlog_2018-08-25_07:38:09 |
| hitcountlog_2018-08-25_07:39:09 |
| hitcountlog_refs                |
| querylog                        |
| querylog_2018-08-25_07:29:48    |
| querylog_2018-08-25_07:30:48    |
| querylog_2018-08-25_07:31:48    |
| querylog_2018-08-25_07:32:48    |
| querylog_2018-08-25_07:33:48    |
| querylog_2018-08-25_07:34:48    |
| querylog_2018-08-25_07:35:48    |
| querylog_2018-08-25_07:36:48    |
| querylog_2018-08-25_07:37:48    |
| querylog_2018-08-25_07:38:48    |
| querylog_2018-08-25_07:39:48    |
| querylog_refs                   |
+---------------------------------+
25 rows in set (0.00 sec)


LOGGING DB PREPARATION
----------------------
The logging DB creation scripts (including table creation, stored procedures and events) is supplied in loggin_db.sql.
The archieving mechanism is configured to do archiving every day. It's not recommended to increase the period as this can affect logging throughput and querying speed.


QUERYING LOGGING DATABASE
-------------------------
Because logging data is split into archive tables [presumably contaning data for 24 hours], querying towards this structure is a non-trivial task.
To make querying easier two stored procedures are introduced:
get_querylog(in _from timestamp, in _to timestamp, in _rcode smallint) - get the record set for the period of time from '_from' to '_to' with response code '_rcode'
Typical invocation is: 
call get_querylog('2018-08-24 15:20:48', '2018-08-25 15:20:48', 0); 
which gets all the records for 24 hours from 24 Aug 2018 15:20:48 with response code 0

count_querylog(in _from timestamp, in _to timestamp, in _rcode smallint) - get the count of records for the period of time from '_from' to '_to' with response code '_rcode'
Typical invocation is: 
call get_querylog('2018-08-24 15:20:48', '2018-08-25 15:20:48', 0); 
which gets number of records for 24 hours from 24 Aug 2018 15:20:48 with response code 0

PDNS LOGGING parameters
-----------------------
To arrange logging PDNS requires a number of parameters to set.
The parameters to be set up are:
--querylog-host [DEFAULT=127.0.0.1] the host where logging DB resides
--querylog-port [DEFAULT=3306] the DB engine port
--querylog-socket [DEFAULT=] the socket logging facilities communicate through 
--querylog-user [DEFAULT=root] username for accessing the logging DB
--querylog-password [DEFAULT=] password for accessing the logging DB
--querylog-dbname [DEFAULT=pdns] logging DB name
--querylog-reconnectperiod [DEFAULT=600] Recoonnect period if logging connection is broken. 

Typical pdns invocation is as follows:
pdns/pdns_server --no-config --daemon=no --launch=gmysql --gmysql-user=pdnsusr --gmysql-password=[password] --gmysql-dbname=pdns --socket-dir=. --distributor-threads=12 --querylog-host=[host] --querylog-user=pdnsusr --querylog-password=[password] --querylog-dbname=pdns --querylog-reconnectperiod=10