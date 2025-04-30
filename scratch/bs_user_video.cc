/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/* *
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
 *
 * Author: Michele Polese <michele.polese@gmail.com>
 * Author: Argha Sen <arghasen10@gmail.com>
 */

#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/mmwave-energy-helper.h"
#include "ns3/mmwave-radio-energy-model-enb-helper.h"
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/three-gpp-propagation-loss-model.h>
#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>
#include <ns3/object-factory.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/flow-monitor.h>

using namespace ns3;
using namespace mmwave;

/**
 * Sample simulation script for MC device. It instantiates a LTE and two MmWave eNodeB,
 * attaches one MC UE to both and starts a flow for the UE to and from a remote host.
 */

NS_LOG_COMPONENT_DEFINE ("McTwoEnbs");


// The number of bytes to send in this simulation.
static const uint32_t totalTxBytes = 100000000;
static uint32_t currentTxBytes = 0;
// Perform series of 1040 byte writes (this is a multiple of 26 since
// we want to detect data splicing in the output stream)
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];

void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);
double energyofBS;
double energyofUe;
double totalPackets;
double energy_per_byte;
// static void 
// CwndTracer (uint32_t oldval, uint32_t newval)
// {
//   NS_LOG_INFO ("Moving cwnd from " << oldval << " to " << newval);
// }

std::ofstream packetSinkFile("packetsink_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::trunc);
void ReceivedPacket(double totaloldbytesReceived, double totalnewbytesReceived)
{
  if (!packetSinkFile.is_open())
    {
        packetSinkFile.open("packetsink_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::app);
        if (packetSinkFile.is_open())
        {
            packetSinkFile << "Time (s),Total Packets (bytes), Current Packet (bytes), Energy/Bytes" << std::endl;
        }
        else
        {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }
    }
    Time currentTime = Simulator::Now ();
    energy_per_byte = energyofBS/totalPackets;
    //std::cout << currentTime.GetSeconds () << "," << totalnewEnergyConsumption << "," << (totalnewEnergyConsumption-totaloldEnergyConsumption) <<std::endl;
    packetSinkFile << currentTime.GetSeconds() << ","
                 << totalnewbytesReceived << ","
                 << (totalnewbytesReceived - totaloldbytesReceived) <<","<<energy_per_byte << std::endl;
  totalPackets = totalnewbytesReceived;
  
}

void
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          Ptr<MmWaveUeNetDevice> mmuedev = node->GetDevice (j)->GetObject <MmWaveUeNetDevice> ();
          Ptr<McUeNetDevice> mcuedev = node->GetDevice (j)->GetObject <McUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at " << pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mmuedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmuedev->GetImsi ()
                      << "\" at " << pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mcuedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mcuedev->GetImsi ()
                      << "\" at " << pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void
DoubleShadowingStd(double oldValue, double newValue)
{
  std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
}

void
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<MmWaveEnbNetDevice> mm2 = CreateObject<MmWaveEnbNetDevice> ();
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          Ptr<MmWaveEnbNetDevice> mmdev = node->GetDevice (j)->GetObject <MmWaveEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at " << pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"blue\" front  point pt 4 ps 0.3 lc rgb \"blue\" offset 0,0"
                      << std::endl;
            }
          else if (mmdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmdev->GetCellId ()
                      << "\" at " << pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps 0.3 lc rgb \"red\" offset 0,0"
                      << std::endl;
            }
        }
    }
}

void
ChangePosition (Ptr<Node> node, Vector vector)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  model->SetPosition (vector);
  NS_LOG_UNCOND ("************************--------------------Change Position-------------------------------*****************");
}

void
ChangeSpeed (Ptr<Node> n, Vector speed)
{
  n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
  NS_LOG_UNCOND ("************************--------------------Change Speed-------------------------------*****************");
}

void
PrintPosition (Ptr<Node> node)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  NS_LOG_UNCOND ("Position +****************************** " << model->GetPosition () << " at time " << Simulator::Now ().GetSeconds ());
}

void
PrintLostUdpPackets (Ptr<UdpServer> app, std::string fileName)
{
  std::ofstream logFile (fileName.c_str (), std::ofstream::app);
  logFile << Simulator::Now ().GetSeconds () << " " << app->GetLost () << std::endl;
  logFile.close ();
  Simulator::Schedule (MilliSeconds (20), &PrintLostUdpPackets, app, fileName);
}


bool
AreOverlapping (Box a, Box b)
{
  return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax) );
}


bool
OverlapWithAnyPrevious (Box box, std::list<Box> m_previousBlocks)
{
  for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it)
    {
      if (AreOverlapping (*it,box))
        {
          return true;
        }
    }
  return false;
}

std::ofstream energyFile;
std::ofstream packettracefile("Packet_Trace_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::trunc);
std::ofstream energyFileBS("energy_consumptionBS_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::trunc);


void
EnergyConsumptionUpdateBS (double totaloldEnergyConsumption, double totalnewEnergyConsumption)
{
  if (!energyFileBS.is_open())
    {
        energyFileBS.open("energy_consumptionBS_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::app);
        if (energyFileBS.is_open())
        {
            energyFileBS << "Time (s),Total Energy Consumption (J),Energy Difference (J)" << std::endl;
        }
        else
        {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }
    }
    Time currentTime = Simulator::Now ();
    //std::cout << currentTime.GetSeconds () << "," << totalnewEnergyConsumption << "," << (totalnewEnergyConsumption-totaloldEnergyConsumption) <<std::endl;
    energyFileBS << currentTime.GetSeconds() << ","
                 << totalnewEnergyConsumption << ","
                 << (totalnewEnergyConsumption - totaloldEnergyConsumption) << std::endl;
    energyofBS = totalnewEnergyConsumption;
  }

void
EnergyConsumptionUpdate (double totaloldEnergyConsumption, double totalnewEnergyConsumption)
{
  if (!energyFile.is_open())
    {
        energyFile.open("energy_consumption_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::app);
        if (energyFile.is_open())
        {
            energyFile << "Time (s),Total Energy Consumption (J),Energy Difference (J)" << std::endl;
        }
        else
        {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }
    }
  Time currentTime = Simulator::Now ();
  //std::cout << currentTime.GetSeconds () << "," << totalnewEnergyConsumption << "," << (totalnewEnergyConsumption-totaloldEnergyConsumption) <<std::endl;
  energyFile << currentTime.GetSeconds() << ","
               << totalnewEnergyConsumption << ","
               << (totalnewEnergyConsumption - totaloldEnergyConsumption) << std::endl;
  energyofUe = totalnewEnergyConsumption;
}

std::pair<Box, std::list<Box> >
GenerateBuildingBounds (double xArea, double yArea, double maxBuildSize, std::list<Box> m_previousBlocks )
{

  Ptr<UniformRandomVariable> xMinBuilding = CreateObject<UniformRandomVariable> ();
  xMinBuilding->SetAttribute ("Min",DoubleValue (40));
  xMinBuilding->SetAttribute ("Max",DoubleValue (xArea));

  NS_LOG_UNCOND ("min " << 0 << " max " << xArea);

  Ptr<UniformRandomVariable> yMinBuilding = CreateObject<UniformRandomVariable> ();
  yMinBuilding->SetAttribute ("Min",DoubleValue (-20));//1.6
  yMinBuilding->SetAttribute ("Max",DoubleValue (yArea));

  NS_LOG_UNCOND ("min " << 0 << " max " << yArea);

  Box box;
  uint32_t attempt = 0;
  do
    {
      NS_ASSERT_MSG (attempt < 100, "Too many failed attempts to position non-overlapping buildings. Maybe area too small or too many buildings?");
      box.xMin = xMinBuilding->GetValue ();

      Ptr<UniformRandomVariable> xMaxBuilding = CreateObject<UniformRandomVariable> ();
      xMaxBuilding->SetAttribute ("Min",DoubleValue (box.xMin));
      xMaxBuilding->SetAttribute ("Max",DoubleValue (box.xMin + maxBuildSize));
      box.xMax = xMaxBuilding->GetValue ();

      box.yMin = yMinBuilding->GetValue ();

      Ptr<UniformRandomVariable> yMaxBuilding = CreateObject<UniformRandomVariable> ();
      yMaxBuilding->SetAttribute ("Min",DoubleValue (box.yMin));
      yMaxBuilding->SetAttribute ("Max",DoubleValue (box.yMin + maxBuildSize));
      box.yMax = yMaxBuilding->GetValue ();

      ++attempt;
    }
  while (OverlapWithAnyPrevious (box, m_previousBlocks));


  NS_LOG_UNCOND ("Building in coordinates (" << box.xMin << " , " << box.yMin << ") and ("  << box.xMax << " , " << box.yMax <<
                 ") accepted after " << attempt << " attempts");
  m_previousBlocks.push_back (box);
  std::pair<Box, std::list<Box> > pairReturn = std::make_pair (box,m_previousBlocks);
  return pairReturn;

}


static ns3::GlobalValue g_mmw1DistFromMainStreet ("mmw1Dist", "Distance from the main street of the first MmWaveEnb",
                                                  ns3::UintegerValue (50), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_mmw2DistFromMainStreet ("mmw2Dist", "Distance from the main street of the second MmWaveEnb",
                                                  ns3::UintegerValue (50), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_mmw3DistFromMainStreet ("mmw3Dist", "Distance from the main street of the third MmWaveEnb",
                                                  ns3::UintegerValue (110), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_mmWaveDistance ("mmWaveDist", "Distance between MmWave eNB 1 and 2",
                                          ns3::UintegerValue (200), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_numBuildingsBetweenMmWaveEnb ("numBlocks", "Number of buildings between MmWave eNB 1 and 2",
                                                        ns3::UintegerValue (10), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_interPckInterval ("interPckInterval", "Interarrival time of UDP packets (us)",
                                            ns3::UintegerValue (20), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_bufferSize ("bufferSize", "RLC tx buffer size (MB)",
                                      ns3::UintegerValue (20), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_x2Latency ("x2Latency", "Latency on X2 interface (us)",
                                     ns3::DoubleValue (500), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_mmeLatency ("mmeLatency", "Latency on MME interface (us)",
                                      ns3::DoubleValue (10000), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_mobileUeSpeed ("mobileSpeed", "The speed of the UE (m/s)",
                                         ns3::DoubleValue (0), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_rlcAmEnabled ("rlcAmEnabled", "If true, use RLC AM, else use RLC UM",
                                        ns3::BooleanValue (true), ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_maxXAxis ("maxXAxis", "The maximum X coordinate for the area in which to deploy the buildings",
                                    ns3::DoubleValue (90), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_maxYAxis ("maxYAxis", "The maximum Y coordinate for the area in which to deploy the buildings",
                                    ns3::DoubleValue (90), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_outPath ("outPath",
                                   "The path of output log files",
                                   ns3::StringValue ("./"), ns3::MakeStringChecker ());
static ns3::GlobalValue g_noiseAndFilter ("noiseAndFilter", "If true, use noisy SINR samples, filtered. If false, just use the SINR measure",
                                          ns3::BooleanValue (false), ns3::MakeBooleanChecker ());
static ns3::GlobalValue g_handoverMode ("handoverMode",
                                        "Handover mode",
                                        ns3::UintegerValue (3), ns3::MakeUintegerChecker<uint8_t> ());
static ns3::GlobalValue g_reportTablePeriodicity ("reportTablePeriodicity", "Periodicity of RTs",
                                                  ns3::UintegerValue (1600), ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_outageThreshold ("outageTh", "Outage threshold",
                                           ns3::DoubleValue (-200), ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_lteUplink ("lteUplink", "If true, always use LTE for uplink signalling",
                                     ns3::BooleanValue (false), ns3::MakeBooleanChecker ());



int
main (int argc, char *argv[])
{
   LogComponentEnable("PacketSink", LOG_INFO);
  // LogComponentEnable("lte-enb-rrc", LOG_INFO);
  bool harqEnabled = true;
  bool fixedTti = false;

  std::list<Box>  m_previousBlocks;

  // Command line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);

  UintegerValue uintegerValue;
  BooleanValue booleanValue;
  StringValue stringValue;
  DoubleValue doubleValue;
  // EnumValue enumValue;
  // GlobalValue::GetValueByName ("numBlocks", uintegerValue);
  // uint32_t numBlocks = uintegerValue.Get ();
  // GlobalValue::GetValueByName ("maxXAxis", doubleValue);
  // double maxXAxis = doubleValue.Get ();
  // GlobalValue::GetValueByName ("maxYAxis", doubleValue);
  // double maxYAxis = doubleValue.Get ();

  // double ueInitialPosition = 78;
  //double ueFinalPosition = 78;

  // Variables for the RT
  int windowForTransient = 150; // number of samples for the vector to use in the filter
  GlobalValue::GetValueByName ("reportTablePeriodicity", uintegerValue);
  int ReportTablePeriodicity = (int)uintegerValue.Get (); // in microseconds
  if (ReportTablePeriodicity == 1600)
    {
      windowForTransient = 150;
    }
  else if (ReportTablePeriodicity == 25600)
    {
      windowForTransient = 50;
    }
  else if (ReportTablePeriodicity == 12800)
    {
      windowForTransient = 100;
    }
  else
    {
      NS_ASSERT_MSG (false, "Unrecognized");
    }

  int vectorTransient = windowForTransient * ReportTablePeriodicity;

  // params for RT, filter, HO mode
  GlobalValue::GetValueByName ("noiseAndFilter", booleanValue);
  bool noiseAndFilter = booleanValue.Get ();
  GlobalValue::GetValueByName ("handoverMode", uintegerValue);
  uint8_t hoMode = uintegerValue.Get ();
  GlobalValue::GetValueByName ("outageTh", doubleValue);
  double outageTh = doubleValue.Get ();

  GlobalValue::GetValueByName ("rlcAmEnabled", booleanValue);
  bool rlcAmEnabled = booleanValue.Get ();
  GlobalValue::GetValueByName ("bufferSize", uintegerValue);
  uint32_t bufferSize = uintegerValue.Get ();
  GlobalValue::GetValueByName ("interPckInterval", uintegerValue);
  uint32_t interPacketInterval = uintegerValue.Get ();
  GlobalValue::GetValueByName ("x2Latency", doubleValue);
  double x2Latency = doubleValue.Get ();
  GlobalValue::GetValueByName ("mmeLatency", doubleValue);
  double mmeLatency = doubleValue.Get ();
  GlobalValue::GetValueByName ("mobileSpeed", doubleValue);
  double ueSpeed = doubleValue.Get ();

  //double transientDuration = double(vectorTransient) / 1000000;
  //double simTime = transientDuration + ((double)ueFinalPosition - (double)ueInitialPosition) / ueSpeed + 1;
  double simTime = 50; //insecs
  NS_LOG_UNCOND ("rlcAmEnabled " << rlcAmEnabled << " bufferSize " << bufferSize << " interPacketInterval " <<
                 interPacketInterval << " x2Latency " << x2Latency << " mmeLatency " << mmeLatency << " mobileSpeed " << ueSpeed);

  GlobalValue::GetValueByName ("outPath", stringValue);
  std::string path = stringValue.Get ();
  std::string mmWaveOutName = "MmWaveSwitchStats";
  std::string lteOutName = "LteSwitchStats";
  std::string dlRlcOutName = "DlRlcStats";
  std::string dlPdcpOutName = "DlPdcpStats";
  std::string ulRlcOutName = "UlRlcStats";
  std::string ulPdcpOutName = "UlPdcpStats";
  std::string  ueHandoverStartOutName =  "UeHandoverStartStats";
  std::string enbHandoverStartOutName = "EnbHandoverStartStats";
  std::string  ueHandoverEndOutName =  "UeHandoverEndStats";
  std::string enbHandoverEndOutName = "EnbHandoverEndStats";
  std::string cellIdInTimeOutName = "CellIdStats";
  std::string cellIdInTimeHandoverOutName = "CellIdStatsHandover";
  std::string mmWaveSinrOutputFilename = "MmWaveSinrTime";
  std::string x2statOutputFilename = "X2Stats";
  std::string udpSentFilename = "UdpSent";
  std::string udpReceivedFilename = "UdpReceived";
  //std::string simulationTypeName = "Building_0_fixedue";
  std::string extension = ".txt";
  std::string version;
  version = "mc";
  Config::SetDefault ("ns3::MmWaveUeMac::UpdateUeSinrEstimatePeriod", DoubleValue (0));

  //get current time
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%d_%m_%Y_%I_%M_%S",timeinfo);
  std::string time_str (buffer);

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue (rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue (fixedTti));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue (6));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue (200.0));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue (100));
  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue (MicroSeconds (100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds (x2Latency)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue (DataRate ("1000Gb/s")));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue (10000));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds (1000)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds (mmeLatency)));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue (MilliSeconds (10.0)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::MmWaveSinrOutputFilename", StringValue("MmWaveSinrTime_0B_Ue6_100MB_3BS.txt"));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::UeHandoverStartOutputFilename", StringValue("Ue_handover_constpos0B_100MB_3BS_Ue6.txt"));
  // handover and RT related params
  switch (hoMode)
    {
    case 1:
      Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue (LteEnbRrc::THRESHOLD));
      break;
    case 2:
      Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue (LteEnbRrc::FIXED_TTT));
      break;
    case 3:
      Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue (LteEnbRrc::DYNAMIC_TTT));
      break;
    }
  // Config::SetDefault ("ns3::LteEnbRrc::MinDynTttValue", DoubleValue (250));
  // Config::SetDefault ("ns3::LteEnbRrc::MaxDynTttValue", DoubleValue (1500));
  // Config::SetDefault ("ns3::LteEnbRrc::MinDiffValue", DoubleValue (300));
  // Config::SetDefault ("ns3::LteEnbRrc::MaxDiffValue", DoubleValue (200));
  Config::SetDefault ("ns3::LteEnbRrc::FixedTttValue", UintegerValue (150));
  Config::SetDefault ("ns3::LteEnbRrc::CrtPeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::LteEnbRrc::OutageThreshold", DoubleValue (outageTh));
  Config::SetDefault ("ns3::MmWaveEnbPhy::UpdateSinrEstimatePeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::MmWaveEnbPhy::Transient", IntegerValue (vectorTransient));
  Config::SetDefault ("ns3::MmWaveEnbPhy::NoiseAndFilter", BooleanValue (noiseAndFilter));

  GlobalValue::GetValueByName ("lteUplink", booleanValue);
  bool lteUplink = booleanValue.Get ();

  Config::SetDefault ("ns3::McUePdcp::LteUplink", BooleanValue (lteUplink));
  std::cout << "Lte uplink " << lteUplink << "\n";

  // settings for the 3GPP the channel
  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue (MilliSeconds (100))); // interval after which the channel for a moving user is updated,
  Config::SetDefault ("ns3::ThreeGppChannelModel::Blockage", BooleanValue (true)); // use blockage or not
  Config::SetDefault ("ns3::ThreeGppChannelModel::PortraitMode", BooleanValue (true)); // use blockage model with UT in portrait mode
  Config::SetDefault ("ns3::ThreeGppChannelModel::NumNonselfBlocking", IntegerValue (4)); // number of non-self blocking obstacles

  // set the number of antennas in the devices
  Config::SetDefault ("ns3::McUeNetDevice::AntennaNum", UintegerValue(16));
  //Config::SetDefault ("ns3::MmWaveNetDevice::AntennaNum", UintegerValue(64));
  
  // set to false to use the 3GPP radiation pattern (proper configuration of the bearing and downtilt angles is needed) 
  //Config::SetDefault ("ns3::ThreeGppAntennaArrayModel::IsotropicElements", BooleanValue (true)); 

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
  mmwaveHelper->SetPathlossModelType ("ns3::ThreeGppUmiStreetCanyonPropagationLossModel");
  mmwaveHelper->SetChannelConditionModelType ("ns3::BuildingsChannelConditionModel");

  Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
  mmwaveHelper->SetHarqEnabled (harqEnabled);
  mmwaveHelper->Initialize ();
  static Ptr<ThreeGppUmiStreetCanyonPropagationLossModel> lossModel = CreateObject<ThreeGppUmiStreetCanyonPropagationLossModel>();
  lossModel->TraceConnectWithoutContext("ShadowingStd", MakeCallback(&DoubleShadowingStd));
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  // Get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet by connecting remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");         
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // create LTE, mmWave eNB nodes and UE node
  NodeContainer ueNodes;
  NodeContainer mmWaveEnbNodes;
  NodeContainer lteEnbNodes;
  NodeContainer allEnbNodes;
  mmWaveEnbNodes.Create (3);
  lteEnbNodes.Create (1);
  ueNodes.Create (1);
  allEnbNodes.Add (lteEnbNodes);
  allEnbNodes.Add (mmWaveEnbNodes);
  
  
  // Positions
  Vector mmw1Position = Vector (-150, 100, 3);
  Vector mmw2Position = Vector (50, 100, 3);
  Vector mmw3Position = Vector (250, 100, 3);

  // std::vector<Ptr<Building> > buildingVector;
  // double maxBuildingSize = 30;

  // for (uint32_t buildingIndex = 0; buildingIndex < numBlocks; buildingIndex++)
  //   {
  //     Ptr < Building > building;
  //     building = Create<Building> ();
      /* returns a vecotr where:
      * position [0]: coordinates for x min
      * position [1]: coordinates for x max
      * position [2]: coordinates for y min
      * position [3]: coordinates for y max
      */
  //     std::pair<Box, std::list<Box> > pairBuildings = GenerateBuildingBounds (maxXAxis, maxYAxis, maxBuildingSize, m_previousBlocks);
  //     m_previousBlocks = std::get<1> (pairBuildings);
  //     Box box = std::get<0> (pairBuildings);
  //     Ptr<UniformRandomVariable> randomBuildingZ = CreateObject<UniformRandomVariable> ();
  //     randomBuildingZ->SetAttribute ("Min",DoubleValue (-50));
  //     randomBuildingZ->SetAttribute ("Max",DoubleValue (40));
  //     double buildingHeight = randomBuildingZ->GetValue ();

  //     building->SetBoundaries (Box (box.xMin, box.xMax,
  //                                   box.yMin,  box.yMax,
  //                                   0.0, buildingHeight));
  //     buildingVector.push_back (building);
  //  }


  // building1->SetBoundaries (Box (45, 60,
  //                               60,  90,
  //                               0.0, 10));
  // building1->SetNFloors (1);
  // building1->SetNRoomsX (1);
  // building1->SetNRoomsY (1) ;     
  // buildingVector.push_back (building1);

  // Ptr < Building > building2;
  // building2 = Create<Building> ();

  // building2->SetBoundaries (Box (45, 60,
  //                               30,  45,
  //                               0.0, 20)); 
  // building2->SetNFloors (1);
  // building2->SetNRoomsX (1);
  // building2->SetNRoomsY (1) ;  
  // buildingVector.push_back (building2);

  // Ptr < Building > building3;
  // building3 = Create<Building> ();

  // building3->SetBoundaries (Box (45, 60,
  //                               5,  20,
  //                               0.0, 20)); 
  // building3->SetNFloors (1);
  // building3->SetNRoomsX (1);
  // building3->SetNRoomsY (1) ;  
  // buildingVector.push_back (building3);

  // Ptr < Building > building4;
  // building4 = Create<Building> ();

  // building4->SetBoundaries (Box (45, 60,
  //                               -20,  -5,
  //                               0.0, 20)); 

  // building4->SetNFloors (1);
  // building4->SetNRoomsX (1);
  // building4->SetNRoomsY (1) ;  
  // buildingVector.push_back (building4);

  // Ptr < Building > building5;
  // building5 = Create<Building> ();

  // building5->SetBoundaries (Box (45, 60,
  //                               -45, -30,
  //                               0.0, 20)); 
  // building5->SetNFloors (1);
  // building5->SetNRoomsX (1);
  // building5->SetNRoomsY (1) ;  
  // buildingVector.push_back (building5);

  // Ptr < Building > building6;
  // building6 = Create<Building> ();

  // building6->SetBoundaries (Box (45, 60,
  //                               -70, -55,
  //                               0.0, 20)); 
  // building6->SetNFloors (1);
  // building6->SetNRoomsX (1);
  // building6->SetNRoomsY (1) ;  
  // buildingVector.push_back (building6);

  // Ptr < Building > building7;
  // building7 = Create<Building> ();

  // building7->SetBoundaries (Box (70, 90,
  //                               50,  85,
  //                               0.0, 20)); 
  // building7->SetNFloors (1);
  // building7->SetNRoomsX (1);
  // building7->SetNRoomsY (1) ;  
  // buildingVector.push_back (building7);

  // Ptr < Building > building8;
  // building8 = Create<Building> ();

  // building8->SetBoundaries (Box (70, 90,
  //                               20,  40,
  //                               0.0, 20)); 
  // building8->SetNFloors (1);
  // building8->SetNRoomsX (1);
  // building8->SetNRoomsY (1) ;  
  // buildingVector.push_back (building8);

  // Ptr < Building > building9;
  // building9 = Create<Building> ();

  // building9->SetBoundaries (Box (20,40,
  //                               60,  90,
  //                               0.0, 20)); 

  // building9->SetNFloors (1);
  // building9->SetNRoomsX (1);
  // building9->SetNRoomsY (1) ;  
  // buildingVector.push_back (building9);

  // Ptr < Building > building10;
  // building10 = Create<Building> ();

  // building10->SetBoundaries (Box (20, 40,
  //                               30, 50,
  //                               0.0, 20)); 

  // building10->SetNFloors (1);
  // building10->SetNRoomsX (1);
  // building10->SetNRoomsY (1) ;  
  // buildingVector.push_back (building10);


  // Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  //enbPositionAlloc->Add (Vector ((double)mmWaveDist/2 + streetWidth, mmw1Dist + 2*streetWidth, mmWaveZ));
  enbPositionAlloc->Add (mmw1Position); // LTE BS, out of area where buildings are deployed
  enbPositionAlloc->Add (mmw1Position);
  enbPositionAlloc->Add (mmw2Position);
  enbPositionAlloc->Add (mmw3Position);
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (allEnbNodes);
  BuildingsHelper::Install (allEnbNodes);

  MobilityHelper uemobility;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  //uePositionAlloc->Add (Vector (ueInitialPosition, -5, 0));
  uePositionAlloc->Add (Vector (100, -75, 1.6));
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetPositionAllocator (uePositionAlloc);
  uemobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);
  
  //ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition, -5, 0));
  // ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition, -5, 1.6));
  //ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

  // Install mmWave, lte, mc Devices to the nodes
  NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice (mmWaveEnbNodes);
  NetDeviceContainer mcUeDevs;
  mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mcUeDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Add X2 interfaces
  mmwaveHelper->AddX2Interface (lteEnbNodes, mmWaveEnbNodes);

  // Manual attachment
  mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs, lteEnbDevs);

  // Energy Framework
  BasicEnergySourceHelper basicSourceHelper;
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000000));
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (5.0));
  // Install Energy Source
  EnergySourceContainer sources = basicSourceHelper.Install (ueNodes.Get (0));
  EnergySourceContainer Enb_sources = basicSourceHelper.Install (mmWaveEnbNodes);
  // Device Energy Model
  MmWaveRadioEnergyModelHelper nrEnergyHelper;
  MmWaveRadioEnergyModelEnbHelper enbEnergyHelper;
  DeviceEnergyModelContainer deviceEnergyModel = nrEnergyHelper.Install (mcUeDevs, sources);
  DeviceEnergyModelContainer bsEnergyModel = enbEnergyHelper.Install (mmWaveEnbDevs, Enb_sources);
  deviceEnergyModel.Get(0)->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&EnergyConsumptionUpdate));
  bsEnergyModel.Get(0)->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&EnergyConsumptionUpdateBS));
 

uint16_t servPort = 50000;

// Create a packet sink to receive these packets on n2...
PacketSinkHelper sink ("ns3::TcpSocketFactory",
                       InetSocketAddress (Ipv4Address::GetAny (), servPort));
//sink.SetAttribute ("PacketWindowSize", UintegerValue (256));
ApplicationContainer apps = sink.Install (ueNodes.Get (0));
apps.Start (Seconds (0.0));
apps.Stop (Seconds (simTime));


// Create a source to send packets from n0.  Instead of a full Application
// and the helper APIs you might see in other example files, this example
// will use sockets directly and register some socket callbacks as a sending
// "Application".

// Create and bind the socket...
Ptr<Socket> localSocket =
  Socket::CreateSocket (remoteHostContainer.Get (0), TcpSocketFactory::GetTypeId ());
localSocket->Bind ();

// Trace changes to the congestion window
// Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));

// ...and schedule the sending "Application"; This is similar to what an 
// ns3::Application subclass would do internally.
Simulator::ScheduleNow (&StartFlow, localSocket,
                        ueIpIface.GetAddress (0), servPort);
// Simulator::Schedule(Seconds(8.0), &Socket::Close, localSocket); // Stop sending at 8s

// Simulator::Schedule(Seconds(8.0), []() {
//    std::cout << "[INFO] Entering idle state at time: 8.0 s" << std::endl;
// });
// One can toggle the comment for the following line on or off to see the
// effects of finite send buffer modelling.  One can also change the size of
// said buffer.

//localSocket->SetAttribute("SndBufSize", UintegerValue(4096));

//Ask for ASCII and pcap traces of network traffic
//AsciiTraceHelper ascii;
// p2ph.EnableAsciiAll (ascii.CreateFileStream ("tcp-large-transfer.tr"));
// p2ph.EnablePcapAll ("tcp-large-transfer");
mmwaveHelper->EnableTraces ();
Ptr<Application> sinkApp = apps.Get(0);
Ptr<PacketSink> sinkChecker = DynamicCast<PacketSink>(sinkApp);
sinkChecker->TraceConnectWithoutContext("TotalBytesReceived",MakeCallback(&ReceivedPacket));
// Finally, set up the simulator to run.  The 1000 second hard limit is a
// failsafe in case some change above causes the simulation to never end
Simulator::Stop (Seconds (simTime));
Simulator::Run ();
Simulator::Destroy ();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//begin implementation of sending "Application"
void StartFlow (Ptr<Socket> localSocket,
              Ipv4Address servAddress,
              uint16_t servPort)
{
NS_LOG_LOGIC ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

// tell the tcp implementation to call WriteUntilBufferFull again
// if we blocked and new tx buffer space becomes available
localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) 
  {
    if (!packettracefile.is_open())
    {
        packettracefile.open("Packet_Trace_0B_Ue6_100MB_3BS.csv", std::ios::out | std::ios::app);
        if (packettracefile.is_open())
        {
          packettracefile << "Time (s),currentTxBytes, left, dataOffset, toWrite, amountSent" << std::endl;
        }
        else
        {
            std::cerr << "Error opening file for writing!" << std::endl;
            return;
        }
    }
    uint32_t left = totalTxBytes - currentTxBytes;
    uint32_t dataOffset = currentTxBytes % writeSize;
    uint32_t toWrite = writeSize - dataOffset;
    toWrite = std::min (toWrite, left);
    toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
    int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
    packettracefile << Simulator:: Now().GetSeconds()<< ","<< currentTxBytes<< "," << left<< "," << dataOffset<<","<< toWrite<< ","<< amountSent<< std::endl;
    
    if(amountSent < 0)
      {
        // we will be called again when new tx space becomes available.
        return;
      }
    currentTxBytes += amountSent;
  }
if (currentTxBytes >= totalTxBytes)
  {
    localSocket->Close (); std::cout <<"Application Ending "<< Simulator ::Now().GetSeconds()<< std::endl;
}
}