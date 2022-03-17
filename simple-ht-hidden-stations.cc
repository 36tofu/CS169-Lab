
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

// This example considers two hidden stations in an 802.11n network which supports MPDU aggregation.
// The user can specify whether RTS/CTS is used and can set the number of aggregated MPDUs.
//
// Example: ./waf --run "simple-ht-hidden-stations --enableRts=1 --nMpdus=8"
//
// Network topology:
//
//   Wifi 192.168.1.0
//
//        AP
//   *    *    *
//   |    |    |
//   n1   n2   n3
//
// Packets in this simulation aren't marked with a QosTag so they are considered
// belonging to BestEffort Access Class (AC_BE).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimplesHtHiddenStations");

uint64_t packetSent = 0;
uint64_t packetRec = 0;


void clientSend (std::string context, Ptr<const Packet> packet)
{
	//NS_LOG_UNCOND(context);
	packetSent+=1;
}


void serverRec (std::string context, Ptr<const Packet> packet)
{
	packetRec+=1;
}

int main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472; //bytes
  uint64_t simulationTime = 10; //seconds
  //float intervalTime = 0.1; //seconds
  uint32_t nMpdus = 1;
  uint32_t maxAmpduSize = 0;
  bool enableRts = 0;

  CommandLine cmd;
  cmd.AddValue ("nMpdus", "Number of aggregated MPDUs", nMpdus);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("enableRts", "Enable RTS/CTS", enableRts); // 1: RTS/CTS enabled; 0: RTS/CTS disabled
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  if (!enableRts)
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
    }
  else
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }

  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

  //Set the maximum size for A-MPDU with regards to the payload size
  maxAmpduSize = nMpdus * (payloadSize + 200);

  // Set the maximum wireless range to 5 meters in order to reproduce a hidden nodes scenario, i.e. the distance between hidden stations is larger than 5 meters
  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (5));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (4);  //Increase # of station nodes to 4
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.AddPropagationLoss ("ns3::RangePropagationLossModel"); //wireless range limited to 5 meters!

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("HtMcs7"), "ControlMode", StringValue ("HtMcs0"));
  WifiMacHelper mac;

  Ssid ssid = Ssid ("simple-mpdu-aggregation");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false),
               "BE_MaxAmpduSize", UintegerValue (maxAmpduSize));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconInterval", TimeValue (MicroSeconds (102400)),
               "BeaconGeneration", BooleanValue (true),
               "BE_MaxAmpduSize", UintegerValue (maxAmpduSize));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // Setting mobility model
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  // AP is between the two stations, each station being located at 5 meters from the AP.
  // The distance between the two stations is thus equal to 10 meters.
  // Since the wireless range is limited to 5 meters, the two stations are hidden from each other.
  // (X,Y,Z)
  positionAlloc->Add (Vector (5.0, 5.0, 0.0));  //set position of AP 
  positionAlloc->Add (Vector (5.0, 10.0, 0.0)); //set positions of MS
  positionAlloc->Add (Vector (0.0, 5.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 5.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  // Internet stack
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevices);
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);

  //Create 4 servers ports 9-12
  UdpEchoServerHelper myServer (9);
  UdpEchoServerHelper myServer1 (10);
  UdpEchoServerHelper myServer2 (11);
  UdpEchoServerHelper myServer3 (12);
  
  //Install 4 servers on AP node 
  ApplicationContainer serverApp = myServer.Install (wifiApNode);
  serverApp.Start (Seconds (0.0));
  serverApp.Stop (Seconds (simulationTime + 2));
   
  ApplicationContainer serverApp1 = myServer1.Install (wifiApNode);
  serverApp1.Start (Seconds (0.0));
  serverApp1.Stop (Seconds (simulationTime + 2));
  
  ApplicationContainer serverApp2 = myServer2.Install (wifiApNode);
  serverApp2.Start (Seconds (0.0));
  serverApp2.Stop (Seconds (simulationTime + 2));
  
  ApplicationContainer serverApp3 = myServer3.Install (wifiApNode);
  serverApp3.Start (Seconds (0.0));
  serverApp3.Stop (Seconds (simulationTime + 2));
  

  //Install UDP clients on each of the MS nodes 
  UdpEchoClientHelper myClient (ApInterface.GetAddress (0), 9);
  myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClient.SetAttribute ("Interval", TimeValue (Time ("0.1"))); //packets/s
  myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  ApplicationContainer clientApp0 = myClient.Install(wifiStaNodes.Get(0));
  clientApp0.Start(Seconds(1));
  clientApp0.Stop (Seconds (simulationTime+1));
 
   
  UdpEchoClientHelper myClient1 (ApInterface.GetAddress (0), 10);
  myClient1.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClient1.SetAttribute ("Interval", TimeValue (Time ("0.1"))); //packets/s
  myClient1.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  ApplicationContainer clientApp1 = myClient1.Install(wifiStaNodes.Get(0));
  clientApp0.Start(Seconds(1));
  clientApp0.Stop (Seconds (simulationTime+1));
  
  UdpEchoClientHelper myClient2 (ApInterface.GetAddress (0), 11);
  myClient2.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClient2.SetAttribute ("Interval", TimeValue (Time ("0.1"))); //packets/s
  myClient2.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  ApplicationContainer clientApp2 = myClient2.Install(wifiStaNodes.Get(0));
  clientApp0.Start(Seconds(1));
  clientApp0.Stop (Seconds (simulationTime+1));
  
  UdpEchoClientHelper myClient3 (ApInterface.GetAddress (0), 12);
  myClient3.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClient3.SetAttribute ("Interval", TimeValue (Time ("0.1"))); //packets/s
  myClient3.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  ApplicationContainer clientApp3 = myClient3.Install(wifiStaNodes.Get(0));
  clientApp0.Start(Seconds(1));
  clientApp0.Stop (Seconds (simulationTime+1));
  

  /*
  // Saturated UDP traffic from stations to AP
  ApplicationContainer clientApp1 = myClient.Install (wifiStaNodes);
  clientApp1.Start (Seconds (1.0));
  clientApp1.Stop (Seconds (simulationTime + 1));
  */

  phy.EnablePcap ("SimpleHtHiddenStations_Ap", apDevice.Get (0));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta1", staDevices.Get (0));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta2", staDevices.Get (1));

  Simulator::Stop (Seconds (simulationTime + 1));

  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx", MakeCallback(&clientSend));
  Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoServer/Rx", MakeCallback(&serverRec));

  Simulator::Run ();
  Simulator::Destroy ();

 //uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
  double throughput = packetRec * payloadSize * 8 / (simulationTime * 1000000.0);
  std::cout << "Throughput: " << throughput << " Mbit/s" << '\n';

  return 0;
}
