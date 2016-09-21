
#ifndef COMMAND_INTERFACE_H
#define COMMAND_INTERFACE_H
#include <string>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <protocol_info.h>
using namespace std;

namespace pepperl_fuchs {

//! Allows accessing the HTTP/JSON interface
class CommandInterface
{
public:
    //! Setup a new HTTP command interface
    //! @param http_ip IP or DNS name of sensor
    //! @param http_port HTTP/TCP port of sensor
    CommandInterface(const string& http_host, int http_port=80);

    //! Get the HTTP hostname/IP of the scanner
    const string& getHttpHost() const { return http_host_; }

    //! Set sensor parameter
    //! @param name Name
    //! @param value Value
    //! @returns True on success, false otherwise
    bool setParameter(const string name, const string value);

    //! Get sensor parameter
    //! @param name Parameter name
    //! @returns Optional string value with value of given parameter name
    boost::optional<string> getParameter(const string name);

    //! Get multiple sensor parameters
    //! @param names Parameter names
    //! @returns vector with string values with the values of the given parameter names
    map< string, string > getParameters( const vector< string >& names );

    //! List available ro/rw parameters
    //! @returns A vector with the names of all available parameters
    vector< string > getParameterList();

    //! Get protocol info (protocol_name, version, commands)
    //! @returns A struct with the requested data
    boost::optional<ProtocolInfo> getProtocolInfo();

    //! Request UDP handle
    //! @param port Set UDP port where scanner data should be sent to
    //! @param hostname Optional: Set hostname/IP where scanner data should be sent to, local IP is determined automatically if not specified
    //! @param start_angle Optional: Set start angle for scans in the range [0,3600000] (1/10000°), defaults to -1800000
    //! @returns A valid HandleInfo on success, an empty boost::optional<HandleInfo> container otherwise
    boost::optional<HandleInfo> requestHandleUDP(int port, string hostname = string(""), int start_angle=-1800000);

    //! Release handle
    bool releaseHandle( const string& handle );

    //! Initiate output of scan data
    bool startScanOutput( const string& handle );

    //! Terminate output of scan data
    bool stopScanOutput( const string& handle );

    //! Feed the watchdog to keep the handle alive
    bool feedWatchdog( const string& handle );

    //! Reboot laserscanner
    bool rebootDevice();

    //! Reset laserscanner parameters to factory default
    //! @param names Names of parameters to reset
    bool resetParameters(const vector< string >& names);

    //! Discovers the local IP of the NIC which talks to the laser range finder
    //! @returns The local IP as a string, an empty string otherwise
    string discoverLocalIP();

private:

    //! Send a HTTP-GET request to http_ip_ at http_port_
    //! @param requestStr The last part of an URL with a slash leading
    //! @param header  The response header returned as string, empty string in case of an error
    //! @param content The response content returned as string, empty string in case of an error
    //! @returns The HTTP status code or 0 in case of an error
    int httpGet(const string request_path, string& header, string& content);

    //! Send a sensor specific HTTP-Command
    //! @param cmd command name
    //! @param keys_values parameter->value map, which is encoded in the GET-request: ?p1=v1&p2=v2
    bool sendHttpCommand(const string cmd, const map< string, string > param_values );

    //! Send a sensor specific HTTP-Command with a single parameter
    //! @param cmd Command name
    //! @param param Parameter
    //! @param value Value
    bool sendHttpCommand(const string cmd, const string param = "", const string value = "" );

    //! Check the error code and text of the returned JSON by the last request
    //! @returns False in case of an error, True otherwise
    bool checkErrorCode();

    //! Scanner IP
    string http_host_;

    //! Port of HTTP-Interface
    int http_port_;

    //! Returned JSON as property_tree
    boost::property_tree::ptree pt_;

    //! HTTP-Status code of last request
    int http_status_code_;

};
}

#endif // COMMAND_INTERFACE_H
