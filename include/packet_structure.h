
#ifndef PACKET_STRUCTURE_H
#define PACKET_STRUCTURE_H
#include <cstdint>
#include <vector>
using namespace std;

namespace pepperl_fuchs {

#pragma pack(1)

struct PacketHeader
{
    //! Magic bytes, must be  5C A2 (hex)
    uint16_t magic;

    //! Packet type, must be 43 00 (hex)
    uint16_t packet_type;

    //! Overall packet size (header+payload), 1404 bytes with maximum payload
    uint32_t packet_size;

    //! Header size, defaults to 60 bytes
    uint16_t header_size;

    //! Sequence for scan (incremented for every scan, starting with 0, overflows)
    uint16_t scan_number;

    //! Sequence number for packet (counting packets of a particular scan, starting with 1)
    uint16_t packet_number;

    //! Raw timestamp of internal clock in NTP time format
    uint64_t timestamp_raw;

    //! With an external NTP server synced Timestamp  (currenty not available and and set to zero)
    uint64_t timestamp_sync;

    //! Status flags
    uint32_t status_flags;

    //! Frequency of scan-head rotation in mHz (Milli-Hertz)
    uint32_t scan_frequency;

    //! Total number of scan points (samples) within complete scan
    uint16_t num_points_scan;

    //! Total number of scan points within this packet
    uint16_t num_points_packet;

    //! Index of first scan point within this packet
    uint16_t first_index;

    //! Absolute angle of first scan point within this packet in 1/10000°
    int32_t first_angle;

    //! Delta between two succeding scan points 1/10000°
    int32_t angular_increment;

    //! Output status
    uint32_t output_status;

    //! Field status
    uint32_t field_status;

    //! Possible padding to align header size to 32bit boundary
    //uint8 padding[0];
};


struct PacketTypeC
{
    PacketHeader header;
    uint32_t distance_amplitude_payload; // distance 20 bit, amplitude 12 bit
};
#pragma pack()

//! \struct ScanData
//! \brief Normally contains one complete laserscan (a full rotation of the scanner head)
struct ScanData
{
    //! Distance data in polar form in millimeter
    vector<uint32_t> distance_data;

    //! Amplitude data, values lower than 32 indicate an error or undefined values
    vector<uint32_t> amplitude_data;

    //! Header received with the distance and amplitude data
    vector<PacketHeader> headers;
};

}

#endif // PACKET_STRUCTURE_H
