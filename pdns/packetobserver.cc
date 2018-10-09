#include "packetobserver.hh"
#include "arguments.hh"
#include "logger.hh"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/duration.hpp>

PacketObserver po;

using namespace std;
using namespace boost::chrono;

PacketObserver::PacketObserver()
{
    PARAM_PREFIX = "querylog";
    declare("host", "Logging database host", "127.0.0.1");
    declare("port", "Logging database instance port", "3306");
    declare("socket", "Logging database socket", "");
    declare("user", "Logging database name", "root");
    declare("password", "Logging database name", "");
    declare("dbname", "Logging database name", "pdnslogging");
    declare("reconnectperiod", "Period between two reconnection attempts to logging database if the connection fails in seconds", "600");
}

PacketObserver::~PacketObserver()
{
}

void PacketObserver::init()
{
    observe_queue = new boost::sync_bounded_queue<DNSPacket>(10000);
    finish_threads = false;
    hitmap_flush_thread = new boost::thread(&PacketObserver::save_observe_data, this);
    observe_flush_thread = new boost::thread(&PacketObserver::save_hitmap_data, this);
}

void PacketObserver::deinit()
{
    finish_threads = true;
    hitmap_flush_thread->join();
    observe_flush_thread->join();
    delete hitmap_flush_thread;
    delete observe_flush_thread;
    delete observe_queue;
}

void PacketObserver::declare(const string &param, const string &help, const string &value)
{
  string fullname=PARAM_PREFIX+"-"+param;
  arg().set(fullname,help)=value;
}

const string& PacketObserver::getArg(const string &key)
{
  return arg()[PARAM_PREFIX+"-"+key];
}

int PacketObserver::getArgAsNum(const string &key)
{
  return arg().asNum(PARAM_PREFIX+"-"+key);
}

void PacketObserver::observe(DNSPacket &p)
{
    if (p.qtype.getName() != "NAPTR")
        return;

    observe_queue->wait_push_back(p); 
    {
        boost::lock_guard<boost::recursive_mutex> lock(guard);
        if (hitmap == nullptr)
            hitmap = new std::map<DNSName, uint32_t>();
        (*hitmap)[p.qdomain]++;
    }
}

SMySQL *PacketObserver::get_sql()
{
    SMySQL * sql =  new SMySQL(getArg("dbname"),
                getArg("host"),
                getArgAsNum("port"),
                getArg("socket"),
                getArg("user"),
                getArg("password"));
                sql->setLog(::arg().mustDo("query-logging"));
    return sql;
}

string PacketObserver::serialize_packet(DNSPacket &p)
{
    ostringstream ss;

    ss << "(FROM_UNIXTIME(" << p.d_dt.time() << "),";
    ss << "" << p.d_dt.udiff() << ",";
    ss << "'" << p.qdomain.toString() << "',";
    ss << "'" << p.getRemote().toString() << "',";
    ss << p.d.rcode << ",";

    string replyString = p.getString();
    replyString.erase(std::remove_if(replyString.begin(), replyString.end(), [](char c){return !isprint(c) || c=='(' || c==')' || c=='\"'|| c=='\'';}), replyString.end());
    ss  << "'" << replyString << "')";

    return ss.str();
}

void PacketObserver::save_observe_data()
{
    SMySQL * sql = nullptr; 
    system_clock::time_point last_connection_attempt_time = system_clock::from_time_t(0);
    while(!finish_threads)
    {
        DNSPacket p = observe_queue->pull_front();
        uint32_t batch_counter = 1;

        try
        {
            if (!sql)
            {
                if (boost::chrono::duration_cast<seconds>(system_clock::now() - last_connection_attempt_time).count() < getArgAsNum("reconnectperiod"))
                {
                    continue;
                }
                last_connection_attempt_time = system_clock::now();
                sql = get_sql();
                g_log<<Logger::Info<< << "Connection to logging database is established" << endl;
            }

            ostringstream ss;
            ss << "insert into querylog (timestamp,response_time,qdomain,source_ip,rcode,answer) values";
            ss << serialize_packet(p) << ',';

            while (batch_counter++ < BATCH_SIZE)
            {
                p = observe_queue->pull_front();
                ss << serialize_packet(p) << ',';
            }

            string query = ss.str();
            query.pop_back();
            sql->execute(query);
        }
        catch (SSqlException)
        {
            g_log<<Logger::Error << "Connection to logging database is broken for querylog, restore in " << std::max(0ll, static_cast<long long int>(getArgAsNum("reconnectperiod") -  boost::chrono::duration_cast<seconds>(system_clock::now() - last_connection_attempt_time).count())) << " seconds"  << endl;
            if (sql) delete sql;
            sql = nullptr;
        }
    }
}

void PacketObserver::save_hitmap_data()
{
    SMySQL * sql = nullptr; 
    system_clock::time_point last_connection_attempt_time = system_clock::from_time_t(0);
    while(!finish_threads)
    {
        boost::this_thread::sleep_for(boost::chrono::seconds{5});
        std::map<DNSName, uint32_t> * map;
        {
            boost::lock_guard<boost::recursive_mutex> lock(guard);
            map = hitmap;
            hitmap = nullptr;
            if (!map)
                continue;
        }
        try
        {
            if (!sql)
            {
                if (boost::chrono::duration_cast<seconds>(system_clock::now() - last_connection_attempt_time).count() < getArgAsNum("reconnectperiod"))
                {
                    delete map;
                    continue;
                }
                sql = get_sql();
                last_connection_attempt_time = system_clock::now();
                g_log<<Logger::Info<< << "Connection to logging database is established" << endl;
            }

            sql->startTransaction();
            for (auto const &x : *map)
            {
                ostringstream ss;
                ss << "insert into hitcountlog (qdomain,hitcount) values(";
                ss << "'" << x.first.toString() << "'," << x.second << ")";
                ss << " ON DUPLICATE KEY UPDATE hitcount = hitcount + VALUES(hitcount)";
                string query = ss.str();
                sql->execute(query);
            }
            sql->commit();
            delete map;
        }
        catch (SSqlException)
        {
            g_log<<Logger::Error << "Connection to logging database is broken for hitcount, restore in " << std::max(0ll, static_cast<long long int>(getArgAsNum("reconnectperiod") -  boost::chrono::duration_cast<seconds>(system_clock::now() - last_connection_attempt_time).count())) << " seconds"  << endl;
            delete map;
            if (sql) delete sql;
            sql = nullptr;
        }    
    }
}