#ifndef _GLEST_GAME_AISHA_H_
#define _GLEST_GAME_AISHA_H_

#include <vector>
#include <list>

#include "world.h"
#include "commander.h"
#include "command.h"
#include "randomgen.h"
#include "leak_dumper.h"
#include "Learning_AI.h"

#define AUTO_STATE_SIZE 4

enum AutoStates
{
		EarlyState,
		SilentState,
		AttackState,
		DefenceState
};



namespace Glest{ namespace Game{

class AiInterface;

class AIsha
{

	AiInterface *aiInterface;
	FILE * logs;

	int GameNumber;
	int startLoc;

	double qValues[AUTO_STATE_SIZE][NUM_OF_ACTIONS];

public:
	~AIsha(){};
	void init(AiInterface *aiInterface,int useStartLocation=-1);
	void update();
	void battleEnd();

};

}}

#endif
