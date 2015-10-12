
//....


#ifndef AQUA_SIM_NODE_H
#define AQUA_SIM_NODE_H


#include "aqua-sim-packetstamp.h"
#include "aqua-sim-phy.h"
#include "aqua-sim-energy-model.h"
#include "aqua-sim-net-device.h"
#include "aqua-sim-simple-propagation.h"

#include "ns3/packet.h"	
#include "ns3/address.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/position-allocator.h"
#include "ns3/application.h"
//#include "trace.h"		//TODO should include trace

// Aqua Sim Node

namespace ns3{

enum TransmissionStatus { SLEEP, NIDLE, SEND, RECV, NStatus };	//idle currently used by ns3:WireState

//class UnderwaterMobilityPattern;	
class MobilityModel;	//TODO inherited Aqua Sim specialized class???
class AquaSimPacketStamp;
class AquaSimEnergyModel;
class AquaSimNetDevice;

/*
class AquaSimPositionHandler : public Handler {
public:
AquaSimPositionHandler(AquaSimNode* n) : Node(n) {}
void Handle(Event*);
private:
AquaSimNode *node;
};
*/

/******************
Should probably inherit from MobilityModel instead of Node
for position/vector purposes.

Adjust rest of module to match
*********************/

class AquaSimNode : public Node
{
public:
  AquaSimNode(void);
  virtual ~AquaSimNode(void);
  static TypeId GetTypeId(void);

  double PropDelay(double); //not this node's responsiblity here
  bool Move(void);	/*start the movement*/
  bool IsMove(void);
  //void Start(void);
  //void CheckPosition(void);

  Ptr<CubicPositionAllocator> T(void) { return m_T; }	//TODO

  /*coordinates*/
  inline double &X(void) { return m_x; }
  inline double &Y(void) { return m_y; }
  inline double &Z(void) { return m_z; }
  /*speeds projected to each axis*/
  inline double &dX(void) { return m_dX; }
  inline double &dY(void) { return m_dY; }
  inline double &dZ(void) { return m_dZ; }
  inline double &speed(void) { return m_speed; }

  inline Time &PositionUpdateTime(void) { return m_positionUpdateTime; }

  //sink related attributes
  int ClearSinkStatus(void);
  int SetSinkStatus(void);
  inline int GetSinkStatus(void) { return m_sinkStatus; }
  inline double &CX(void) { return m_cX; }
  inline double &CY(void) { return m_cY; }
  inline double &CZ(void) { return m_cZ; }

  inline bool FailureStatus(void) { return m_failureStatus; }
  inline double FailurePro(void) { return m_failurePro; }
  inline double FailureStatusPro(void) { return m_failureStatusPro; }

  inline void SetTransmissionStatus(enum TransmissionStatus status) {
    m_transStatus = status;
  }
  inline enum TransmissionStatus TransmissionStatus(void) { return m_transStatus; }

  inline bool CarrierSense(void) { return m_carrierSense; }
  inline void ResetCarrierSense(void) { m_carrierSense = false; }
  inline void SetCarrierSense(bool f){
    m_carrierSense = f;
    m_carrierId = f;
  }
  inline bool CarrierId(void) { return m_carrierId; }
  inline void ResetCarrierId(void) { m_carrierId = false; }

  int m_nextHop;
  int m_setHopStatus;
  int m_sinkStatus;
  AquaSimPacketStamp* m_pStamp;



  void UpdatePosition(void);  //out of date... should be using ns3's mobility module

  //inherited from Node class
  virtual uint32_t AddApplication(Ptr<Application> application);
  virtual uint32_t AddDevice(Ptr<AquaSimNetDevice> device);
  virtual Ptr<Application> GetApplication(uint32_t index) const;
  virtual Ptr<AquaSimNetDevice> GetDevice(uint32_t index) const;
  virtual uint32_t GetId(void) const;
  virtual uint32_t GetNApplications(void) const;
  virtual uint32_t GetNDevices(void) const;
  virtual uint32_t GetSystemId(void) const;
  //virtual void RegisterDeviceAdditionListener(DeviceAdditionListener listener);
  //virtual void RegisterProtocolHandler(ProtocolHandler handler, uint16_t protocolType,
  //				Ptr<AquaSimNetDevice> device, bool promiscuous=false);
  //virtual void UnregisterDeviceAdditionLister (DeviceAdditionListener listener);
  //virtual void UnregisterProtocolHandler(ProtocolHandler handler);

private:
  enum  TransmissionStatus m_transStatus;
  double m_statusChangeTime;  //the time when changing m_preTransStatus to m_transStatus

  bool	m_failureStatus;// 1 if node fails, 0 otherwise
  double 	m_failurePro;
  double 	m_failureStatusPro;

  /*
  *  The following indicate the (x,y,z) position of the node on
  *  the "terrain" of the simulation.
  */
  double m_x;
  double m_y;
  double m_z;
  double m_speed;	// meters per second

  /*
  *  The following is a unit vector that specifies the
  *  direction of the mobile node.  It is used to update
  *  position
  */
  double m_dX;
  double m_dY;
  double m_dZ;

  //the following attributes are added by Peng Xie for RMAC and VBF
  double m_cX;
  double m_cY;
  double m_cZ;

  bool m_carrierSense;
  bool m_carrierId;

  Time m_positionUpdateTime;

  std::vector< Ptr<Application> > m_applications;
  std::vector< Ptr<AquaSimNetDevice> > m_devices;
  uint32_t m_id;
  uint32_t m_sid;

protected:
  /*
  double m_maxSpeed;
  double m_minSpeed;
  void RandomSpeed(void);
  void RandomDestination(void);
  */
  void GenerateFailure(void);

  //TODO remove.
  //UnderwaterMobilityPattern * MP_; //our new mobility pattern module
  Ptr<MobilityModel> m_MP;

  Ptr<CubicPositionAllocator> m_T;	//TODO

  void RandomPosition(void);
  int m_randomMotion;	// is mobile

  Ptr<UniformRandomVariable> m_uniformRand;
  Ptr<AquaSimEnergyModel> m_energyModel;

public:
  Ptr<AquaSimEnergyModel> EnergyModel(void) { return m_energyModel; }
};  // class AquaSimNode

}  // namespace ns3

#endif /* AQUA_SIM_NODE_H */
