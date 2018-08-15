#ifndef PACKETOBSERVER_HH
#define PACKETOBSERVER_HH

#include <map>
#include <boost/thread.hpp>
#include <boost/thread/sync_bounded_queue.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "dnspacket.hh"

class PacketObserver
{
    private:
        boost::sync_bounded_queue<DNSPacket> *observe_queue;
        std::map<DNSName, uint32_t> *hitmap;
        boost::recursive_mutex guard;
        boost::thread *hitmap_flush_thread;
        boost::thread *observe_flush_thread;

    public:
        PacketObserver();
        ~PacketObserver();
        void set_queue_size(size_t size);
        void observe(DNSPacket &p);

    private:
        void save_observe_data();
        void save_hitmap_data();
};

extern PacketObserver po;

#endif