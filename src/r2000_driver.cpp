
#include <ctime>
#include <r2000_driver.h>
#include <packet_structure.h>
#include <command_interface.h>
#include <data_receiver.h>
using namespace std;

namespace pepperl_fuchs {

R2000Driver::R2000Driver()
{
    command_interface_ = 0;
    data_receiver_ = 0;
    is_connected_ = false;
    is_capturing_ = false;
    watchdog_feed_time_ = 0;
}

//-----------------------------------------------------------------------------
bool R2000Driver::connect(const string hostname, int port)
{
    command_interface_ = new CommandInterface(hostname,port);
    auto opi = command_interface_->getProtocolInfo();
    if( !opi || (*opi).version_major != 1 )
    {
        cerr << "ERROR: Could not connect to laser range finder!" << endl;
        return false;
    }

    if( (*opi).version_major != 1 )
    {
        cerr << "ERROR: Wrong protocol version (version_major=" << (*opi).version_major << ", version_minor=" << (*opi).version_minor << ")" << endl;
        return false;
    }

    protocol_info_ = *opi;
    parameters_ = command_interface_->getParameters(command_interface_->getParameterList());
    is_connected_ = true;
    return true;
}

//-----------------------------------------------------------------------------
R2000Driver::~R2000Driver()
{
    disconnect();
}

//-----------------------------------------------------------------------------
bool R2000Driver::startCapturingUDP()
{
    if( !checkConnection() )
        return false;

    data_receiver_ = new DataReceiver();
    if( !data_receiver_->isConnected() )
        return false;
    int udp_port = data_receiver_->getUDPPort();

    handle_info_ = command_interface_->requestHandleUDP(udp_port);
    if( !handle_info_ || !command_interface_->startScanOutput((*handle_info_).handle) )
        return false;

    food_timeout_ = floor(max((handle_info_->watchdog_timeout/1000.0/3.0),1.0));
    is_capturing_ = true;
    return true;
}

//-----------------------------------------------------------------------------
bool R2000Driver::stopCapturing()
{
    if( !is_capturing_ || !command_interface_ )
        return false;

    bool return_val = checkConnection();

    return_val = return_val && command_interface_->stopScanOutput((*handle_info_).handle);

    delete data_receiver_;
    data_receiver_ = 0;

    is_capturing_ = false;
    return_val = return_val && command_interface_->releaseHandle(handle_info_->handle);
    handle_info_ = boost::optional<HandleInfo>();
    return return_val;
}

//-----------------------------------------------------------------------------
bool R2000Driver::checkConnection()
{
    if( !command_interface_ || !isConnected() || !command_interface_->getProtocolInfo() )
    {
        cerr << "ERROR: No connection to laser range finder or connection lost!" << endl;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
ScanData R2000Driver::getFullScan()
{
    feedWatchdog();
    if( data_receiver_ )
        return data_receiver_->getFullScan();
    else
    {
        cerr << "ERROR: No scan capturing started!" << endl;
        return ScanData();
    }
}

//-----------------------------------------------------------------------------
void R2000Driver::disconnect()
{
    if( isCapturing() )
        stopCapturing();

    delete data_receiver_;
    delete command_interface_;
    data_receiver_ = 0;
    command_interface_ = 0;

    is_capturing_ = false;
    is_connected_ = false;

    handle_info_ = boost::optional<HandleInfo>();
    protocol_info_ = ProtocolInfo();
    parameters_ = map< string, string >();
}

//-----------------------------------------------------------------------------
bool R2000Driver::isCapturing()
{
    return is_capturing_ && data_receiver_->isConnected();
}

//-----------------------------------------------------------------------------
const map< string, string >& R2000Driver::getParameters()
{
    if( command_interface_ )
        parameters_ = command_interface_->getParameters(command_interface_->getParameterList());
    return parameters_;
}

//-----------------------------------------------------------------------------
bool R2000Driver::setScanFrequency(unsigned int frequency)
{
    if( !command_interface_ )
        return false;
    return command_interface_->setParameter("scan_frequency",to_string(frequency));
}

//-----------------------------------------------------------------------------
bool R2000Driver::setSamplesPerScan(unsigned int samples)
{
    if( !command_interface_ )
        return false;
    return command_interface_->setParameter("samples_per_scan",to_string(samples));
}

//-----------------------------------------------------------------------------
bool R2000Driver::rebootDevice()
{
    if( !command_interface_ )
        return false;
    return command_interface_->rebootDevice();
}

//-----------------------------------------------------------------------------
bool R2000Driver::resetParameters(const vector<string> &names)
{
    if( !command_interface_ )
        return false;
    return command_interface_->resetParameters(names);
}

//-----------------------------------------------------------------------------
bool R2000Driver::setParameter(const string &name, const string &value)
{
    if( !command_interface_ )
        return false;
    return command_interface_->setParameter(name,value);
}

//-----------------------------------------------------------------------------
void R2000Driver::feedWatchdog(bool feed_always)
{
    const double current_time = time(0);

    if( (feed_always || watchdog_feed_time_<(current_time-food_timeout_)) && handle_info_ && command_interface_  )
    {
        if( !command_interface_->feedWatchdog(handle_info_->handle) )
            cerr << "ERROR: Feeding watchdog failed!" << endl;
        watchdog_feed_time_ = current_time;
    }
}

}
