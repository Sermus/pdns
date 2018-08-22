#ifndef PACKETOBSERVER_HH
#define PACKETOBSERVER_HH

#include <map>
#include <boost/thread.hpp>
#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "dnspacket.hh"
#include "modules/gmysqlbackend/smysql.hh"

class PacketObserver
{
    private:
        static const uint32_t BATCH_SIZE = 1000;
        string PARAM_PREFIX;
        bool finish_threads;

    private:
        boost::sync_bounded_queue<DNSPacket> *observe_queue;
        std::map<DNSName, uint32_t> *hitmap;
        boost::recursive_mutex guard;
        boost::thread *hitmap_flush_thread;
        boost::thread *observe_flush_thread;

    public:
        PacketObserver();
        ~PacketObserver();
        void init();
        void deinit();
        void set_queue_size(size_t size);
        void observe(DNSPacket &p);

    private:
        void save_observe_data();
        void save_hitmap_data();
        string serialize_packet(DNSPacket &p);
        void declare(const string &param, const string &help, const string &value);
        const string& getArg(const string &key);
        int getArgAsNum(const string &key);
        SMySQL *get_sql();
};

extern PacketObserver po;

#endif