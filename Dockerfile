# start from base
FROM ubuntu:18.04
MAINTAINER Andrey Filimonov <andrey.v.filimonov@gmail.com>

RUN apt update && apt install -y libboost-system1.62.0 libboost-program-options1.62.0 libboost-chrono1.62.0 libboost-thread1.62.0 libmariadbclient18 openssl

# copy the application
COPY ./pdns/pdns_server /opt/pdns/pdns_server
WORKDIR /opt/pdns

# expose port
EXPOSE 53

# start app
CMD /opt/pdns/pdns_server --no-config --daemon=no --launch=gmysql --gmysql-host=${DB_HOST} --gmysql-user=${DB_USER} --gmysql-password=${DB_PWD} --gmysql-dbname=${DB_NAME} --distributor-threads=$(nproc) --querylog-host=${LOG_DB_HOST} --querylog-user=${LOG_DB_USER} --querylog-password=${LOG_DB_PWD} --querylog-dbname=${LOG_DB_NAME}
