
//...

//#include "tclcl.h"
#include "aqua-sim-hash-table.h"

namespace ns3{

NS_OBJECT_ENSURE_REGISTERED(AquaSimHashTable);


TypeId
AquaSimHashTable::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimHashTable")
    .SetParent<Object>()
    .AddConstructor<AquaSimHashTable>()
    ;
  return tid;
}

void
AquaSimHashTable::PutInHash(int addr)
{
  bool exist = false;
  int index = 0;
  for (int i = 0; i < m_currentIndex; i++){
    if (m_table[i].node == addr) {
      index = i;
      exist = true;
    }
  }

  if (exist) m_table[index].num++;
  else {
    m_table[m_currentIndex].node = addr;
    m_table[m_currentIndex].num = 1;
    m_currentIndex++;
  }
}

int
AquaSimHashTable::GetNode(int index)
{
  return m_table[index].node;
}

int
AquaSimHashTable::GetNumber(int index)
{
  return m_table[index].num;
}

}  //namespace ns3
