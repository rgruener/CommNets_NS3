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

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab_1");

int main (int argc, char *argv[]){

    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

    std::string delay = "20ms";
    std::string rate = "5Mbps"; // Data rate in bps;
    double interval = 0.05;

    CommandLine cmd;
    cmd.AddValue ("Delay", "P2P Delay/Latency of Link (string format ex: 20ms)", delay);
    cmd.AddValue ("DataRate", "P2P Data Rate (string format ex: 5Mbps)", rate);
    cmd.AddValue ("PacketInterval", "UDP client packet interval (seconds)", interval);

    cmd.Parse (argc, argv);

    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (rate));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));

    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    UdpServerHelper server_1 (8080);
    UdpServerHelper server_2 (8081);

    ApplicationContainer serverApp_1 = server_1.Install (nodes.Get (1));
    serverApp_1.Start (Seconds (1.0));
    serverApp_1.Stop (Seconds (10.0));

    ApplicationContainer serverApp_2 = server_2.Install (nodes.Get (1));
    serverApp_2.Start (Seconds (1.0));
    serverApp_2.Stop (Seconds (10.0));

    UdpClientHelper client_1 (interfaces.GetAddress (1), 8080);
    UdpClientHelper client_2 (interfaces.GetAddress (1), 8081);
    client_1.SetAttribute ("Interval", TimeValue (Seconds (interval)));
    client_1.SetAttribute ("PacketSize", UintegerValue (1024));
    client_2.SetAttribute ("Interval", TimeValue (Seconds (interval)));
    client_2.SetAttribute ("PacketSize", UintegerValue (1024));


    ApplicationContainer clientApps_1 = client_1.Install (nodes.Get (0));
    clientApps_1.Start (Seconds (2.0));
    clientApps_1.Stop (Seconds (10.0));
    
    ApplicationContainer clientApps_2 = client_2.Install (nodes.Get (0));
    clientApps_2.Start (Seconds (2.0));
    clientApps_2.Stop (Seconds (10.0));

    pointToPoint.EnablePcapAll("lab_1");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop (Seconds(11.0));
    Simulator::Run ();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); i++){
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        if ((t.sourceAddress=="10.1.1.1" && t.destinationAddress == "10.1.1.2")){
            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
            std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() -
                                                                        i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
        }
    }

    monitor->SerializeToXmlFile("lab_1.flowmon", true, true);

    Simulator::Destroy ();
    return 0;
}
