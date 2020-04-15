/// очередной неизвестный файл
#ifndef FSM_STORAGE_H
#define FSM_STORAGE_H

#include <unordered_map>
#include <string>

#include "FSM.h"
typedef std::unordered_map<std::string, FSM> FSMMap;

class FSMStorage
{
	public:
		FSMStorage() {}

	private:
		FSMMap fsmMap;
};
#endif

