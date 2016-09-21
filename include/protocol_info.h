
#ifndef PROTOCOL_INFO_H
#define PROTOCOL_INFO_H
#include <vector>
using namespace std;

namespace pepperl_fuchs {

//! \class ProtocolInfo
//! \brief Information about the HTTP/JSON protocol
struct ProtocolInfo
{
    //! protocol name, defaults to "pfsdp"
    string protocol_name;

    //! Major version of protocol
    int version_major;

    //! Minor version of protocol
    int version_minor;

    //! List of available commands
    vector< string > commands;
};

//! \class HandleInfo
//! \brief Encapsulates data about an etablished data connection
struct HandleInfo
{
    static const int HANDLE_TYPE_UDP = 1;

    int handle_type;

    //! UDP Handle: IP at client side where scanner data is sent to
    string hostname;

    //! UDP port at client side where scanner data is sent to
    int port;

    //! Handle ID
    string handle;

    //! Packet type, possible values are A,B or C
    char packet_type;

    //! Start angle of scan in 1/10000°, defaults to -1800000
    int start_angle;

    //! If watchdog is enabled, it has to be fed otherwise scanner closes the connection after timeout
    bool watchdog_enabled;

    //! Watchdog timeout in ms
    int watchdog_timeout;
};
}

#endif // PROTOCOL_INFO_H
