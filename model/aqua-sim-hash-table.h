
//...


#ifndef AQUA_SIM_HASH_TABLE_H
#define AQUA_SIM_HASH_TABLE_H

//#include "config.h"
//#include "tclcl.h"

#include "ns3/object.h"

namespace ns3{

#define TABLE_SIZE 20

struct ValueRecord{
	int node;
	int num;
};


class AquaSimHashTable : public Object {
public:
	AquaSimHashTable()
	{
		m_currentIndex = 0;
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			m_table[i].node = -1;
			m_table[i].num = -1;
		}
	}

	static TypeId GetTypeId(void);
	int m_currentIndex;
	int GetCurrentIndex(void) { return m_currentIndex; }
	void PutInHash(int addr);
	int  GetNode(int);
	int  GetNumber(int);
	ValueRecord m_table[TABLE_SIZE];
};

} // namespace ns3

#endif /* AQUA_SIM_HASH_TABLE_H */
