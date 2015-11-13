//...


#include <math.h>
#include <stdlib.h>

#include "aqua-sim-mac.h"
#include "aqua-sim-node.h"
#include "aqua-sim-channel.h"
#include "aqua-sim-phy-cmn.h"
#include "aqua-sim-simple-propagation.h"

#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/boolean.h"
//#include "trace.h"		//TODO include trace
//#include "arp.h"
//#include "ll.h"
//#include "connector.h"
//#include "delay.h"
//#include "god.h"			

//...

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AquaSimNode");
NS_OBJECT_ENSURE_REGISTERED(AquaSimNode);

/*
void
AquaSimPositionHandler::Handle(Event*)
{
Scheduler& s = Scheduler::instance();


#if 0
fprintf(stderr, "*** POSITION HANDLER for node %d (time: %f) ***\n",
Node->Address(), s.clock());
#endif

* Update current location


//	printf("*** POSITION HANDLER for node %d (time: %f) ***\n", node->address(), s.clock()); // added by Peng Xie
if( node->MP_ != NULL )
node->MP_->update_position();
else{
node->update_position();
//node->random_destination();
}


* Choose a new random speed and direction

#ifdef DEBUG
fprintf(stderr, "%d - %s: calling random_destination()\n",
node->address_, __PRETTY_FUNCTION__);
#endif
//	node->random_destination();
node->check_position();
s.schedule(&node->uw_pos_handle_, &node->uw_pos_intr_,
node->position_update_interval_);
}
*/

AquaSimNode::AquaSimNode(void)
{	
  m_MP = NULL;		//MobileNode()

  m_id = 0;
  m_sid = 0;
  m_sinkStatus = 0;
  m_failureStatus = false;// add by peng xie
  m_failureStatusPro = 0;//added by peng xie AND ZHENG
  m_failurePro = 0.0; // add by peng xie
  m_transStatus = NIDLE;
  m_setHopStatus = 0;
  m_nextHop = -10;
  m_carrierId = false;
  m_carrierSense = false;
  m_statusChangeTime = 0.0;

  m_x = m_y = m_z = 0.0;
  m_dX = m_dY = m_dZ = 0.0;
  m_cX = m_cY = m_cZ = 0.0;
  m_speed = 0.0;

  m_randomMotion = 0;
}

AquaSimNode::~AquaSimNode(void)
{
}

/*void
AquaSimNode::start()
{
Scheduler& s = Scheduler::instance();

//printf("underwatersensornode: start\n");
if(m_randomMotion == 0) {
//printf("underwatersensornode: before log_movement\n");
log_movement();
return;
}

assert(initialized());

//printf("underwatersensornode: before random_position\n");

RandomPosition();
#ifdef DEBUG
fprintf(stderr, "%d - %s: calling random_destination()\n",
address_, __PRETTY_FUNCTION__);
#endif
RandomDestination();

s.schedule(&uw_pos_handle_, &uw_pos_intr_, position_update_interval_);
}*/

TypeId
AquaSimNode::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimNode")
    .SetParent<Node>()
    .SetParent<MobilityModel>()
    //3 following commands are for VBF related protocols only
    .AddAttribute("SetCx", "Set x for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNode::m_cX),
      MakeDoubleChecker<double>())
    .AddAttribute("SetCy", "Set y for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNode::m_cY),
      MakeDoubleChecker<double>())
    .AddAttribute("SetCz", "Set z for VBF related protocols.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNode::m_cZ),
      MakeDoubleChecker<double>())
    .AddAttribute("RandomMotion", "Node is mobile.",
      IntegerValue(0),
      MakeIntegerAccessor(&AquaSimNode::m_randomMotion),
      MakeIntegerChecker<int>())
    .AddAttribute("SetFailureStatus", "Set node failure status. Default false.",
      BooleanValue(0),
      MakeBooleanAccessor(&AquaSimNode::m_failureStatus),
      MakeBooleanChecker())
    .AddAttribute("SetFailureStatusPro", "Set node failure status pro.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNode::m_failureStatusPro),
      MakeDoubleChecker<double>())
    .AddAttribute("SetFailurePro", "Set node failure pro.",
      DoubleValue(0),
      MakeDoubleAccessor(&AquaSimNode::m_failurePro),
      MakeDoubleChecker<double>())
    .AddAttribute("NextHop", "Set next hop. Default is 1.",
      IntegerValue(1),
      MakeIntegerAccessor(&AquaSimNode::m_nextHop),
      MakeIntegerChecker<int>())
    .AddAttribute("SinkStatus", "Set the sink's status, int value.",
      IntegerValue(0),
      MakeIntegerAccessor(&AquaSimNode::m_sinkStatus),
      MakeIntegerChecker<int> ())
    ;
  return tid;
}

bool
AquaSimNode::IsMoving(void)
{
  NS_LOG_FUNCTION(this);

  if (m_MP == NULL){
      return false;
  }

  Vector vel = m_MP->GetVelocity();
  if (vel.x==0 && vel.y==0 && vel.z==0) {
      return false;
  }

  return true;
}

/*void
AquaSimNode::RandomPosition(void)
{
if (T_ == 0) {
fprintf(stderr, "No TOPOLOGY assigned\n");
exit(1);
}

X_ = m_uniformRand->GetValue() * T_->upperX();
Y_ = m_uniformRand->GetValue() * T_->upperY();
Z_ = m_uniformRand->GetValue() * T_->upperZ();

m_positionUpdateTime = 0.0;
}*/

int
AquaSimNode::SetSinkStatus()
{
  m_sinkStatus = 1;
  return 0;
}


int
AquaSimNode::ClearSinkStatus()
{
  m_sinkStatus = 0;
  return 0;
}

void
AquaSimNode::GenerateFailure()
{
  double error_pro = m_uniformRand->GetValue();
  if (error_pro<m_failureStatusPro)
    m_failureStatus = true;
}

/*
void
AquaSimNode::CheckPosition()
{
  if ((m_x == destX_) || (m_y == destY_)) {
  RandomSpeed();
  RandomDestination();
  }
  else {
    log_movement();
  }
}*/

/*
void
AquaSimNode::RandomSpeed()
{

speed_ = (m_uniformRand->GetValue() * (max_speed-min_speed))+min_speed;
//	printf("underwatersensornode: ?????????????????the max_speed%f min %f speed  %f interval%f\n",max_speed,min_speed,speed_,position_update_interval_);
}

void
AquaSimNode::RandomDestination()
{
//printf("mobilenode: ?????????????????the randmo_destionation\n");
if (T_ == 0) {
fprintf(stderr, "No TOPOLOGY assigned\n");
exit(1);
}

RandomSpeed();
#ifdef DEBUG
fprintf(stderr, "%d - %s: calling set_destination()\n",
address_, __FUNCTION__);
#endif
(void) set_destination(m_uniformRand->GetValue() * T_->upperX(),
m_uniformRand->GetValue() * T_->upperY(),
speed_);
}
*/

double
AquaSimNode::PropDelay(double distance)
{
  NS_LOG_FUNCTION(this);
  //printf("aquasimnode: ?????????????????the properdelay\n");
  return distance / SOUND_SPEED_IN_WATER;
  /*
   *	redudant, this is done in AquaSimChannel
  */
}

uint32_t
AquaSimNode::AddApplication(Ptr<Application> application)
{
  NS_LOG_FUNCTION(this);
  uint32_t index = m_applications.size();
  m_applications.push_back (application);
  application->SetNode(this);
  Simulator::ScheduleWithContext(GetId(), Seconds(0.0),
			  &Application::Initialize, application);
  return index;
}

uint32_t
AquaSimNode::AddDevice(Ptr<AquaSimNetDevice> device)
{
  NS_LOG_FUNCTION(this);
  uint32_t index = m_devices.size();
  m_devices.push_back(device);
  device->SetNode(this);
  return index;
}
Ptr<Application>
AquaSimNode::GetApplication(uint32_t index) const
{
  NS_LOG_FUNCTION(this);
  return m_applications[index];
}
Ptr<AquaSimNetDevice> 
AquaSimNode::GetDevice(uint32_t index) const
{
  NS_LOG_FUNCTION(this);
  return m_devices[index];
}
uint32_t 
AquaSimNode::GetId(void) const
{
  NS_LOG_FUNCTION(this);
  return m_id;
}
uint32_t 
AquaSimNode::GetSystemId(void) const
{
  NS_LOG_FUNCTION(this);
  return m_sid;
}
uint32_t 
AquaSimNode::GetNApplications(void) const
{
  NS_LOG_FUNCTION(this);
  return m_applications.size();
}
uint32_t 
AquaSimNode::GetNDevices(void) const
{
  NS_LOG_FUNCTION(this);
  return m_devices.size();
}

double
AquaSimNode::DistanceFrom(Ptr<AquaSimNode> n)
{
  return m_MP->GetDistanceFrom(n->m_MP);
}

Vector
AquaSimNode::Position(void)
{
  return m_MP->GetPosition();
}

double
AquaSimNode::RelativeSpeed(Ptr<AquaSimNode> n)
{
  return m_MP->GetRelativeSpeed(n->m_MP);
}

Vector
AquaSimNode::Velocity(void)
{
  return m_MP->GetVelocity();
}

void
AquaSimNode::SetNodePosition(const Vector &position)
{
  m_MP->SetPosition(position);
}

} // namespace ns3
