//...

#include <stdlib.h>
#include <stdio.h>

#include "aqua-sim-sink.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-header.h"
#include "aqua-sim-hash-table.h"

#include "ns3/random-variable-stream.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/attribute.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/string.h"

#define REPORT_PERIOD     2
#define INTEREST_PERIOD   2

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimSink");
NS_OBJECT_ENSURE_REGISTERED(AquaSimSink);


bool
operator<(const SenseAreaElem&  e1, const SenseAreaElem& e2)
{
  return (e1.senseX < e2.senseX) && (e1.senseY < e2.senseY)
	  && (e1.senseZ < e2.senseZ) && (e1.senseR < e2.senseR);
}

void
SenseArea::Insert(double x, double y, double z, double r)
{
  AreaSet.insert(SenseAreaElem(x, y, z, r));
}

bool
SenseArea::IsInSenseArea(double nx, double ny, double nz)
{
  double deltaX, deltaY, deltaZ, distance;
  set<SenseAreaElem>::iterator pos = AreaSet.begin();
  while (pos != AreaSet.end()) {
    deltaX = nx - (*pos).senseX;
    deltaY = ny - (*pos).senseY;
    deltaZ = nz - (*pos).senseZ;

    distance = sqrt(deltaX*deltaX + deltaY*deltaY + deltaZ*deltaZ);
    if (distance < (*pos).senseR)
      return true;
    pos++;
  }
  return false;
}


int AquaSimSink::m_pktId = 0;

AquaSimSink::AquaSimSink() :	//Agent(PT_UWVB), 
	m_running(0), 
	m_random(0), 
	m_sinkTimer(Timer::CANCEL_ON_DESTROY),
	m_periodicTimer(Timer::CANCEL_ON_DESTROY),
	m_reportTimer(Timer::CANCEL_ON_DESTROY)
{
  // set option first. ... TODO clean this up...

  m_AppDup = true; // allow duplication

  m_periodic = false; //just send out interest once
  // always_max_rate_ = false; //?

  // Initialize variables.

  m_maxPkts = 1000;
  m_pktCount = 0;
  m_numRecv = 0;
  m_numSend = 0;
  //  RecvPerSec=0;

  strcpy(m_fName, "test.data");

  m_targetX = 0;
  m_targetY = 0;
  m_targetZ = 0;
  //m_targetId.port_ = 0;

  m_passive = 0;
  m_cumDelay = 0.0;
  m_activeSense = 0;

  /*
  // bind("data_type_", &data_type_);
  bind_time("data_rate_", &data_rate_);
  bind("packetsize_", &packetsize_);
  bind("random_", &random_);
  bind("passive", &passive);
  bind("ActiveSense", &ActiveSense);
  bind("SenseInterval", &SenseInterval);
  // bind("num_recv", &num_recv);
  // bind("num_send", &num_send);
  // bind("cum_delay", &cum_delay);
  */

  /*
  size_=64;
  interval_=1;
  random_=0;
  */
  //simple_report_rate = ORIGINAL;

  m_dataCounter = 0;
  m_lastArrivalTime = -1.0;

  m_range = 0.0;
  m_dataInterval = 0.0;
  m_packetSize = 0;
  m_dataRate = 0.0;

  m_senseX = 0.0;
  m_senseY = 0.0;
  m_senseZ = 0.0;
  m_senseR = 0.0;
  m_senseInterval = 0;
  m_interval = 0;

  m_exploreStatus = 0;
  m_exploreRate = 0;
  m_exploreInterval = 0.0;
  m_exploreCounter = 0;
}


void
AquaSimSink::Start()
{
  m_running = 1;
  m_interval = 1.0 / m_dataRate;
  m_random = 0;
  SendPkt();

  if (m_sinkTimer.IsRunning()) {
    m_sinkTimer.Remove();
  }
  m_sinkTimer.Schedule(Time(m_interval));
}

void
AquaSimSink::ExponentialStart()
{
  m_random = 2;
  m_running = 1;
  GenerateInterval();

  if (m_sinkTimer.IsRunning()) {
    m_sinkTimer.Remove();
  }
  m_sinkTimer.Schedule(Time(m_interval));
}

void
AquaSimSink::GenerateInterval()
{
  double R = m_uniformRand->GetValue();
  double lambda = m_dataRate;
  m_interval = -log(R) / lambda;
  // printf("\naquasimsink: !!!!!!!!GenerateInterval the  interval is %f\n",m_interval);
  return;
}

void
AquaSimSink::Stop()
{
  if (m_running) {
    m_running = 0;
  }

  if (m_periodic == true) {
    m_periodic = false;
    m_periodicTimer.Cancel();
  }
}

void
AquaSimSink::Report()
{
  NS_LOG_FUNCTION(this << m_here << Simulator::Now());

  if (m_reportTimer.IsRunning()) {
    m_reportTimer.Remove();
  }
  m_reportTimer.Schedule(Time(REPORT_PERIOD));
  // RecvPerSec = 0;
}

void
AquaSimSink::Timeout(int)
{
  if (m_running) {
    SendPkt();
    double t = m_interval;
    if (m_random == 1) {
	/* add some zero-mean white noise */
	t += m_interval * m_uniformRand->GetValue(-0.5, 0.5);
    }
    if (m_random == 2) {
      GenerateInterval();
      t = m_interval;
    }
    if (m_sinkTimer.IsRunning()) {
      m_sinkTimer.Remove();
    }
    m_sinkTimer.Schedule(Time(t));
  }
}


void
AquaSimSink::SendPkt()
{
  if (m_activeSense) {
    m_node->UpdatePosition();
    if (!SenseAreaSet.IsInSenseArea(m_node->X(), m_node->Y(), m_node->Z())) {
      //detect frequently
      m_interval = 1;
      return;
    }
    else {
      m_interval = m_senseInterval;
    }
  }


  Ptr<Packet> pkt = CreatePacket();
  AquaSimHeader ashdr;

  //TODO implement other headers
  //hdr_uwvb* vbh = HDR_UWVB(pkt);		//not implemented

  ashdr.SetUId(m_pktId++);
  ashdr.SetSAddr( m_here );
  ashdr.SetDAddr( m_targetId );		//??? here_.addr_
  ashdr.SetDPort(255);

  //cmh->ptype() = PT_UWVB;
  //cmh->size() = m_packetSize;

  //Not implemented yet
  /*
  vbh->mess_type = DATA;
  vbh->pk_num = m_pktCount;
  vbh->ts_ = NOW;
  m_pktCount++;
  vbh->sender_id = m_here;
  // vbh->data_type = data_type_;
  vbh->forward_agent_id = m_here;

  vbh->target_id = m_targetId;
  vbh->range = m_range;


  vbh->info.tx = m_targetX;
  vbh->info.ty = m_targetY;
  vbh->info.tz = m_targetZ;

  vbh->info.fx = m_node->CX();
  vbh->info.fy = m_node->CY();
  vbh->info.fz = m_node->CZ();

  vbh->info.ox = m_node->CX();
  vbh->info.oy = m_node->CY();
  vbh->info.oz = m_node->CZ();

  vbh->info.dx = 0;
  vbh->info.dy = 0;
  vbh->info.dz = 0;
  */

  /*
  vbh->original_source.x=vbh->info.fx;
  vbh->original_source.y=vbh->info.fy;
  vbh->original_source.z=vbh->info.fz;
  */
  //printf("uw_sink:source(%d,%d) send packet %d at %lf : the coordinates of target is (%lf,%lf,%lf) and range=%lf and my position (%f,%f,%f) and cx is(%f,%f,%f)  type is %d\n", vbh->sender_id.addr_,vbh->sender_id.port_,vbh->pk_num, NOW, vbh->info.tx=target_x,vbh->info.ty=target_y, vbh->info.tz=target_z,vbh->range,node->X(),node->Y(),node->Z(),node->CX(),node->CY(),node->CZ(),vbh->mess_type);

  m_numSend++;
  //   vbh->attr[0] = data_type_;

  pkt->AddHeader(ashdr);

  if (m_targetId.IsInvalid())  printf("The target's address is invalid\n");
  else {
    NS_ASSERT(pkt != NULL);
    // printf("The target's address is not  empty\n");
    m_phy->SendPktDown(pkt);
  }
  // printf("I exit the sendpk \n");
}


void
AquaSimSink::DataReady()
{
  // printf("I am in the sendpk1 \n");
  if (m_pktCount >= m_maxPkts) {
    m_running = 0;
    return;
  }
  // printf("I am in the sendpk2 \n");
  Ptr<Packet> pkt = CreatePacket();
  AquaSimHeader ashdr;

  //hdr_uwvb* vbh = HDR_UWVB(pkt);

  ashdr.SetSAddr( m_here );
  ashdr.SetDAddr( m_targetId );	//???	here_.addr_
  ashdr.SetDPort(255);

  /*Not implemented yet*/
  /*
  vbh->mess_type = DATA_READY;
  vbh->pk_num = m_pktCount;
  m_pktCount++

  vbh->sender_id = m_here;
  //      vbh->data_type = m_dataType;
  vbh->forward_agent_id = m_here;
  vbh->target_id = m_targetId;
  vbh->range = m_range;
  vbh->ts_ = NOW;

  NS_LOG_FUNCTION(this << "Source: " << vbh->sender_id.addr_ << vbh->sender_id.port_
	  << node->X() << node->Y() << node->Z() << "Send data ready packet"
	  << vbh->pk_num << " at " << NOW << " the target is : "
	  << vbh->target_id.addr_ << vbh->target_id.port_);
  */

  NS_LOG_FUNCTION(this << "Source: " << ashdr.GetSAddr() << ", ("
	  << m_node->X()<<","<<m_node->Y()<<","<<m_node->Z()<<") Send data ready packet: "
	  << m_pktCount<< " at " << Simulator::Now() << " the target is : "
	  << ashdr.GetDAddr()<< "," << ashdr.GetDPort());

  // Send the packet
  // printf("Source %s send packet ( %x,%d) at %lf.\n", name(),
  // vbh->sender_id, vbh->pk_num, NOW);
  //  num_send++;
  //      vbh->attr[0] = data_type_;

  pkt->AddHeader(ashdr);

  if (m_targetId.IsInvalid())  printf("The target's address is invalid\n");
  else {
    NS_ASSERT(pkt != NULL);
    // printf("The target's address is not  empty\n");
    m_phy->SendPktDown(pkt);
  }
  // printf("I exit the sendpk \n");
}



void
AquaSimSink::SourceDeny(uint32_t id, double x, double y, double z)
{
  Ptr<Packet> pkt = CreatePacket();
  AquaSimHeader ashdr;

  //hdr_uwvb* vbh = HDR_UWVB(pkt);

  ashdr.SetSAddr( m_here );
  ashdr.SetDAddr( m_targetId );	//???	here_.addr_
  ashdr.SetDPort(255);

  /* Not implemented yet

  // Set message type, packet number and sender ID
  vbh->mess_type = SOURCE_DENY;
  vbh->pk_num = m_pktCount;
  m_pktCount++;

  vbh->sender_id = m_here;
  //      vbh->data_type = m_dataType;
  vbh->forward_agent_id = m_here;
  vbh->target_id = id;

  vbh->info.tx = x;
  vbh->info.ty = y;
  vbh->info.tz = z;
  vbh->range = m_range;
  vbh->ts_ = NOW;

  // Send the packet
  vbh->info.ox = m_node->X();
  vbh->info.oy = m_node->Y();
  vbh->info.oz = m_node->Z();


  vbh->original_source.x=vbh->info.ox;
  vbh->original_source.y=vbh->info.oy;
  vbh->original_source.z=vbh->info.oz;


  vbh->info.dx = 0;
  vbh->info.dy = 0;
  vbh->info.dz = 0;

  NS_LOG_FUNCTION(this << "Source: " << vbh->sender_id.addr_ << vbh->sender_id.port_
	  << node->X() << node->Y() << node->Z() << "Send data ready packet"
	  << vbh->pk_num << " at " << NOW << " the target is : "
	  << vbh->target_id.addr_ << vbh->target_id.port_);
  */

  NS_LOG_FUNCTION(this << "Source: " << ashdr.GetSAddr() << ", ("
	  << m_node->X() << "," << m_node->Y() << "," << m_node->Z() << ") Send data ready packet: "
	  << m_pktCount << " at " << Simulator::Now() << " the target is : "
	  << ashdr.GetDAddr() << "," << ashdr.GetDPort());

  pkt->AddHeader(ashdr);

  m_phy->SendPktDown(pkt);
}


void
AquaSimSink::BcastInterest()
{
  // printf("uw_sink: the address of thenew packet is%d\n",pkt);

  Ptr<Packet> pkt = CreatePacket();
  AquaSimHeader ashdr;

  ashdr.SetSAddr( m_here );
  ashdr.SetDAddr( m_targetId );	//???	here_.addr_
  ashdr.SetDPort(255);

  /* Not implemented yet */
  /*
  // Set message type, packet number and sender ID
  vbh->mess_type = INTEREST;
  vbh->pk_num = m_pktCount;
  m_pktCount++;
  vbh->sender_id = m_here;
  // vbh->data_type = m_dataType;
  vbh->forward_agent_id = m_here;

  vbh->target_id = m_targetId;

  vbh->info.tx = m_targetX;
  vbh->info.ty = m_targetY;
  vbh->info.tz = m_targetZ;
  vbh->range = m_range;
  vbh->ts_ = NOW;

  // Send the packet

  vbh->info.ox = m_node->X();
  vbh->info.oy = m_node->Y();
  vbh->info.oz = m_node->Z();

  vbh->original_source.x=vbh->info.ox;
  vbh->original_source.y=vbh->info.oy;
  vbh->original_source.z=vbh->info.oz;

  vbh->info.dx = 0;
  vbh->info.dy = 0;
  vbh->info.dz = 0;

  NS_LOG_FUNCTION(this << "Source: " << vbh->sender_id.addr_ << vbh->sender_id.port_
	  << node->X() << node->Y() << node->Z() << "Send data ready packet"
	  << vbh->pk_num << " at " << NOW << " the target is : "
	  << vbh->target_id.addr_ << vbh->target_id.port_);
  */
  //printf("uw_sink:source(%d,%d) send interest packet %d at %lf : the target is (%d,%d) coordinate is (%f,%f,%f)\n", vbh->sender_id.addr_, vbh->sender_id.port_, vbh->pk_num, NOW, vbh->target_id.addr_, vbh->target_id.port_, vbh->info.tx, vbh->info.ty, vbh->info.tz);

  // printf("Sink %s send packet (%x, %d) at %f to %x.\n",
  //    name_, dfh->sender_id,
  //     dfh->pk_num,
  //     NOW,
  //     iph->dst_);
  // printf("uw_sink:I am in the sendpk before send\n") ;

  NS_LOG_FUNCTION(this << "Source: " << ashdr.GetSAddr() << ", ("
	  << m_node->X() << "," << m_node->Y() << "," << m_node->Z() << ") Send data ready packet: "
	  << m_pktCount << " at " << Simulator::Now() << " the target is : "
	  << ashdr.GetDAddr() << "," << ashdr.GetDPort());

  pkt->AddHeader(ashdr);

  m_phy->SendPktDown(pkt);

  if (m_periodic == true) {
    if (m_periodicTimer.IsRunning()) {
      m_periodicTimer.Remove();
    }
    m_periodicTimer.Schedule(Time(INTEREST_PERIOD));
  }
}


/*
void
AquaSimSink::DataReady()
{

// Create a new packet
Ptr<Packet> pkt = CreatePacket();

// Access the Sink header for the new packet:
hdr_uwvb* vbh = HDR_UWVB(pkt);
// hdr_ip* iph = HDR_IP(pkt);

// Set message type, packet number and sender ID
vbh->mess_type = DATA_READY;
vbh->pk_num = m_pktCount;
m_pktCount++;
vbh->sender_id = m_here;
vbh->data_type = m_dataType;
vbh->forward_agent_id = m_here;

Send(pkt, 0);
}
*/


void
AquaSimSink::Terminate()
{
  FILE * fp;
  if ((fp = fopen(m_fName, "a")) == NULL){
    NS_LOG_WARN("SINK " << m_here << " can not open file");
    return;
  }

  NS_LOG_DEBUG(fp << "SINK: " << m_here << " terminates ( send "
	  << m_numSend << ", recv " << m_numRecv << ", cum_delay "
	  << m_cumDelay << ")");

  int index = DataTable->GetCurrentIndex();

  for (int i = 0; i < index; i++){
    NS_LOG_FUNCTION(this << " Sink " << m_here << " : sendId = "
	    << DataTable->GetNode(i) << ", numRecv = n/a");
    //printf("SINK(%d) : send_id = %d, num_recv = %d\n",
    //	here_.addr_, DataTable.node(i), DataTable.number(i));
  }

  fclose(fp);
  m_running = 0;
}

TypeId
AquaSimSink::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimSink")
    .SetParent<Object>()
    .AddConstructor<AquaSimSink>()
    .AddAttribute("Duplicate", "Set duplicate on/off. Default is on.",
      BooleanValue(true),
      MakeBooleanAccessor(&AquaSimSink::m_AppDup),
      MakeBooleanChecker ())
    .AddAttribute("TargetAddr", "Target address.",
      AddressValue(),
      MakeAddressAccessor(&AquaSimSink::m_targetId),
      MakeAddressChecker())
    .AddAttribute("TargetX", "Set target's x",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimSink::m_targetX),
      MakeDoubleChecker<double>())
    .AddAttribute("TargetY", "Set target's y",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimSink::m_targetY),
      MakeDoubleChecker<double>())
    .AddAttribute("TargetZ", "Set target's z",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimSink::m_targetZ),
      MakeDoubleChecker<double>())
    .AddAttribute("Range", "Set range",
      DoubleValue(0.0),
      MakeDoubleAccessor(&AquaSimSink::m_range),
      MakeDoubleChecker<double>())
    .AddAttribute("PacketSize", "Set the packet size.",
      IntegerValue(0),
      MakeIntegerAccessor(&AquaSimSink::m_packetSize),
      MakeIntegerChecker<int>())
    //.AddAttribute("Filename", "Set input filename.",
    //	StringValue("test.data"),
    //	MakeStringAccessor(&AquaSimSink::m_fName),
    //	StringChecker())
    .AddAttribute("SetAddr", "Set the sink's address.",
      AddressValue(),
      MakeAddressAccessor(&AquaSimSink::m_here),
      MakeAddressChecker())
    //.AddAttribute("SenseArea", "Set the sensing area.",
    //	DoubleValue(SenseAreaSet.Insert(0.0, 0.0, 0.0, 1.0)),
    //	MakeDoubleAccessor(&AquaSimSink::SenseAreaSet),
    //	MakeDoubleChecker<double>())
    ;
  return tid;
}
//TODO attach routing layer...
//TODO the following arg commands:			(all are functions)
//terminate()?, announce .. bcast_interest()?,
//ready .. dataready()?, send .. SendPkt()?, exp-start .. ExponentialStart()?
//stop .. Stop()?  node lookup???


void
AquaSimSink::Recv(Ptr<Packet> pkt)  //handler not implemented
{
  //hdr_uwvb* vbh = HDR_UWVB(pkt);

  /*****Dummy Recv for now**********/

  // printf("SK %d recv (%x, %x, %d) %s size %d at time %lf\n", here_.addr_,
  // (dfh->sender_id).addr_, (dfh->sender_id).port_,
  // dfh->pk_num, MsgStr[dfh->mess_type], cmh->size(), NOW);

  /*
  if (data_type_ != vbh->data_type) {
  printf("SINK: Hey, What are you doing? I am not a sink for %d. I'm a sink for %d. \n", vbh->data_type, data_type_);
  Packet::free(pkt);
  return;
  }
  */

  // printf("AquaSimSink: I get the packet  \n");

  /*
  switch (vbh->mess_type) {
  */
	  /*
	  case DATA_REQUEST :

	  if (always_max_rate_ == false)
	  simple_report_rate = vbh->report_rate;

	  if (!running_) start();

	  //      printf("I got a data request for data rate %d at %lf. Will send it right away.\n",
	  //	     simple_report_rate, NOW);

	  break;


	  case DATA_STOP :

	  if (running_) stop();
	  break;
	  */
/*
  case DATA_READY:

	  NS_LOG_FUNCTION("AquaSimSInk (id:" << m_here << ")("
		  << m_node->CX() << "," << m_node->CY() << ","
		  << m_node->CZ() << "): I get the data ready packet no."
		  << vbh->m_pktNum);

	  if (m_node->SinkStatus()){
		  m_passive = 1;
		  // m_numRecv++;
		  m_targetX = m_node->X() - m_node->CX();
		  m_targetY = m_node->Y() - m_node->CY();
		  m_targetZ = m_node->Z() - m_node->CZ();
		  m_lastArrivalTime = NOW;
		  m_targetId = vbh->sender_id;
		  BcastInterest();
	  }
	  else{
		  m_targetX = 0;
		  m_targetY = 0;
		  m_targetZ = 0;
		  m_targetId = vbh->sender_id;
		  Start();
	  }
	  break;

  case INTEREST:

	  num_recv++;
	  target_id = vbh->sender_id;
	  target_x = vbh->info.ox;
	  target_y = vbh->info.oy;
	  target_z = vbh->info.oz;
	  running_ = 1;

	  start();
	  break;

  case DATA:

  case TARGET_DISCOVERY:
  {    //printf("uw_sink: the source is out of scope %d\n",passive);
	  if (!m_passive){
		  if (IsDeviation())
		  {
		  NS_LOG_INFO(this << "The source is out of scope");
		  double x = m_node->X() - m_node->CX();
		  double y = m_node->Y() - m_node->CY();
		  double z = m_node->Z() - m_node->CZ();
		  uint32_t id = vbh->sender_id;
		  SourceDeny(id, x, y, z);
		  BcastInterest();
	  }
  }

  NS_LOG_INFO(this << "(id:" << m_here << "): I get the packet data no."
	  << vbh->pk_num << " from " << vbh->forward_agent_id.addr_);
  m_cumDelay = m_cumDelay + (NOW - vbh->ts_);
  m_numRecv++;
  // RecvPerSec++;
  //      God::instance()->IncrRecv();

  //        int* sender_addr=new int[1];
  // sender_addr[0]=vbh->sender_id.addr_;

  int sender_addr = vbh->sender_id.addr_;

  DataTable.PutInHash(sender_addr);

  if (m_lastArrivalTime > 0.0) {
	  NS_LOG_INFO("SK " << m_here << ": NumRecv " << m_numRecv
		  << ", InterArrival " << (NOW)-m_lastArrivalTime);
  }

  m_lastArrivalTime = NOW;
  break;
  }
  default:
	  break;
  }
  */
  NS_LOG_FUNCTION(this << " I received packet " << pkt <<
	  " Dummy Recv, needs to be implemented");
  pkt = 0;
}


bool
AquaSimSink::IsDeviation(){
  double dx = m_node->CX() - m_node->X();
  double dy = m_node->CY() - m_node->Y();
  double dz = m_node->CZ() - m_node->Z();
  if (sqrt((dx*dx) + (dy*dy) + (dz*dz)) < m_range) return false;
  return true;
}

void
AquaSimSink::Reset()
{
}

void
AquaSimSink::SetAddr(Address address)
{
  m_here = address;
}


int
AquaSimSink::GetPktCount()
{
  return m_pktCount;
}

void
AquaSimSink::IncrPktCount()
{
  m_pktCount++;
}

Ptr<Packet>
AquaSimSink::CreatePacket()
{
  Ptr<Packet> pkt = Create<Packet>();
  //Packet *pkt = allocpkt();


  if (pkt == NULL) return NULL;
  AquaSimHeader ashdr;

  //ashdr->size() = 0;  // the control packet size is determined by the vbf header
  //cmh->ptype() = PT_UWVB;
  //hdr_uwvb* vbh = HDR_UWVB(pkt);
  //vbh->ts_ = NOW;

  pkt->AddHeader(ashdr);

  return pkt;
}


}  //namespace ns3
