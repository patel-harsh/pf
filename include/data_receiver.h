
#ifndef DATA_RECEIVER_H
#define DATA_RECEIVER_H

#define BOOST_CB_DISABLE_DEBUG
#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <array>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <packet_structure.h>
using namespace std;

namespace pepperl_fuchs {

//! Receives data of the laser range finder via IP socket
//! Receives the scanner data with asynchronous functions of the Boost::Asio library
class DataReceiver
{
public:

    //! Open an UDP port and listen on it
    DataReceiver();

    //! Disconnect cleanly
    ~DataReceiver();

    //! Get open and receiving UDP port
    int getUDPPort() const { return udp_port_; }

    //! Return connection status
    bool isConnected() const { return is_connected_; }

    //! Disconnect and cleanup
    void disconnect();

    //! Pop a single full scan out of the internal FIFO queue if there is any
    //! If no full scan is available yet, blocks until a full scan is available
    //! @returns A ScanData struct with distance and amplitude data as well as the packet headers belonging to the data
    ScanData getFullScan();


private:

    //! Asynchronous callback function, called if data has been reveived by the UDP socket
    void handleSocketRead(const boost::system::error_code& error, size_t bytes_transferred);

    //! Try to read and parse next packet from the internal ring buffer
    //! @returns True if a packet has been parsed, false otherwise
    bool handleNextPacket();

    //! Search for magic header bytes in the internal ring buffer
    //! @returns Position of possible packet start, which normally should be zero
    int findPacketStart();

    //! Try to read a packet from the internal ring buffer
    //! @returns True on success, False otherwise
    bool retrievePacket( size_t start, PacketTypeC* p );

    //! Checks if the connection is alive
    //! @returns True if connection is alive, false otherwise
    bool checkConnection();

    //! Read fast from the front of the internal ring buffer
    //! @param dst Destination buffer
    //! @numbytes Number of bytes to read
    void readBufferFront(char* dst, size_t numbytes );

    //! Write fast at the back of the internal ring buffer
    //! @param src Source buffer
    //! @numbytes Number of bytes to write
    void writeBufferBack(char* src, size_t numbytes );

    //! Data (UDP) port at local side
    int udp_port_;

    //! Internal connection state
    bool is_connected_;

    //! Event handler thread
    boost::thread io_service_thread_;
    boost::asio::io_service io_service_;

    //! Boost::Asio streambuffer
    boost::asio::streambuf inbuf_;

    //! Input stream
    istream instream_;

    //! Receiving socket
    boost::asio::ip::udp::socket* udp_socket_;

    //! Endpoint in case of UDP receiver
    boost::asio::ip::udp::endpoint udp_endpoint_;

    //! Buffer in case of UDP receiver
    array< char, 65536 > udp_buffer_;

    //! Internal ringbuffer for temporarily storing reveived data
    boost::circular_buffer<char> ring_buffer_;

    //! Protection against data races between ROS and IO threads
    mutex data_mutex_;

    //! Data notification condition variable
    condition_variable data_notifier_;

    //! Double ended queue with sucessfully received and parsed data, organized as single complete scans
    deque<ScanData> scan_data_;

    //! time in seconds since epoch, when last data was received
    double last_data_time_;
};

}
#endif // DATA_RECEIVER_H
