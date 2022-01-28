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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (4);
  //node 0 is server
  //node 1-3 are clients 

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("4ms"));

  //install internet stack on all nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  //Set IP network address 
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.2.0", "255.255.255.0");

  Ipv4AddressHelper address2;
  address2.SetBase ("10.1.3.0", "255.255.255.0");
 
  //assign IP to NetDevice 
  //Ipv4InterfaceContainer interfaces = address.Assign (den91Gvices);

  UdpEchoServerHelper echoServer (9);

  //assign node 0 as server 
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (25.0));

  //Install p2p link between server and all clients
  //and assign IP to net device each time 
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
  Ipv4InterfaceContainer interface = address.Assign(devices);


  NetDeviceContainer devices1;
  devices1 = pointToPoint.Install (nodes.Get(0), nodes.Get(2));
  Ipv4InterfaceContainer interface1 = address1.Assign(devices1);


  NetDeviceContainer devices2;
  devices2= pointToPoint.Install (nodes.Get(0), nodes.Get(3));
  Ipv4InterfaceContainer interface2 = address2.Assign(devices2);


  std::vector<Ipv4InterfaceContainer> interfaces;
  interfaces.push_back(interface);
  interfaces.push_back(interface1);
  interfaces.push_back(interface2);



  for(int i = 0; i<=2; i++)
{
  UdpEchoClientHelper echoClient (interfaces[i].GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (4));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (i+1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (25.0));
}

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
