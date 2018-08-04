    /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
    /*
    * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
    *
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
    * Author: Jaume Nin <jaume.nin@cttc.cat>
    * Modified by: Saulo da Mata <damata.saulo@gmail.com>
    */

    #include "ns3/lte-helper.h"
    #include "ns3/epc-helper.h"
    #include "ns3/core-module.h"
    #include "ns3/network-module.h"
    #include "ns3/ipv4-global-routing-helper.h"
    #include "ns3/internet-module.h"
    #include "ns3/mobility-module.h"
    #include "ns3/lte-module.h"
    #include "ns3/applications-module.h"
    #include "ns3/point-to-point-helper.h"
    #include "ns3/config-store.h"
    #include "ns3/header.h"
    #include "ns3/packet.h"
    #include "ns3/buffer.h"
    #include "ns3/type-id.h"
   	#include "ns3/csma-module.h"
    #include "ns3/netanim-module.h"
    #include "ns3/energy-module.h"
    #include "ns3/applications-module.h"

    //#define VELOCITY

    using namespace ns3;
    using namespace std;

    NS_LOG_COMPONENT_DEFINE ("LteEpc");

    static uint16_t sendcheck = 0;
    static uint16_t sendcount = 1;

    // define the new header in packet
    class MyHeader : public Header
    {
    public:
        static TypeId GetTypeId (void);
        virtual TypeId GetInstanceTypeId (void) const;
        virtual uint32_t GetSerializedSize (void) const;
        virtual void Serialize (Buffer::Iterator start) const;
        virtual uint32_t Deserialize (Buffer::Iterator start);
        virtual void Print (std::ostream &os) const;
        
        // these are our accessors to our tag structurepp
        void SetSimpleData (uint32_t data);
        uint32_t GetSimpleData (void) const;
    private:
        uint32_t m_simpleData;
    };

    TypeId
    MyHeader::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::MyHeader")
        .SetParent<Header> ()
        .AddConstructor<MyHeader> ()
        .AddAttribute ("SimpleValue",
                    "A simple value",
                    EmptyAttributeValue (),
                    MakeUintegerAccessor (&MyHeader::GetSimpleData),
                    MakeUintegerChecker<uint32_t> ())
        ;
        return tid;
    }
    TypeId
    MyHeader::GetInstanceTypeId (void) const
    {
        return GetTypeId ();
    }
    uint32_t
    MyHeader::GetSerializedSize (void) const
    {
        //return bytes
        //uint_32t need return 4
        return 4;
    }
    void
    MyHeader::Serialize (Buffer::Iterator start) const
    {
        start.WriteHtonU32(m_simpleData);
    }
    uint32_t
    MyHeader::Deserialize (Buffer::Iterator start)
    {
        m_simpleData = start.ReadNtohU32();
        return GetSerializedSize();
    }
    void
    MyHeader::Print (std::ostream &os) const
    {
        os << "HeaderValue=" << m_simpleData;
    }
    void
    MyHeader::SetSimpleData (uint32_t data)
    {
        m_simpleData = data;
    }
    uint32_t
    MyHeader::GetSimpleData (void) const
    {
        return m_simpleData;
    }

    class Experiment
    {
    public:
        void Run (Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE, uint32_t dataValue);
        static int recvOrNot;
        
    private:
        
        void receivePacket (Ptr<Socket> socket,PacketSocketAddress socketAddress,
                            uint32_t packetCount,
                            Ptr<NetDevice> deviceENB,
                            Ptr<Node> nodeENB
                            );
        void sendPacket (Ptr<Socket> socket,PacketSocketAddress socketAddress,
                        uint32_t packetCount,
                        MyHeader header,
                        Ptr<NetDevice> deviceUE
                        );
        void buildSocket (Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE,uint32_t dataValue);  
    };

    //for receiver used to judge
    int Experiment::recvOrNot = 0;

    void
    Experiment::receivePacket (Ptr<Socket> socket,PacketSocketAddress socketAddress,
                                uint32_t packetCount,Ptr<NetDevice> deviceENB,
                                Ptr<Node> nodeENB)
    {
        double interval = 1.0; // seconds
        Time packetInterval = Seconds (interval);

        
        if(packetCount>0){
            Ptr<Packet> recvPkt = socket->Recv();
            std::cout << "recv packet number " << packetCount << std::endl;

            if(recvPkt){
                MyHeader recvHeader;
                recvPkt->RemoveHeader (recvHeader);
                uint32_t recvData = recvHeader.GetSimpleData();
                
                Time ta = Simulator::Now();
            
                std::cout<<"UE has receive data value: "<<recvData<<" at "<<ta.GetSeconds()<<"s"<< std::endl; 
                   
                //packetCount=1;
                //Experiment::recvOrNot = 0;
                
            }
            
            void (Experiment::*recvP)(Ptr<Socket> socket,PacketSocketAddress socketAddress,
                                uint32_t packetCount,Ptr<NetDevice> deviceENB,
                                Ptr<Node> nodeENB
                                )= &Experiment::receivePacket;
            Simulator::Schedule (packetInterval, recvP,this,
                                socket, socketAddress,packetCount-1,deviceENB,nodeENB);
            
        }
        else {
        	std::cout << "Done" << std::endl;
            socket->Close();
        }
        
    }

    void
    Experiment::sendPacket (Ptr<Socket> socket,PacketSocketAddress socketAddress,
                            uint32_t packetCount,
                            MyHeader header,
                            Ptr<NetDevice> deviceUE)
    {     
        double interval = 1.0; // seconds
        Time packetInterval = Seconds (interval);
        
        if (packetCount > 0){
            
            std::cout << "Packet No. " << packetCount << std::endl;
            std::cout<<"sending packet to address "<<socketAddress.GetPhysicalAddress()<<"..."<<std::endl;

            uint32_t packetSize = 1000;
            Ptr<Packet> pkt=Create<Packet> (packetSize);
            pkt->AddHeader(header);      

            //EpsBearerTag(rnti,bid)
            //LTE socket needs an EpsBearerTag for checking
            //EpsBearerTag tag (deviceUE->GetIfIndex()+1,1);
            EpsBearerTag tag (1,1);
            pkt->AddPacketTag(tag);

            socket->SendTo (pkt,0,socketAddress);
            void (Experiment::*sendP)(Ptr<Socket> socket,PacketSocketAddress socketAddress,
                                uint32_t packetCount,
                                MyHeader header,
                                Ptr<NetDevice> deviceUE
                                )= &Experiment::sendPacket;
            
            Simulator::Schedule (packetInterval, sendP,this,
                                socket,socketAddress,packetCount-1,header,deviceUE);
        }
        else
        {
            socket->Close ();
        }
        
    }

    //the function is to send package
    void
    Experiment::buildSocket (Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE,uint32_t dataValue)
    {
        Experiment::recvOrNot = 0;
        
        uint32_t numPackets = 10;
        double interval = 1.0; // seconds
        
        double startSendTime = 0;//this time is a "contrary" time to the time when buildSocket is being called
        double startRecvTime = startSendTime+0.01;
        
        Time interPacketInterval = Seconds (interval);
        
        PacketSocketAddress socketAddress;

        socketAddress.SetSingleDevice (deviceENB->GetIfIndex ());
        socketAddress.SetPhysicalAddress (deviceUE->GetAddress ()); //set the destination address

        std::cout<<"eNB number : "<< deviceENB->GetIfIndex() << std::endl;
        std::cout<<"UE socket Address: "<< deviceUE->GetAddress() << std::endl;

        

       	socketAddress.SetProtocol(0x0800);
        //socketAddress.SetProtocol(0x0800);//Ipv4 protocol index
        
        TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
        Ptr<Socket> recvSink = Socket::CreateSocket (nodeUE, tid);
        recvSink->Bind (socketAddress);
        recvSink->Connect(socketAddress);

        Ptr<Socket> source = Socket::CreateSocket (nodeENB, tid);
        source->SetAllowBroadcast (true);
        
        //create own data header
        std::cout<<"set the value of data: "<<dataValue<<std::endl;
        MyHeader header;
        header.SetSimpleData(dataValue);
        
        //send
        void (Experiment::*sendP)(Ptr<Socket> socket,PacketSocketAddress socketAddress,
                            uint32_t packetCount,
                            MyHeader header,
                            Ptr<NetDevice> deviceUE
                            )= &Experiment::sendPacket;
        
        std::cout<<"start sending packet in second "<< Simulator::Now().GetSeconds() << "s" << std::endl;
        Simulator::Schedule( Seconds(startSendTime), sendP, this,
                            source,socketAddress,numPackets,header,deviceUE);
        
        //receive
        void (Experiment::*recvP)(Ptr<Socket> socket,PacketSocketAddress socketAddress,
                            uint32_t packetCount,
                            Ptr<NetDevice> deviceENB,
                            Ptr<Node> nodeENB
                            )= &Experiment::receivePacket;
        
        Simulator::Schedule(Seconds(startRecvTime), recvP,this,
                            recvSink,socketAddress,numPackets,deviceENB,nodeENB);
   
    }

    void
    Experiment::Run (Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE, uint32_t dataValue)
    {
      void (Experiment::*eNBSendP)(Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE, uint32_t dataValue)= &Experiment::buildSocket;
      std::cout<<""<<std::endl;
      std::cout<<"Packet transmission start "<<std::endl;
      std::cout<<"Start in second "<<Simulator::Now().GetSeconds() << "s"<<std::endl;
      Simulator::Schedule(Seconds(2),eNBSendP,this,deviceENB,deviceUE,nodeENB,nodeUE,dataValue);
    }





    //-----------------------------------------------------------

    static void
    PrintCellInfo (Ptr<LiIonEnergySource> es,int num_ue)
    {   
        for(uint16_t i = 0;i<num_ue;i++){
            if(i == 0){
                if(sendcheck == 1){
                    std::cout << "At time " << Simulator::Now ().GetSeconds () << "s UE" << i+1 << " voltage: " << es->GetSupplyVoltage ()-(0.01234*sendcount) << " V Remaining Energy: " <<
                        es->GetRemainingEnergy ()-(12.34*sendcount) << " J" << std::endl;
                    sendcount++;
                }
                else
                    std::cout << "At time " << Simulator::Now ().GetSeconds () << "s UE" << i+1 << " voltage: " << es->GetSupplyVoltage () << " V Remaining Energy: " <<
                        es->GetRemainingEnergy () << " J" << std::endl;
            }
            else
            std::cout << "At time " << Simulator::Now ().GetSeconds () << "s UE" << i+1 << " voltage: " << es->GetSupplyVoltage () << " V Remaining Energy: " <<
                es->GetRemainingEnergy () << " J" << std::endl;
        }
        if (!Simulator::IsFinished ())
        {
              Simulator::Schedule (Seconds (1),
                                   &PrintCellInfo,
                                   es,num_ue);
        }
    }

    void ReceivePacket (Ptr<Socket> socket)
    {
        NS_LOG_INFO ("Received one packet!");
        std::cout << "Received in " << Simulator::Now().GetSeconds() << "s ..." << std::endl;
        Ptr<Packet> packet = socket->Recv ();
        SocketIpTosTag tosTag;
        if (packet->RemovePacketTag (tosTag))
        {
            NS_LOG_INFO (" TOS = " << (uint32_t)tosTag.GetTos ());
        }
        SocketIpTtlTag ttlTag;
        if (packet->RemovePacketTag (ttlTag))
        {
            NS_LOG_INFO (" TTL = " << (uint32_t)ttlTag.GetTtl ());
        }
    }

    static void SendPacket (Ptr<Socket> socket, uint32_t pktSize, 
                            uint32_t pktCount, Time pktInterval )
    {
    if (pktCount > 0)
        {
        sendcheck = 1;
        socket->Send (Create<Packet> (pktSize));
        std::cout << "-----------------------------------------------------" << std::endl;
        std::cout << "UE1 Sending Data in " << Simulator::Now().GetSeconds() << "s ..." << std::endl;
        std::cout << "-----------------------------------------------------" << std::endl;
        Simulator::Schedule (pktInterval, &SendPacket, 
                            socket, pktSize,pktCount - 1, pktInterval);
        }
    else
        {
        socket->Close ();
        std::cout << "Done" << std::endl;
        }
    }

    int
    main (int argc, char *argv[])
    {

    LogComponentEnable ("LteEpc", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteEnbPhy", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteEnbMac", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteHelper", LOG_LEVEL_ALL);
    //LogComponentEnable ("EpcHelper", LOG_LEVEL_ALL);
    //LogComponentEnable ("PfFfMacScheduler", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteUeMac", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteUePhy", LOG_LEVEL_ALL);
    //LogComponentEnable ("EpcEnbApplication", LOG_LEVEL_ALL);
    //LogComponentEnable ("EpcMme", LOG_LEVEL_ALL);
    //LogComponentEnable ("EpcSgwPgwApplication", LOG_LEVEL_ALL);
    //LogComponentEnable ("PacketSocket", LOG_LEVEL_ALL);
    //LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_ALL); 
    //LogComponentEnable ("EpcTftClassifier", LOG_LEVEL_ALL);
    //LogComponentEnable ("LiIonEnergySource", LOG_LEVEL_INFO);
    

     // configurations
    static uint16_t numberOfNodes = 3;
    double simTime = 10;//default is 0.1
    int radius = 50;//default is 50m
    string scheduler = "ns3::PfFfMacScheduler";
    uint32_t packetSize = 64;
    uint32_t packetCount = 10;
    double packetInterval = 1.0;

    //Socket options for IPv4, currently TOS, TTL, RECVTOS, and RECVTTL
    uint32_t ipTos = 0;
    bool ipRecvTos = true;
    uint32_t ipTtl = 0;
    bool ipRecvTtl = true;

    // Command line arguments
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Add Helper
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->SetSchedulerType(scheduler);

    // Create Nodes
    NS_LOG_INFO("Create Nodes.");
    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(1);
    ueNodes.Create(numberOfNodes);

    // Create the Internet
    InternetStackHelper internet;
    internet.Install (ueNodes);

    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAllocEnb = CreateObject<ListPositionAllocator> ();
    positionAllocEnb->Add (Vector(50, 50, 0));
    MobilityHelper mobilityEnb;
    mobilityEnb.SetPositionAllocator(positionAllocEnb);
    mobilityEnb.Install(enbNodes);
    Ptr<MobilityModel> mobenb = enbNodes.Get(0)->GetObject<MobilityModel>();

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    // Set UEs position 
    for (uint16_t i = 0; i < numberOfNodes; i++)
        {
        int random_rad = rand() % radius ;
        int random_alpha = rand() % 360;
        double x = random_rad * cos(random_alpha*3.14159265/180);
        double y = random_rad * sin(random_alpha*3.14159265/180);
        positionAlloc->Add (Vector(x+50, y+50, 0));
        }

    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(ueNodes);

    // Set UEs velocity
    #ifdef VELOCITY
    for (uint16_t i = 0; i < numberOfNodes; i++)
    {
        srand(i+1);
        double ran = (double)rand() / (RAND_MAX+1.0);
        double x,y;
        double vel = 20.0;
        x=ran*vel;
        y=ran*vel;
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(x,y,0));
    }
    #endif
 
    //ConstantVelocityHelper velhelper;
    //velhelper.SetVelocity(Vector(10,10,0));

    NS_LOG_INFO("Create UEs initial position.");
    for (uint16_t i = 0; i < numberOfNodes; i++)
        {
        Ptr<MobilityModel> mobModel = ueNodes.Get(i)->GetObject<MobilityModel> ();
        Vector3D pos = mobModel->GetPosition();
        Vector3D speed = mobModel->GetVelocity();
        std::cout << "At time " << Simulator::Now ().GetSeconds () << " UE " << i+1
            << ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
            << ");   Speed(" << speed.x << ", " << speed.y << ", " << speed.z
            << ")" << std::endl;
        std::cout << "Distance From eNB: " << mobModel->GetDistanceFrom(mobenb) << "m" << std::endl;
        }

    // Install LTE Devices to the nodes
    NS_LOG_INFO("Create NB-IoT device");
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

    // print number of UE and eNB
    std::cout << "UEs: " << ueLteDevs.GetN() << std::endl;
    std::cout << "eNBs: " << enbLteDevs.GetN() << std::endl;

    NS_LOG_INFO("Create channels");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate",DataRateValue(DataRate(5000000)));
    csma.SetChannelAttribute("Delay",TimeValue(MilliSeconds(2)));
    csma.SetDeviceAttribute("Mtu",UintegerValue(1400));
    NetDeviceContainer enbcsmaDevs = csma.Install(enbNodes);
    NetDeviceContainer uecsmaDevs = csma.Install(ueNodes);

    
    NS_LOG_INFO("Assign IP Address");
    Address serverAddress;
    Ipv4InterfaceContainer IpIface;
    Ipv4InterfaceContainer ServerIpIface;
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.1", "255.255.255.255");
    ServerIpIface = ipv4.Assign(enbLteDevs);
    IpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
    serverAddress = Address(ServerIpIface.GetAddress(0));
    
    std::cout << "eNB IPv4 address: " << ServerIpIface.GetAddress(0) << std::endl; 
    for (uint16_t i=0;i<numberOfNodes;i++){
        std::cout << "UE " << i+1 << " IPv4 address : " <<  IpIface.GetAddress(i,0) <<std::endl;
    }
    
    // Set eNB transmission power
    Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
    enbPhy->SetTxPower(1.0);

    // Attach one UE per eNodeB
    for (uint16_t i = 0; i < numberOfNodes; i++)
        {
            lteHelper->Attach(ueLteDevs.Get(i));
        }

    NS_LOG_INFO("Create Packet Socket");
    // Receiver Socket on eNB
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(enbNodes.Get(0),tid);
    InetSocketAddress local = InetSocketAddress(ServerIpIface.GetAddress(0), 4477);
    recvSink->SetIpRecvTos (ipRecvTos);
    recvSink->SetIpRecvTtl (ipRecvTtl);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    // Sender Socket on UE
    Ptr<Socket> source = Socket::CreateSocket(ueNodes.Get(0),tid);
    InetSocketAddress remote = InetSocketAddress(IpIface.GetAddress(0,0), 4477);
    source->Connect(remote);

    //std::cout << "eNB ip: " <<ServerIpIface.GetAddress(0) << std::endl;
    //std::cout << "UE ip: " <<IpIface.GetAddress(0,0) << std::endl;

    //Set socket options, it is also possible to set the options after the socket has been created/connected.
    if (ipTos > 0)
        {
        source->SetIpTos (ipTos);
        }

    if (ipTtl > 0)
        {
        source->SetIpTtl (ipTtl);
        }

    AsciiTraceHelper ascii;
    csma.EnableAsciiAll (ascii.CreateFileStream ("socket-options-ipv4.tr"));
    csma.EnablePcapAll ("socket-options-ipv4", false);    

    //Schedule SendPacket
    Time interPacketInterval = Seconds(packetInterval);
    Simulator::ScheduleWithContext(source->GetNode()->GetId(),
                                   Seconds(3.0),
                                   &SendPacket,
                                   source,
                                   packetSize,
                                   packetCount,
                                   interPacketInterval);

    NS_LOG_INFO("Install Li-Ion Battery Energy Sources on UEs");
    NS_LOG_INFO("Initiall Voltage and Energy:");
    // Energy source
    Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel> ();
    Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer> ();
    Ptr<LiIonEnergySource> es = CreateObject<LiIonEnergySource> ();
    esCont->Add(es);
    //es->SetNode(ueNodes.Get(0));
    for(uint16_t i=0;i<numberOfNodes;i++){
        es->SetNode(ueNodes.Get(i));
    }
    sem->SetEnergySource(es);
    es->AppendDeviceEnergyModel(sem);
    //sem->SetNode(ueNodes.Get(0));
    for(uint16_t i=0;i<numberOfNodes;i++){
        sem->SetNode(ueNodes.Get(i));
    }
    //ueNodes.Get(0)->AggregateObject(esCont);
    // for(uint16_t i=0;i<numberOfNodes;i++){
    //     ueNodes.Get(i)->AggregateObject(esCont);
    // }
    Time now = Simulator::Now();
    sem->SetCurrentA (2.33);
    now += Seconds (1701);
    Simulator::Schedule (now,
                       &SimpleDeviceEnergyModel::SetCurrentA,
                       sem,
                       4.66);
    now += Seconds (600);
                                   
    // Print every node energy infos
    PrintCellInfo (es,numberOfNodes);

    //std::cout << "testtesttesttesttest" << std::endl;
    //std::cout << source->GetNode()->GetId() << std::endl;

    //set the sending data
    //int dataValue = 1234;
    //Experiment experiment;

    //NS_LOG_INFO("Create Packet Socket");
    //void (Experiment::*executeSendPkt)(Ptr<NetDevice> deviceENB,Ptr<NetDevice> deviceUE,Ptr<Node> nodeENB,Ptr<Node> nodeUE, uint32_t dataValue)= &Experiment::Run;
    //Simulator::Schedule(Seconds(3),executeSendPkt,&experiment,enbLteDevs.Get(0),ueLteDevs.Get(0),enbNodes.Get(0),ueNodes.Get(0),dataValue);

    
    lteHelper->EnableTraces ();

    // NetAnimation
    //AnimationInterface anim("nbiot.xml");

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    // the cell voltage should be under 3.3v
    //DoubleValue v;
    //es->GetAttribute ("ThresholdVoltage", v);
    //NS_ASSERT (es->GetSupplyVoltage () <= v.Get ());

    return 0;

    }
