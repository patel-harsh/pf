
#include "data_receiver.h"
#include <chrono>
#include <ctime>
using namespace std;

namespace pepperl_fuchs {

//-----------------------------------------------------------------------------
DataReceiver::DataReceiver():inbuf_(4096),instream_(&inbuf_),ring_buffer_(65536),scan_data_()
{
    udp_socket_ = 0;
    udp_port_ = -1;
    is_connected_ = false;

    try
    {
        udp_socket_ = new boost::asio::ip::udp::socket(io_service_, boost::asio::ip::udp::v4());
        udp_socket_->bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
        udp_port_ = udp_socket_->local_endpoint().port();
        // Start async reading
        udp_socket_->async_receive_from(boost::asio::buffer(&udp_buffer_[0],udp_buffer_.size()), udp_endpoint_,
                                        boost::bind(&DataReceiver::handleSocketRead, this,
                                                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        io_service_thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_));
        is_connected_ = true;
    }
    catch (exception& e)
    {
        cerr << "Exception: " <<  e.what() << endl;
    }
    cout << "Receiving scanner data at local UDP port " << udp_port_ << " ... \n"<<endl;

}

//-----------------------------------------------------------------------------
DataReceiver::~DataReceiver()
{
    disconnect();
    delete udp_socket_;
}

//-----------------------------------------------------------------------------
void DataReceiver::handleSocketRead(const boost::system::error_code &error, size_t bytes_transferred)
{
    if (!error )
    {
        // Read all received data and write it to the internal ring buffer
        writeBufferBack(&udp_buffer_[0],bytes_transferred);

        // Handle (read and parse) packets stored in the internal ring buffer
        while( handleNextPacket() ) {}

        // Read data asynchronously
        udp_socket_->async_receive_from(boost::asio::buffer(&udp_buffer_[0],udp_buffer_.size()), udp_endpoint_,
                                        boost::bind(&DataReceiver::handleSocketRead, this,
                                                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        if( error.value() != 995 )
            cerr << "ERROR: " << "data connection error: " << error.message() << "(" << error.value() << ")" << endl;
        disconnect();
    }
    last_data_time_ = time(0);
}

//-----------------------------------------------------------------------------
bool DataReceiver::handleNextPacket()
{
    // Search for a packet
    int packet_start = findPacketStart();
    if( packet_start<0 )
        return false;

    // Try to retrieve packet
    char buf[65536];
    PacketTypeC* p = (PacketTypeC*) buf;
    if( !retrievePacket(packet_start,p) )
        return false;

    // Lock internal outgoing data queue, automatically unlocks at end of function
    unique_lock<mutex> lock(data_mutex_);

    // Create new scan container if necessary
    if( p->header.packet_number == 1 || scan_data_.empty() )
    {
        scan_data_.emplace_back();
        if( scan_data_.size()>100 )
        {
            scan_data_.pop_front();
            cerr << "Too many scans in receiver queue: Dropping scans!" << endl;
        }
        data_notifier_.notify_one();
    }
    ScanData& scandata = scan_data_.back();

    // Parse payload of packet
    uint32_t* p_scan_data = (uint32_t*) &buf[p->header.header_size];
    int num_scan_points = p->header.num_points_packet;

    for( int i=0; i<num_scan_points; i++ )
    {
        unsigned int data = p_scan_data[i];
        unsigned int distance = (data & 0x000FFFFF);
        unsigned int amplitude = (data & 0xFFFFF000) >> 20;

        scandata.distance_data.push_back(distance);
        scandata.amplitude_data.push_back(amplitude);
    }

    // Save header
    scandata.headers.push_back(p->header);

    return true;
}

//-----------------------------------------------------------------------------
int DataReceiver::findPacketStart()
{
    if( ring_buffer_.size()<60 )
        return -1;
    for( size_t i=0; i<ring_buffer_.size()-4; i++)
    {
        if(   ((unsigned char) ring_buffer_[i])   == 0x5c
           && ((unsigned char) ring_buffer_[i+1]) == 0xa2
           && ((unsigned char) ring_buffer_[i+2]) == 0x43
           && ((unsigned char) ring_buffer_[i+3]) == 0x00 )
        {
            return i;
        }
    }
    return -2;
}

//-----------------------------------------------------------------------------
bool DataReceiver::retrievePacket(size_t start, PacketTypeC *p)
{
    if( ring_buffer_.size()<60 )
        return false;

    // Erase preceding bytes
    ring_buffer_.erase_begin(start);

    char* pp = (char*) p;
    // Read header
    readBufferFront(pp,60);

    if( ring_buffer_.size() < p->header.packet_size )
        return false;

    // Read header+payload data
    readBufferFront(pp,p->header.packet_size);

    // Erase packet from ring buffer
    ring_buffer_.erase_begin(p->header.packet_size);
    return true;
}

//-----------------------------------------------------------------------------
void DataReceiver::disconnect()
{
    is_connected_ = false;
    try
    {
        if( udp_socket_ )
            udp_socket_->close();
        io_service_.stop();
        if( boost::this_thread::get_id() != io_service_thread_.get_id() )
            io_service_thread_.join();
    }
    catch (exception& e)
    {
        cerr << "Exception: " <<  e.what() << endl;
    }
}

//-----------------------------------------------------------------------------
bool DataReceiver::checkConnection()
{
    if( !isConnected() )
        return false;
    if( (time(0)-last_data_time_) > 2 )
    {
        disconnect();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
ScanData DataReceiver::getFullScan()
{
    unique_lock<mutex> lock(data_mutex_);
    while( checkConnection() && isConnected() && scan_data_.size()<2 )
    {
        data_notifier_.wait_for(lock, chrono::seconds(1));
    }
    ScanData data;
    if( scan_data_.size() >= 2 && isConnected() )
    {
        data = ScanData(move(scan_data_.front()));
        scan_data_.pop_front();
    }
    return data;
}

//-----------------------------------------------------------------------------
void DataReceiver::writeBufferBack(char *src, size_t numbytes)
{
    if( ring_buffer_.size()+numbytes > ring_buffer_.capacity() )
        throw exception();
    ring_buffer_.resize(ring_buffer_.size()+numbytes);
    char* pone = ring_buffer_.array_one().first;
    size_t pone_size = ring_buffer_.array_one().second;
    char* ptwo = ring_buffer_.array_two().first;
    size_t ptwo_size = ring_buffer_.array_two().second;

    if( ptwo_size >= numbytes )
    {
        memcpy(ptwo+ptwo_size-numbytes, src, numbytes);
    }
    else
    {
        memcpy(pone+pone_size+ptwo_size-numbytes,
                    src,
                    numbytes-ptwo_size );
        memcpy(ptwo,
                    src+numbytes-ptwo_size,
                    ptwo_size );
    }
}

//-----------------------------------------------------------------------------
void DataReceiver::readBufferFront(char *dst, size_t numbytes)
{
    if( ring_buffer_.size() < numbytes )
        throw exception();
    char* pone = ring_buffer_.array_one().first;
    size_t pone_size = ring_buffer_.array_one().second;
    char* ptwo = ring_buffer_.array_two().first;
    //size_t ptwo_size = ring_buffer_.array_two().second;

    if( pone_size >= numbytes )
    {
        memcpy( dst, pone, numbytes );
    }
    else
    {
        memcpy( dst, pone, pone_size );
        memcpy( dst+pone_size, ptwo, numbytes-pone_size);
    }
}

//-----------------------------------------------------------------------------
}
