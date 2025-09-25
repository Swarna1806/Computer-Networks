#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CsmaCdSimulation");

// Global variables for data collection
std::map<std::string, std::ofstream> outputFiles;
Time binInterval = Seconds(0.1);
uint32_t totalCollisions = 0;
Time lastCollisionTime = Seconds(0);
Time collisionDebounceTime = MicroSeconds(100);

void CollisionCallback(std::string context, Ptr<const Packet> packet) {
    Time now = Simulator::Now();
    if (now - lastCollisionTime >= collisionDebounceTime) {
        totalCollisions++;
        lastCollisionTime = now;
        outputFiles["collisions"] << now.GetSeconds() << " " << totalCollisions << std::endl;
    }
}

void CollectStatistics(Ptr<FlowMonitor> flowMonitor, Ptr<NetDevice> device) {
    static std::map<uint32_t, uint64_t> lastTxBytes;
    static std::map<uint32_t, uint64_t> lastRxBytes;

    Time now = Simulator::Now();
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    uint64_t totalTxBytes = 0;
    uint64_t totalRxBytes = 0;
    double totalDelay = 0;
    double totalLatency = 0;
    uint32_t totalPackets = 0;
    
    for (auto &stat : stats) {
        totalTxBytes += stat.second.txBytes;
        totalRxBytes += stat.second.rxBytes;
        totalDelay += stat.second.delaySum.GetSeconds();
        double latency = stat.second.timeLastRxPacket.GetSeconds() - stat.second.timeFirstTxPacket.GetSeconds();
        if (latency >= 0) {
            totalLatency += latency;
        }
        totalPackets += stat.second.rxPackets;
    }

    // Calculate metrics
    double throughput = (totalRxBytes - lastRxBytes[0]) * 8.0 / binInterval.GetSeconds();
    double packetLoss = ((totalTxBytes - totalRxBytes) * 100.0) / (totalTxBytes > 0 ? totalTxBytes : 1);
    double avgDelay = totalPackets > 0 ? (totalDelay / totalPackets) * 1000 : 0; // in ms
    double avgLatency = totalPackets > 0 ? (totalLatency / totalPackets) * 1000 : 0; // in ms

    // Write metrics to files
    outputFiles["throughput"] << now.GetSeconds() << " " << throughput / 1000000.0 << std::endl; // Mbps
    outputFiles["packet_loss"] << now.GetSeconds() << " " << packetLoss << std::endl; // %
    outputFiles["delay"] << now.GetSeconds() << " " << avgDelay << std::endl; // ms
    outputFiles["latency"] << now.GetSeconds() << " " << avgLatency << std::endl; // ms

    // Update last values
    lastTxBytes[0] = totalTxBytes;
    lastRxBytes[0] = totalRxBytes;

    // Schedule next collection
    Simulator::Schedule(binInterval, &CollectStatistics, flowMonitor, device);
}

int main(int argc, char *argv[]) {
    uint32_t nNodes = 5;
    uint32_t simTime = 60;
    uint32_t dataRate = 25; // Mbps
    uint32_t packetSize = 1460;
    double channelDataRate = 100.0; // Mbps

    // Custom command-line parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--nNodes" && i + 1 < argc) {
            nNodes = std::stoi(argv[++i]);
        } else if (arg == "--simTime" && i + 1 < argc) {
            simTime = std::stoi(argv[++i]);
        } else if (arg == "--dataRate" && i + 1 < argc) {
            dataRate = std::stoi(argv[++i]);
        } else if (arg == "--packetSize" && i + 1 < argc) {
            packetSize = std::stoi(argv[++i]);
        }
    }

    // Create output files
    outputFiles["throughput"].open("throughput.dat");
    outputFiles["packet_loss"].open("packet_loss.dat");
    outputFiles["delay"].open("delay.dat");
    outputFiles["latency"].open("latency.dat");
    outputFiles["collisions"].open("collisions.dat");

    // Enable logging
    LogComponentEnable("CsmaCdSimulation", LOG_LEVEL_INFO);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(nNodes);

    // Configure CSMA channel
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(channelDataRate * 1000000)));
    csma.SetChannelAttribute("Delay", TimeValue(MicroSeconds(100)));

    // Create devices
    NetDeviceContainer devices = csma.Install(nodes);

    // Connect collision detection
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTxBackoff", MakeCallback(&CollisionCallback));

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create applications
    uint16_t port = 9;

    // Install packet sinks
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(simTime));

    // Create OnOff applications
    OnOffHelper onoff("ns3::UdpSocketFactory", Address());
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRate * 1000000)));
    onoff.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer apps;
    for (uint32_t i = 0; i < nodes.GetN(); i++) {
        for (uint32_t j = 0; j < nodes.GetN(); j++) {
            if (i != j) {
                onoff.SetAttribute("Remote", AddressValue(InetSocketAddress(interfaces.GetAddress(j), port)));
                apps.Add(onoff.Install(nodes.Get(i)));
            }
        }
    }

    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(simTime));

    // Install FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Schedule statistics collection
    Simulator::Schedule(Seconds(0.0), &CollectStatistics, monitor, devices.Get(0));

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Print final statistics
    monitor->CheckForLostPackets();
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::cout << "\n=== Simulation Summary ===\n";
    std::cout << "Total Collisions: " << totalCollisions << "\n";
    std::cout << "Average Collision Rate: " << (totalCollisions * 1.0 / simTime)
              << " collisions/s\n\n";

    // Close output files
    for (auto &file : outputFiles) {
        file.second.close();
    }

    Simulator::Destroy();
    return 0;
}

