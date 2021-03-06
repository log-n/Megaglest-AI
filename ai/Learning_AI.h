// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_LEARNING_AI_H_
#define _GLEST_GAME_LEARNING_AI_H_


#include <vector>
#include <list>

#include "world.h"
#include "commander.h"
#include "command.h"
#include "randomgen.h"
#include "leak_dumper.h"
#include "unit_type.h"

	namespace Glest{ namespace Game{

class AiInterface;

#define NUM_OF_FEATURES 13
// 7 snapshot variables + 6 resource types

#define NUM_OF_RESOURCES 6
#define NUM_OF_ACTIONS 17
#define NUM_OF_STATES 6

enum Resources
{
	gold,
	wood,
	stone,
	food,
	energy,
	housing
};

enum NumberedActions
{	
    actHarvestGold, //0
	actHarvestWood,//1
	actHarvestStone,//2
	actHarvestFood,//3
	actHarvestEnergy,//4
	actHarvestHousing,//5
	actProduceWorker,//6
	actProduceWarrior,    //7
    actBuildDefensiveBuilding,//8
    actBuildWarriorProducerBuilding,//9
    actBuildResourceProducerBuilding,//10
    actBuildFarm,//11
    actRepairDamagedUnit,//12
    actUpgrade,    //13
	actSendScoutPatrol,//14
    actAttack,//15
	actDefend,//16
};

enum Features
{
	featureBeingAttacked ,
	featureReadyForAttack,
	featureDamagedUnitCount,
    featureNoOfWorkers,
	featureNoOfWarriors,
    featureNoOfBuildings,
	featureUpgradeCount,
	featureRsourcesAmountGold,
	featureRsourcesAmountWood,
	featureRsourcesAmountStone,
	featureRsourcesAmountFood,
	featureRsourcesAmountEnergy,
	featureRsourcesAmountHousing
};

enum States
{
		stateGatherResource,
		stateProduceUnit,
		stateRepair,
		stateUpgrade,
		stateAttack,
		stateDefend
};


//SNAPSHOTS
class Snapshot
{
private:
    static const int baseRadius= 25;
    static const int minWarriors = 25;

public:
	FILE * logs;
    AiInterface * aiInterface ; 
    int beingAttacked ;
    int readyForAttack;    
	int  damagedUnitCount ; 
    int noOfWorkers;    
	int noOfWarriors;
    int noOfBuildings ;     
	int resourcesAmount[NUM_OF_RESOURCES] ;
    int upgradeCount;
	int noOfKills;
	int noOfDeaths;
    /*
    int noOfDefensiveBuildings;
    int noOfWarriorProducerBuildings;
    int noOfResourceProducerBuildings;
    */
	Snapshot(AiInterface * aiInterface, FILE * logs);
    Snapshot &operator = ( const Snapshot &s );

private:
    void getKills();
	int getUpgradeCount();
	bool CheckIfBeingAttacked(Vec2i &pos, Field &field);
	bool CheckIfStableBase();
	int  countDamagedUnits( );
	int getCountOfType(const UnitType *ut);	
	int getCountOfClass(UnitClass uc);
	float getRatioOfClass(UnitClass uc);
	void  getResourceStatus();
	void setNumOfDeaths();
	//int  getCountOfDefensiveBuildings();
};


class Actions
{
private:
	static const int villageRadius= 15;
	static const int maxBuildRadius= 40; 

private:
	AiInterface *aiInterface;
	RandomGen random;
	int startLoc;
	FILE * logs;
private:
	bool findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly);
    bool canProduceUnit(const UnitType *ut);
	Vec2i getRandomHomePosition();
	bool isWarriorProducer(const UnitType *building);
	bool isResourceProducer(const UnitType *building);
	bool findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos);
	bool CheckAttackPosition(Vec2i &pos, Field &field, int radius);
	const UnitType* getFarm();
	void saveResourceAmount(int resourcesAmount[NUM_OF_RESOURCES]);
	bool switchAction(int actionNumber);

public:
	int resourceSpent[NUM_OF_RESOURCES];
	void clearResourceSpent();
	void UpdateSpentResource(const ProducibleType *pt);

//ACTIONS
	Actions(AiInterface * aiInterface, int startLoc, FILE * logs);
    bool sendScoutPatrol();
    bool massiveAttack(const Vec2i &pos, Field field);
    bool returnBase(int unitIndex);
    bool harvest(int resourceNumber) ;
    bool repairDamagedUnit();
    bool upgrade();
    bool produceOneUnit(UnitClass unitClass);    
    bool buildDefensiveBuilding();
    bool buildWarriorProducerBuilding();    
    bool buildResourceProducerBuilding();
    bool buildFarm();
	bool selectAction(int actionNumber);

};

class QObject
{
public:
	int state, action;
	bool actionSucceeded;
};

class LearningAI
{
	AiInterface *aiInterface;
	FILE * logs;
	bool probSelect;
	int resourceExtra[NUM_OF_RESOURCES];
	int isAllowed[NUM_OF_STATES][NUM_OF_ACTIONS];

	int startLoc;
	int GameNumber;

	Snapshot * lastSnapshot;
	bool lastActionSucceed;
	int lastState;
	int lastAct;
	double featureWeights[NUM_OF_FEATURES];

	double stateProbabs[NUM_OF_STATES];
	double sumStateProbabs[NUM_OF_STATES];

	double actionsProbabs[NUM_OF_ACTIONS]; // Probability of choosing an action
	double qValues[NUM_OF_STATES][NUM_OF_ACTIONS];
	float immediateReward[NUM_OF_STATES][NUM_OF_ACTIONS];
	RandomGen random;
	Actions  *action;
	int interval;
	int delayInterval;

	vector<QObject> state_action_list;
	int qLength;
	bool isBeingAttackedIntm;
	int lastAttackFrame;
	int updateCount;

public:
	~LearningAI();
	void printQvalues();
	void init(AiInterface *aiInterface,int useStartLocation=-1);
	void update(); 
	void battleEnd();
	Snapshot* takeSnapshot();
	void init_qvalues();
	void initImmediateRewards();
	double best_qvalue(int &state, int &best_action);	
	void update_qvalues(int olds,int news,int action,double r);
	int  semi_uniform_choose_action  (int & state);	
	void assignFeatureWeights();
	void  getStateProbabilityDistribution(Snapshot * currSnapshot);
	void getActionProbabilityDistribution(Snapshot *currSnapshot);
	void normalizeProbabilities(double values[] , int length);
	float getImmediateReward(int state, int action, bool actionSucceed);
	float getReward(Snapshot* preSnapshot, Snapshot* newSnapshot, bool actionSucceed);
	void back_update_qvalues( double reward);

private:
	void updateLoop();
	double probable_qvalue(int &best_action);
	int choose_probable_action  ();
	void  update_probable_qvalues(int action, double reward);
	int choose_one_on_prob(double arr[], int size);

	void back_update_reward(Snapshot* lastSnapshot, Snapshot* newSnapshot);
	void back_update_qvalues(double reward, int action);

	void saveStateProbDistribution();
};
}}

#endif
