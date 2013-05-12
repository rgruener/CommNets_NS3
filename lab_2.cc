/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Robert Gruener
 * Mark Bryk
 *
 * Lab 2 Network Topology
 *
 *      Node 0 ---+         +--- Node 2
 *                |         |
 *              Node 4 --- Node 5
 *                |         |
 *      Node 1 ---+         +--- Node 3
*/

#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab_2");

class LabTwoApplication: public Application{

    private:
        void StartApplication();
        void StopApplication();

        void ScheduleTransaction();
        void SendPacket();

        Ptr<Socket> socket; // Ptr is ns3 class for pointers
        Address peer;
        int packetSize;
        int numPackets;
        DataRate dataRate;
        EventId sendEvent;
        bool isRunning;
        int numPacketsSent;

    public:
        LabTwoApplication();
        void Setup(Ptr<Socket> socket, Address address, int packetSize, int numPackets, DataRate dataRate);
        void ChangeDataRate(DataRate newDataRate);

};

LabTwoApplication::LabTwoApplication(){
    this->socket = 0;
    this->packetSize = 0;
    this->numPackets = 0;
    this->dataRate = 0;
    this->isRunning = false;
    this->numPacketsSent = 0;
}

void LabTwoApplication::StartApplication(){
    this->isRunning = true;
    this->numPacketsSent = 0;
    this->socket->Bind();
    this->socket->Connect(this->peer);
    this->SendPacket();
}

void LabTwoApplication::StopApplication(){
    this->isRunning = false;

    // Cancel Current Event if In Process of Sending
    if (this->sendEvent.IsRunning()){
        Simulator::Cancel(this->sendEvent);
    }

    if (this->socket){
        this->socket->Close();
    }
}

void LabTwoApplication::SendPacket(){
    Ptr<Packet> newPacket = Create<Packet>(this->packetSize);
    this->socket->Send(newPacket);

    if (++this->numPacketsSent < this->numPackets){
        this->ScheduleTransaction();
    }
}

void LabTwoApplication::ScheduleTransaction(){
    if (this->isRunning){
        Time nextTime(Seconds(this->packetSize*8/static_cast<double>(this->dataRate.GetBitRate())));
        this->sendEvent = Simulator::Schedule(nextTime, &LabTwoApplication::SendPacket, this);
    }
}

void LabTwoApplication::Setup(Ptr<Socket> socket, Address address, int packetSize, int numPackets, DataRate dataRate){
    this->socket = socket;
    this->peer = address;
    this->packetSize = packetSize;
    this->numPackets = numPackets;
    this->dataRate = dataRate;
}
void LabTwoApplication::ChangeDataRate(DataRate newDataRate){
    this->dataRate = newDataRate;
}

void IncreaseDataRate(Ptr<LabTwoApplication> application, DataRate newRate){
    application->ChangeDataRate(newRate);
}

static void TraceCwndChange(uint32_t oldCwnd, uint32_t newCwnd){
    std::cout << Simulator::Now().GetSeconds() << ":" << newCwnd << std::endl;
}

int main (int argc, char *argv[]){
    std::string delay = "2ms"; // Latency in ms
    std::string rate = "500kb/s"; // P2P link
    bool enableFlowMonitor = true; // Packet Interval in seconds

    CommandLine cmd;
    cmd.AddValue ("Delay", "P2P Delay/Latency of Link (string format ex: 2ms", delay);
    cmd.AddValue ("DataRate", "P2P Data Rate (string format ex: 500kb/s)", rate);
    cmd.AddValue ("EnableFlowMonitor", "Enable Flow Monitor (true/false)", enableFlowMonitor);

    cmd.Parse (argc, argv);

    // Create the nodes
    NodeContainer nodes;
    nodes.Create(6);

    NodeContainer n0n4 = NodeContainer (nodes.Get (0), nodes.Get (4));
    NodeContainer n1n4 = NodeContainer (nodes.Get (1), nodes.Get (4));
    NodeContainer n2n5 = NodeContainer (nodes.Get (2), nodes.Get (5));
    NodeContainer n3n5 = NodeContainer (nodes.Get (3), nodes.Get (5));
    NodeContainer n4n5 = NodeContainer (nodes.Get (4), nodes.Get (5));

    // Install Internet Stack
    InternetStackHelper internet;
    internet.Install (nodes);

    // Create Point To Point Links Between Nodes
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
    p2p.SetChannelAttribute ("Delay", StringValue (delay));
    NetDeviceContainer d0d4 = p2p.Install(n0n4);
    NetDeviceContainer d1d4 = p2p.Install(n1n4);
    NetDeviceContainer d4d5 = p2p.Install(n4n5);
    NetDeviceContainer d2d5 = p2p.Install(n2n5);
    NetDeviceContainer d3d5 = p2p.Install(n3n5);

    // Setup IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i4 = ipv4.Assign (d0d4);
    ipv4.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i4 = ipv4.Assign (d1d4);
    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);
    ipv4.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i5 = ipv4.Assign (d2d5);
    ipv4.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i5 = ipv4.Assign (d3d5);

    // Setup Routing Tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // TCP connection from Node 0 to Node 2
    uint16_t tcpHostPort = 8080;
    Address tcpHostAddress(InetSocketAddress(i2i5.GetAddress(0),tcpHostPort));
    PacketSinkHelper tcpPacketSinkHelper("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),tcpHostPort));
    ApplicationContainer tcpHostApps = tcpPacketSinkHelper.Install(nodes.Get(2));
    tcpHostApps.Start (Seconds (0.));
    tcpHostApps.Stop (Seconds (100.));
 
    Ptr<Socket> tcpSocket = Socket::CreateSocket(nodes.Get(0),TcpSocketFactory::GetTypeId());

    // Trace Congestion window
    tcpSocket->TraceConnectWithoutContext("CongestionWindow",MakeCallback(&TraceCwndChange));

    // Create TCP application at Node 0
    Ptr<LabTwoApplication> tcpClientApp = CreateObject<LabTwoApplication>();
    tcpClientApp->Setup(tcpSocket,tcpHostAddress,1040,100000,DataRate("250Kbps"));
    nodes.Get(0)->AddApplication(tcpClientApp);
    tcpClientApp->SetStartTime(Seconds(1.0));
    tcpClientApp->SetStopTime(Seconds(100.0));

    // UDP connection from Node 1 to Node 3

    uint16_t udpHostPort = 8081;
    Address udpHostAddress(InetSocketAddress(i3i5.GetAddress(0),udpHostPort));
    PacketSinkHelper udpPacketSinkHelper("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),udpHostPort));
    ApplicationContainer udpHostApps = udpPacketSinkHelper.Install(nodes.Get(3));
    udpHostApps.Start(Seconds(0.0));
    udpHostApps.Stop(Seconds(100.0));

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket(nodes.Get(1),UdpSocketFactory::GetTypeId());

    // Create UDP application at Node 1
    Ptr<LabTwoApplication> udpClientApp = CreateObject<LabTwoApplication>();
    udpClientApp->Setup(ns3UdpSocket, udpHostAddress,1040,100000,DataRate("250Kbps"));
    nodes.Get(1)->AddApplication(udpClientApp);
    udpClientApp->SetStartTime(Seconds(20.0));
    udpClientApp->SetStopTime (Seconds(100.0));

    // Increase UDP Rate at time 30s
    Simulator::Schedule(Seconds(30.0),&IncreaseDataRate,udpClientApp,DataRate("500kbps"));

    // Setup Flow Monitor
    Ptr<FlowMonitor> flowmon;
    if (enableFlowMonitor){
      FlowMonitorHelper flowMonitorHelper;
      flowmon = flowMonitorHelper.InstallAll();
    }

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();
    if (enableFlowMonitor){
        flowmon->CheckForLostPackets();
        flowmon->SerializeToXmlFile("lab_2.flowmon",true,true);
    }
    Simulator::Destroy();
}
