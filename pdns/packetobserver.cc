#include "packetobserver.hh"
#include "modules/gmysqlbackend/smysql.hh"
#include "arguments.hh"
#include <iostream>
#include <sstream>
#include <algorithm>

PacketObserver po;

using namespace std;

PacketObserver::PacketObserver()
{
    observe_queue = new boost::sync_bounded_queue<DNSPacket>(10000);
    hitmap_flush_thread = new boost::thread(&PacketObserver::save_observe_data, this);
    observe_flush_thread = new boost::thread(&PacketObserver::save_hitmap_data, this);
}

PacketObserver::~PacketObserver()
{
    delete hitmap_flush_thread;
    delete observe_flush_thread;
    delete observe_queue;
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


static const string& getArg(const string &key)
{
  return arg()["gmysql-"+key];
}

int getArgAsNum(const string &key)
{
  return arg().asNum("gmysql-"+key);
}

void PacketObserver::save_observe_data()
{
    SMySQL * sql = nullptr; 
    while(1)
    {
        DNSPacket p = observe_queue->pull_front();

        if (!sql)
        {
            sql = new SMySQL(getArg("dbname"),
                getArg("host"),
                getArgAsNum("port"),
                getArg("socket"),
                getArg("user"),
                getArg("password"));
                sql->setLog(::arg().mustDo("query-logging"));
        }

        ostringstream ss;
        ss << "insert into querylog (qdomain,source_ip,rcode,answer) values(";
        ss <<endl;
        ss << "'" << p.qdomain.toString() << "',";
        ss <<endl;
        ss << "'" << p.getRemote().toString() << "',";
        ss <<endl;
        ss << p.d.rcode << ",";
        ss <<endl;

        string replyString = p.getString();
        replyString.erase(std::remove_if(replyString.begin(), replyString.end(), [](char c){return !isprint(c) || c=='(' || c==')' || c=='\"'|| c=='\'';}), replyString.end());
        ss  << "'" << replyString << "')";
        string query = ss.str();
        sql->execute(query);
    }
}

void PacketObserver::save_hitmap_data()
{
    SMySQL * sql = nullptr; 
    while(1)
    {
        boost::this_thread::sleep_for(boost::chrono::seconds{5});

        if (!sql)
        {
            sql = new SMySQL(getArg("dbname"),
                getArg("host"),
                getArgAsNum("port"),
                getArg("socket"),
                getArg("user"),
                getArg("password"));
                sql->setLog(::arg().mustDo("query-logging"));
        }

        std::map<DNSName, uint32_t> * map;
        {
            boost::lock_guard<boost::recursive_mutex> lock(guard);
            map = hitmap;
            hitmap = nullptr;
            if (!map)
                continue;
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
}