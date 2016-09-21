
#include <command_interface.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace std;

namespace pepperl_fuchs {

//-----------------------------------------------------------------------------
CommandInterface::CommandInterface(const string &http_host, int http_port)
{
    http_host_ = http_host;
    http_port_ = http_port;
    http_status_code_ = 0;
}

//-----------------------------------------------------------------------------
int CommandInterface::httpGet(const string request_path, string &header, string &content)
{
    header = "";
    content = "";
    using boost::asio::ip::tcp;
    try
    {
        boost::asio::io_service io_service;

        // Lookup endpoint
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(http_host_, to_string(http_port_));
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        // Create socket
        tcp::socket socket(io_service);
        boost::system::error_code error = boost::asio::error::host_not_found;

        // Iterate over endpoints and etablish connection
        while (error && endpoint_iterator != end)
        {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw boost::system::system_error(error);

        // Prepare request
        boost::asio::streambuf request;
        ostream request_stream(&request);
        request_stream << "GET " << request_path << " HTTP/1.0\r\n\r\n";

        boost::asio::write(socket, request);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        istream response_stream(&response);
        string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        string status_message;
        getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            cout << "Invalid response\n";
            return 0;
        }

        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        string tmp;
        while (getline(response_stream, tmp) && tmp != "\r")
            header += tmp+"\n";

        // Write whatever content we already have to output.
        while (getline(response_stream, tmp))
            content += tmp;

        // Read until EOF, writing data to output as we go.
        while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
        {
            response_stream.clear();
            while (getline(response_stream, tmp))
                content += tmp;
        }

        if (error != boost::asio::error::eof)
            throw boost::system::system_error(error);

        // Substitute CRs by a space
        for( size_t i=0; i<header.size(); i++ )
            if( header[i] == '\r' )
                header[i] = ' ';

        for( size_t i=0; i<content.size(); i++ )
            if( content[i] == '\r' )
                content[i] = ' ';

        return status_code;
    }
    catch (exception& e)
    {
        cerr << "Exception: " <<  e.what() << endl;
        return 0;
    }
}

//-----------------------------------------------------------------------------
bool CommandInterface::sendHttpCommand(const string cmd, const map<string, string> param_values)
{
    // Build request string
    string request_str = "/cmd/" + cmd + "?";
    for( auto& kv : param_values )
        request_str += kv.first + "=" + kv.second + "&";
    if(request_str.back() == '&' )
        request_str = request_str.substr(0,request_str.size()-1);

    // Do HTTP request
    string header, content;
    http_status_code_ = httpGet(request_str,header,content);

    // Try to parse JSON response
    try
    {
        stringstream ss(content);
        boost::property_tree::json_parser::read_json(ss,pt_);
    }
    catch (exception& e)
    {
        cerr << "ERROR: Exception: " <<  e.what() << endl;
        return false;
    }

    // Check HTTP-status code
    if( http_status_code_ != 200 )
        return false;
    else
        return true;
}

//-----------------------------------------------------------------------------
bool CommandInterface::sendHttpCommand(const string cmd, const string param, const string value)
{
    map<string, string> param_values;
    if( param != "" )
        param_values[param] = value;
    return sendHttpCommand(cmd,param_values);
}

//-----------------------------------------------------------------------------
bool CommandInterface::setParameter(const string name, const string value)
{
    return sendHttpCommand("set_parameter",name,value) && checkErrorCode();
}

//-----------------------------------------------------------------------------
boost::optional< string > CommandInterface::getParameter(const string name)
{
    if( !sendHttpCommand("get_parameter","list",name) || ! checkErrorCode()  )
        return boost::optional<string>();
    return pt_.get_optional<string>(name);
}

//-----------------------------------------------------------------------------
map< string, string > CommandInterface::getParameters(const vector<string> &names)
{
    // Build request string
    map< string, string > key_values;
    string namelist;
    for( const auto& s: names )
        namelist += (s + ";");
    namelist.substr(0,namelist.size()-1);

    // Read parameter values via HTTP/JSON request/response
    if( !sendHttpCommand("get_parameter","list",namelist) || ! checkErrorCode()  )
        return key_values;

    // Extract values from JSON property_tree
    for( const auto& s: names )
    {
        auto ov = pt_.get_optional<string>(s);
        if( ov )
            key_values[s] = *ov;
        else
            key_values[s] = "--COULD NOT RETRIEVE VALUE--";
    }

    return key_values;
}

//-----------------------------------------------------------------------------
bool CommandInterface::checkErrorCode()
{
    // Check the JSON response if error_code == 0 && error_text == success
    boost::optional<int> error_code = pt_.get_optional<int>("error_code");
    boost::optional<string> error_text = pt_.get_optional<string>("error_text");
    if( !error_code || (*error_code) != 0 || !error_text || (*error_text) != "success" )
    {
        if( error_text )
            cerr << "ERROR: scanner replied: " << *error_text << endl;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
boost::optional<ProtocolInfo> CommandInterface::getProtocolInfo()
{
    // Read protocol info via HTTP/JSON request/response
    if( !sendHttpCommand("get_protocol_info") || !checkErrorCode() )
        return boost::optional<ProtocolInfo>();

    // Read and set protocol info
    boost::optional<string> protocol_name = pt_.get_optional<string>("protocol_name");
    boost::optional<int> version_major = pt_.get_optional<int>("version_major");
    boost::optional<int> version_minor = pt_.get_optional<int>("version_minor");
    auto ocommands = pt_.get_child_optional("commands");
    if( !protocol_name || !version_major || !version_minor || !ocommands )
        return boost::optional<ProtocolInfo>();

    ProtocolInfo pi;
    pi.protocol_name = *protocol_name;
    pi.version_major = *version_major;
    pi.version_minor = *version_minor;

    // Read available commands of the protocol
    boost::property_tree::ptree commands = *ocommands;
    for( auto i= commands.begin(); i!=commands.end(); i++ )
    {
        string cmd = i->second.get<string>("");
        pi.commands.push_back(cmd);
    }

    return pi;
}

//-----------------------------------------------------------------------------
vector< string > CommandInterface::getParameterList()
{
    // Read available parameters via HTTP/JSON request/response
    vector< string > parameter_list;
    if( !sendHttpCommand("list_parameters") || !checkErrorCode() )
        return parameter_list;

    // Check if JSON contains the key "parameters"
    auto oparameters = pt_.get_child_optional("parameters");
    if( !oparameters )
        return parameter_list;

    // Extract parameter names from JSON
    boost::property_tree::ptree parameters = *oparameters;
    for( auto i= parameters.begin(); i!=parameters.end(); i++ )
    {
        string param = i->second.get<string>("");
        parameter_list.push_back(param);
    }

    return parameter_list;

}

//-----------------------------------------------------------------------------
boost::optional<HandleInfo> CommandInterface::requestHandleUDP(int port, string hostname, int start_angle)
{
    // Prepare HTTP request
    if( hostname == "" )
        hostname = discoverLocalIP();
    map< string, string > params;
    params["packet_type"] = "C";
    params["start_angle"] = to_string(start_angle);
    params["port"] = to_string(port);
    params["address"] = hostname;

    // Request handle via HTTP/JSON request/response
    if( !sendHttpCommand("request_handle_udp", params) || !checkErrorCode() )
        return boost::optional<HandleInfo>();

    // Extract handle info from JSON response
    boost::optional<string> handle = pt_.get_optional<string>("handle");
    if(!handle)
        return boost::optional<HandleInfo>();

    // Prepare return value
    HandleInfo hi;
    hi.handle_type = HandleInfo::HANDLE_TYPE_UDP;
    hi.handle = *handle;
    hi.hostname = hostname;
    hi.port = port;
    hi.packet_type = 'C';
    hi.start_angle = start_angle;
    hi.watchdog_enabled = true;
    hi.watchdog_timeout = 60000;
    return hi;
}

//-----------------------------------------------------------------------------
bool CommandInterface::releaseHandle(const string& handle)
{
    if( !sendHttpCommand("release_handle", "handle", handle) || !checkErrorCode() )
        return false;
    return true;
}

//-----------------------------------------------------------------------------
bool CommandInterface::startScanOutput(const string& handle)
{
    if( !sendHttpCommand("start_scanoutput", "handle", handle) || !checkErrorCode() )
        return false;
    return true;
}

//-----------------------------------------------------------------------------
bool CommandInterface::stopScanOutput(const string& handle)
{
    if( !sendHttpCommand("stop_scanoutput", "handle", handle) || !checkErrorCode() )
        return false;
    return true;
}
//-----------------------------------------------------------------------------
bool CommandInterface::feedWatchdog(const string &handle)
{
    if( !sendHttpCommand("feed_watchdog", "handle", handle) || !checkErrorCode() )
        return false;
    return true;
}

//-----------------------------------------------------------------------------
bool CommandInterface::rebootDevice()
{
    if( !sendHttpCommand("reboot_device") || !checkErrorCode() )
        return false;
    return true;
}

//-----------------------------------------------------------------------------
bool CommandInterface::resetParameters(const vector<string> &names)
{
    // Prepare HTTP request
    string namelist;
    for( const auto& s: names )
        namelist += (s + ";");
    namelist.substr(0,namelist.size()-1);

    if( !sendHttpCommand("reset_parameter","list",namelist) || ! checkErrorCode()  )
        return false;

    return true;
}

//-----------------------------------------------------------------------------
string CommandInterface::discoverLocalIP()
{
    string local_ip;
    try
    {
        using boost::asio::ip::udp;
        boost::asio::io_service netService;
        udp::resolver resolver(netService);
        udp::resolver::query query(udp::v4(), http_host_ , "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        local_ip = addr.to_string();
    }
    catch (exception& e)
    {
        cerr << "Could not deal with socket-exception: " << e.what() << endl;
    }

    return local_ip;
}

}
