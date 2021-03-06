#include "Learning_AI.h"
#include "ai_interface.h"
#include "ai.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"
#include <time.h>
#include <cassert>
#include <math.h>


#define EXPERIMENT_ID_DONT_CHANGE 1

#define EXPLORATION_THRESHOLD 0.1

#define BETA 0.001
#define ALPHA 0.001
#define  GAMMA 0.05
#define DELTA 0.01
#define MAX_Q_LEN 100

namespace Glest { namespace Game {

static const int maxWorkers = 30;
static const int maxWarriors =  45;
static const int maxBuildings = 10;
static const int maxResource = 10000;

void LearningAI::init(AiInterface *aiInterface, int useStartLocation) 
{
	updateCount = 0;
	random.init(time(NULL));
	probSelect = false;
	isBeingAttackedIntm = false;
	lastAttackFrame = 0;

	resourceExtra[gold] = 200;
	resourceExtra[wood] = 150;
	resourceExtra[stone] = 120;
	resourceExtra[food] = 20;
	resourceExtra[energy] = 0;
	resourceExtra[housing]= 0;

	logs = fopen("learning_ai_log_file.txt", "w+");
	this->aiInterface= aiInterface;
	if(useStartLocation == -1) 
	{
		startLoc = random.randRange(0, aiInterface->getMapMaxPlayers()-1);
	}
	else 
	{
		startLoc = useStartLocation;
	}
	assignFeatureWeights();
	action = new Actions(aiInterface, startLoc, logs);
	init_qvalues();
	initImmediateRewards();
	lastSnapshot = takeSnapshot();
	getStateProbabilityDistribution(lastSnapshot);
	saveStateProbDistribution();

	if(probSelect)
	{
		lastAct = choose_probable_action();
	}
	else
	{
		lastAct =semi_uniform_choose_action  (lastState);
	}

	lastActionSucceed = action->selectAction(lastAct);
	//best_qvalue(int &state, int &best_action);

	interval = 25;
	delayInterval = 5000;
	qLength = 0 ;

	fprintf(logs," out of init...\n");
	fflush(logs);
}

void LearningAI::printQvalues()
{
	for(int i = 0 ; i < NUM_OF_STATES; i++)	
	{
		for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
		{
			fprintf(logs, "State %d Action %d : %f \n",i,j, qValues[i][j]);
		}
	}
}

void LearningAI::update()
{
	int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(ucBuilding) && aiInterface->getMyUnit(i)->getType()->getMaxHp() > 6000){
			++count;
		}
	}
	if(count == 0){
		return;
	}
	updateCount++;
		Snapshot * newSnapshot;
	//	if((aiInterface->getTimer() % (interval * GameConstants::updateFps / 1000)) == 0)
		{
//		printQvalues();
			aiInterface->printLog(4, " Inside update  :  " + intToStr(aiInterface->getTimer() )+"\n");
			fprintf(logs," Inside update  %d \n", aiInterface->getTimer() );
			fflush(logs);
			newSnapshot = takeSnapshot();
			//float reward = getReward(lastSnapshot, newSnapshot,  lastActionSucceed)	 ;		
			float reward = getImmediateReward(lastState, lastAct, lastActionSucceed);
			//free(lastSnapshot);
			//lastSnapshot = newSnapshot;
			getStateProbabilityDistribution(newSnapshot);
			saveStateProbDistribution();

			// push into queue
			QObject next ;
			next.action = lastAct;
			next.state = lastState;
			next.actionSucceeded = lastActionSucceed;
			if(next.actionSucceeded){
				state_action_list.push_back(next);
				qLength ++;
			}

			int iteration = 0;
		//	while(iteration < 10)
			{

				int newAct;

				if(probSelect)
				{
					newAct = choose_probable_action();
					if(lastActionSucceed){
						update_probable_qvalues(lastAct, reward);
					}
				}
				else
				{
					int newState ;
					newAct =semi_uniform_choose_action  (newState);
					fprintf(logs, "after semi_uniform_choose_action\n");
					fflush(logs);
					//update_qvalues(lastState, newState,lastAct,reward);
					fprintf(logs, "after update qvals\n");
					fflush(logs);
					lastState = newState;
				}

				lastActionSucceed  = action->selectAction(newAct);
				fprintf(logs, "after selectAction\n");
				fflush(logs);

				lastAct = newAct;
				aiInterface->printLog(4, " state :  " + intToStr( lastState )+ " Action : " + intToStr( lastAct )+"\n");
				fprintf(logs," state  :  %d action : %d \n",  lastState, lastAct);
				fprintf(logs," out of update ...\n");
				fflush(logs);

				if(lastActionSucceed){
			//		break;
				}
				iteration++;
			}
		}

		if(qLength !=0 && qLength%MAX_Q_LEN == 0)
		{
			
			//float reward = getReward(lastSnapshot, newSnapshot,  lastActionSucceed)	 ;
			//back_update_qvalues(reward);
			back_update_reward(lastSnapshot, newSnapshot);
			free(lastSnapshot);
			lastSnapshot = newSnapshot;
			lastAttackFrame = newSnapshot->beingAttacked ? 1 : 0;
			isBeingAttackedIntm = newSnapshot->beingAttacked;
		}
}

void LearningAI::back_update_qvalues(double reward, int action)
{
	if(qLength == 0){
		return;
	}
	if(reward < 0){
		reward = 0;
	}
	reward = reward * qLength/MAX_Q_LEN;

	double rewardStateRatio [NUM_OF_STATES];
	bool isPositive = false;
	for(int s=0; s< NUM_OF_STATES; s++)
	{
		if(qValues[s][action] > 0 && isAllowed[s][action] && sumStateProbabs[s]>0){
			rewardStateRatio[s] = sumStateProbabs[s];
			isPositive = true;
		}
		else{
			rewardStateRatio[s] = 0;
		}
	}
	if(isPositive == false){
		return;
	}
	normalizeProbabilities(rewardStateRatio, NUM_OF_STATES);

	for(int s=0; s< NUM_OF_STATES; s++)
	{
		if(isAllowed[s][action])
		{
			double newVal = (1-ALPHA)*qValues[s][action] + ALPHA*reward* rewardStateRatio[s];
			if(!isnan(newVal))
			{
				qValues[s][action] = newVal;
			}
		}
	}

	/*for(int i = qLength-1 ; i >= 0 ; i--)
	{
		QObject obj = state_action_list.at(i);
		if(obj.actionSucceeded && obj.action == action)
		{
			double qval = qValues[obj.state][obj.action]	;

			qValues[obj.state][obj.action] =   (1 - ALPHA)*qval + ALPHA*(reward);
		}
		reward -= DELTA*reward;
	}*/
}

void LearningAI :: back_update_qvalues(double reward)
{
	double qval, new_qval;	
	
	for(int i = qLength-2 ; i >= 0 ; i--)
	{
		QObject newS = state_action_list.at(i+1);
		QObject oldS = state_action_list.at(i);
		if(oldS.actionSucceeded){
			qval = qValues[oldS.state][oldS.action]	;
			new_qval=qValues[newS.state][newS.action]	;

			qValues[oldS.state][oldS.action] =   (1 - ALPHA)*qval + ALPHA*(reward  + GAMMA*new_qval);

			reward -= DELTA*reward;
		}
	}
	state_action_list.clear();
	qLength = 0;
}


Snapshot* LearningAI :: takeSnapshot()
{
	Snapshot *s = new  Snapshot(aiInterface , logs);
	if(s->beingAttacked && !lastAttackFrame){
		this->lastAttackFrame = qLength;
	}
	if(!s->beingAttacked && lastAttackFrame){
		lastAttackFrame = 0;
	}
	if(s->beingAttacked){
		this->isBeingAttackedIntm = true;
	}
	aiInterface->printLog(4, "Out of take snapshot \n");
	return s;
}

void LearningAI::saveStateProbDistribution()
{
	for(int i=0; i<NUM_OF_STATES; i++)
	{
		sumStateProbabs[i] += stateProbabs[i];
	}
}

void LearningAI:: getStateProbabilityDistribution(Snapshot *currSnapshot)
{
/*	fprintf(logs, "stateGatherResource  : %d \n",stateGatherResource);
	fprintf(logs, "stateProduceUnit  : %d \n",stateProduceUnit);
	fprintf(logs, "stateUpgrade  : %d \n",stateUpgrade);
	fprintf(logs, "stateRepair  : %d \n",stateRepair);	
	fprintf(logs, "stateAttack  : %d \n",stateAttack);
	fprintf(logs, "stateDefend  : %d \n",stateDefend);
	fflush(logs);
	aiInterface->printLog(4, " Inside State prob distribution \n");
	aiInterface->printLog(4, " State GatherResource  :  " + intToStr(stateGatherResource) +"\n" );
	aiInterface->printLog(4, " State stateProduceUnit  :  " + intToStr(stateProduceUnit) +"\n" );
	aiInterface->printLog(4, " State stateUpgrade  :  " + intToStr(stateUpgrade) +"\n" );
	aiInterface->printLog(4, " State stateRepair  :  " + intToStr(stateRepair) +"\n" );
	aiInterface->printLog(4, " State stateAttack  :  " + intToStr(stateAttack) +"\n" );
	aiInterface->printLog(4, " State stateDefend  :  " + intToStr(stateDefend) +"\n" );

*/
	// state gatherResource
	int resourceAmt = 0 ;
	for(int i =0 ; i < NUM_OF_RESOURCES; i++)
		resourceAmt+= currSnapshot-> resourcesAmount[i];
	//When all town centers are down, resources becomes zero ... to fix division by zero.
	if(resourceAmt == 0){
		resourceAmt = 1;
	}
	fprintf(logs , "Creating state probability distribution..\n");
	fprintf(logs , "Resources amount : %d .. Max resources :  %d \n", resourceAmt , maxResource );
	stateProbabs[stateGatherResource] =(maxWorkers/(currSnapshot-> noOfWorkers +1) )* featureWeights[featureNoOfWorkers] + (maxResource/resourceAmt)  * featureWeights[featureRsourcesAmountWood];
		
	 //stateproduceUnit
	float totalWeight = (maxWorkers/(currSnapshot-> noOfWorkers  +1))* featureWeights[featureNoOfWorkers]  + (maxWarriors/(currSnapshot-> noOfWarriors+1)) * featureWeights[featureNoOfWarriors]  +  (maxBuildings /(currSnapshot-> noOfBuildings +1) ) * featureWeights[featureNoOfBuildings] ; 
	stateProbabs[stateProduceUnit] = totalWeight;
	
	//stateRepair
	stateProbabs[stateRepair] = currSnapshot-> damagedUnitCount * featureWeights[featureDamagedUnitCount]  ;
	
	//stateUpgrade
	int totalUnits = currSnapshot-> noOfWorkers   +  currSnapshot-> noOfWarriors  + currSnapshot-> noOfBuildings  ;
	 stateProbabs[stateUpgrade] = totalUnits * featureWeights[featureUpgradeCount] ;
	fprintf(logs, "no. of units : %d \n" , totalUnits);

	//stateAttack
	stateProbabs[stateAttack] = currSnapshot->readyForAttack * featureWeights[featureReadyForAttack]  ;
	
	//stateDefend	
	stateProbabs[stateDefend] =  currSnapshot ->beingAttacked * featureWeights[featureBeingAttacked ]  ;	

	normalizeProbabilities(stateProbabs, NUM_OF_STATES);
}

void LearningAI::normalizeProbabilities(double values[] , int length)
{
		double sum = 0 ;
		for(int i =0 ; i < length ; i++)
		{
			fprintf(logs, " %d : %f\n" , i , values[i]);
			sum +=values[i];
		}
		fflush(logs);

		if(sum != 0)
		{
			for(int i =0 ; i < length ; i++)
				values[i] = values[i]/sum;
		}
		else
		{
			for(int i =0 ; i < length ; i++){
				values[i] = 0;
			}
			values[random.randRange(0, length-1)] = 1;
		}
}

void LearningAI::getActionProbabilityDistribution(Snapshot *currSnapshot)
{

}

void  LearningAI:: assignFeatureWeights()
{
	 //beingAttacked 
	featureWeights[featureBeingAttacked ] =  300;
    //readyForAttack
	featureWeights[featureReadyForAttack]  = 70 ;
	//damagedUnitCount 
	featureWeights[featureDamagedUnitCount 	]  = .2 ;
    //noOfWorkers;    
	featureWeights[featureNoOfWorkers]  = 10 ;
	//noOfWarriors;
	featureWeights[featureNoOfWarriors]  = 15 ;
    //noOfBuildings ;     
	featureWeights[featureNoOfBuildings ]  = 13 ;
	//upgradeCount;
	featureWeights[featureUpgradeCount]  = .5 ;
	//resourcesAmounts weights 
	//resource_name == "gold" 
	featureWeights[featureRsourcesAmountGold]  = 20 ;
	//	resource_name == "wood"  
	featureWeights[	featureRsourcesAmountWood]  = 10 ;
	//resource_name == "stone"
	featureWeights[featureRsourcesAmountStone]  = 10 ;
	//resource_name == "food"  
	featureWeights[featureRsourcesAmountFood]  = 10 ;
	//resource_name == "energy"  
	featureWeights[featureRsourcesAmountEnergy]  = 10 ;
	//resource_name == "housing"  
	featureWeights[featureRsourcesAmountHousing]  = 10 ;


	for (int s=0;s< NUM_OF_STATES;s++)
	{
		for (int a=0;a< NUM_OF_ACTIONS ;a++)
		{
				isAllowed[s][a] =0;
		}
	}
	 // Assign reward 1 to actions supported by each state
	  //stateGatherResource
	  	isAllowed[stateGatherResource][actHarvestGold] = 1;
	  	isAllowed[stateGatherResource][actHarvestWood] = 1;
	  	isAllowed[stateGatherResource][actHarvestStone] = 1;
	  	isAllowed[stateGatherResource][actHarvestFood] = 1;
	  	isAllowed[stateGatherResource][actHarvestEnergy] = 1;
	  	isAllowed[stateGatherResource][actHarvestHousing] = 1;
	  	isAllowed[stateGatherResource][actProduceWorker] = 1;

		//stateProduceUnit
	  	isAllowed[stateProduceUnit][actProduceWorker] = 1;
	  	isAllowed[stateProduceUnit][actProduceWarrior] = 1;
	  	isAllowed[stateProduceUnit][actBuildDefensiveBuilding] = 1;
	  	isAllowed[stateProduceUnit][actBuildWarriorProducerBuilding] = 1;
	  	isAllowed[stateProduceUnit][actBuildResourceProducerBuilding] = 1;
	  	isAllowed[stateProduceUnit][actBuildFarm] = 1;

		//stateRepair
	  	isAllowed[stateRepair][actRepairDamagedUnit] = 1;

		//stateUpgrade
	  	isAllowed[stateUpgrade][actUpgrade] = 1;

		//stateAttack
	  	isAllowed[stateAttack][actSendScoutPatrol] = 1;
	  	isAllowed[stateAttack][actAttack] = 1;
	  	isAllowed[stateAttack][actProduceWarrior] = 1;
	  	isAllowed[stateAttack][actBuildWarriorProducerBuilding] = 1;


		//stateDefend
	  	isAllowed[stateDefend][actDefend]= 1;
	  	isAllowed[stateDefend][actProduceWarrior] = 1;
	  	isAllowed[stateDefend][actBuildDefensiveBuilding] = 1;
	  	isAllowed[stateDefend][actBuildWarriorProducerBuilding] = 1;
	  // For actions that r not supported in a state we need to give -INT_MAX q value so that those r never chosen..
}

LearningAI :: ~LearningAI(){
//	printQvalues();
//battleEnd();
}

void LearningAI :: battleEnd()
{
	back_update_reward(lastSnapshot, takeSnapshot());

	FILE *  fp = fopen("Q_values.txt" , "w");
	fprintf(fp, "%d\n", EXPERIMENT_ID_DONT_CHANGE);
	fprintf(fp, "%d\n", GameNumber);
	fprintf(fp, "%d\n", updateCount);
	for(int i = 0 ; i < NUM_OF_STATES ; i++)
	{
		for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
		{
			if(j% 4 == 0){
				fprintf(fp, "\n");
			}
			fprintf(fp, "%lE\t", qValues[i][j]);
			fflush(fp);
		}
	}
	fclose(fp);
}

void  LearningAI ::  initImmediateRewards()
{
		FILE *  fp = fopen("ImmediateRewards.txt" , "r");
		if(fp !=NULL)
		{
			fprintf(logs, "Reward values read from file..\n");
			rewind (fp);			
			for(int i = 0 ; i < NUM_OF_STATES ; i++)
			{
				for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
				{
					double value;
					//fscanf(fp, "%f\n", &qValues[i][j]);
					fscanf(fp, "%lE", &value);
					immediateReward[i][j] = value;
					fprintf(logs, "Reward value for state %d Action %d : %lE \n", i,j,qValues[i][j]);
					fflush(logs);
				}				
			}
			fclose(fp);
			return;
		}
		
		for (int s=0;s< NUM_OF_STATES;s++)
		{
			for (int a=0;a< NUM_OF_ACTIONS ;a++)
			{
					immediateReward[s][a] =0; 
			}
		}

  // Assign reward 1 to actions supported by each state
  //stateGatherResource
  	immediateReward[stateGatherResource][actHarvestGold] = 1;
	immediateReward[stateGatherResource][actHarvestWood] = 1;
	immediateReward[stateGatherResource][actHarvestStone] = 1;
	immediateReward[stateGatherResource][actHarvestFood] = 1;
	immediateReward[stateGatherResource][actHarvestEnergy] = 1;
	immediateReward[stateGatherResource][actHarvestHousing] = 1;
	immediateReward[stateGatherResource][actProduceWorker] = 1;

	//stateProduceUnit
	immediateReward[stateProduceUnit][actProduceWorker] = 0.5;
	immediateReward[stateProduceUnit][actProduceWarrior] = 2;
	immediateReward[stateProduceUnit][actBuildDefensiveBuilding] = 3;
	immediateReward[stateProduceUnit][actBuildWarriorProducerBuilding] = 2;
	immediateReward[stateProduceUnit][actBuildResourceProducerBuilding] = 1.5;
	immediateReward[stateProduceUnit][actBuildFarm] = 1;

	//stateRepair
	immediateReward[stateRepair][actRepairDamagedUnit] = 1;

	//stateUpgrade
	immediateReward[stateUpgrade][actUpgrade] = 2;

	//stateAttack
	immediateReward[stateAttack][actSendScoutPatrol] = 1;
	immediateReward[stateAttack][actAttack] = 5;
	immediateReward[stateDefend][actProduceWarrior] = 1;
	immediateReward[stateDefend][actBuildWarriorProducerBuilding] = 1;

	//stateDefend
	immediateReward[stateDefend][actDefend]= 4;
	immediateReward[stateDefend][actProduceWarrior] = 4;
	immediateReward[stateDefend][actBuildDefensiveBuilding] = .5;
	immediateReward[stateDefend][actBuildWarriorProducerBuilding] = .5;

	fp = fopen("ImmediateRewards.txt" , "w");
		for(int i = 0 ; i < NUM_OF_STATES ; i++)
			{
				for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
				{
					fprintf(fp, "%f\n", immediateReward[i][j]);
				}
		}
		fclose(fp);	
}

void LearningAI ::  init_qvalues()
{
		FILE *  fp = fopen("Q_values.txt" , "r");

		if(fp != NULL)
		{
			fprintf(logs, "Q values read from file..\n");
			rewind (fp);
			int expId = 0;
			fscanf(fp, "%d", &expId);
			if(expId == EXPERIMENT_ID_DONT_CHANGE)
			{
				fscanf(fp, "%d", &GameNumber);
				GameNumber++;
				int timepass;
				fscanf(fp, "%d", &timepass);
				for(int i = 0 ; i < NUM_OF_STATES ; i++)
				{
					for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
					{
						double value;
						//fscanf(fp, "%f\n", &qValues[i][j]);
						fscanf(fp, "%lE", &value);
						qValues[i][j] = value;
						fprintf(logs, "Q value for state %d Action %d : %lE \n", i,j,qValues[i][j]);
						fflush(logs);
					}
				}
				fclose(fp);
				return;
			}
			fclose(fp);
		}

	 int s,a;
	 GameNumber = 1;
  
	for (s=0;s< NUM_OF_STATES;s++)
		for (a=0;a< NUM_OF_ACTIONS ;a++)
		{
			if(probSelect){
				qValues[s][a] = -1;
			}
			else{
				qValues[s][a] =0; // -1 * INT_MAX;;
			}
		}

  // Assign reward 1 to actions supported by each state
  //stateGatherResource
  	qValues[stateGatherResource][actHarvestGold] = 1;
	qValues[stateGatherResource][actHarvestWood] = 1;
	qValues[stateGatherResource][actHarvestStone] = 1;
	qValues[stateGatherResource][actHarvestFood] = 1;
	qValues[stateGatherResource][actHarvestEnergy] = 1;
	qValues[stateGatherResource][actHarvestHousing] = 1;
	qValues[stateGatherResource][actProduceWorker] = 1;

	//stateProduceUnit
	qValues[stateProduceUnit][actProduceWorker] = 1;
	qValues[stateProduceUnit][actProduceWarrior] = 1;
	qValues[stateProduceUnit][actBuildDefensiveBuilding] = 1;
	qValues[stateProduceUnit][actBuildWarriorProducerBuilding] = 1;
	qValues[stateProduceUnit][actBuildResourceProducerBuilding] = 1;
	qValues[stateProduceUnit][actBuildFarm] = 1;

	//stateRepair
	qValues[stateRepair][actRepairDamagedUnit] = 1;

	//stateUpgrade
	qValues[stateUpgrade][actUpgrade] = 1;

	//stateAttack
	qValues[stateAttack][actSendScoutPatrol] = 1;
	qValues[stateAttack][actAttack] = 1;
	qValues[stateAttack][actProduceWarrior] = 1;
	qValues[stateAttack][actBuildWarriorProducerBuilding] = 1;


	//stateDefend
	qValues[stateDefend][actDefend]= 1;
	qValues[stateDefend][actProduceWarrior] = 1;
	qValues[stateDefend][actBuildDefensiveBuilding] = 1;
	qValues[stateDefend][actBuildWarriorProducerBuilding] = 1;
  // For actions that r not supported in a state we need to give -INT_MAX q value so that those r never chosen..	
}

double LearningAI :: best_qvalue(int &state, int &best_action)
{
	state = choose_one_on_prob(stateProbabs, NUM_OF_STATES);
	fprintf(logs, "state returned is : %d \n", state);
	double qvals [NUM_OF_ACTIONS];

	for(int a=0; a< NUM_OF_ACTIONS; a++)
	{
		if(qValues[state][a] >0)
		{
			qvals[a] = qValues[state][a];
		}
		else
		{
			qvals[a] = 0;
		}
	}
	best_action = choose_one_on_prob(qvals, NUM_OF_ACTIONS);

	fprintf(logs, "Action chosen : %d ", best_action);
	/*
  int  s, a,best_act=0;
  double best_val = -1E+10;

  for(s = 0 ; s < NUM_OF_STATES; s++)
  {
	  for (a=0;a<NUM_OF_ACTIONS;a++)
	  {
			float value = stateProbabs[s] * qValues[s][a] ;
			if (value > best_val )
			{
				state = s;
				best_action = a;
				best_act = a;
				best_val = value;
			}
	  }
  }
*/
  fprintf(logs, "\nIn best Value !!! state %d Action %d\n", state , best_action);

  fflush(logs);

  return (qValues[state][best_action]);
}

double LearningAI:: probable_qvalue(int &best_action)
{
	double actQvals [NUM_OF_ACTIONS];
	for(int a=0; a< NUM_OF_ACTIONS; a++)
	{
		double sum = 0;
		for(int s = 0 ; s < NUM_OF_STATES; s++)
		{
			sum+= stateProbabs[s]*qValues[s][a];
		}
		if(sum < 0){
			sum = 0;
		}
		actQvals[a] = sum;
	}

	best_action = choose_one_on_prob(actQvals, NUM_OF_ACTIONS);
	return actQvals[best_action];
}

int LearningAI:: choose_one_on_prob(double arr[], int size)
{
	for(int i=0; i<size ; i++)
	{
		if (arr[i] < 0){
			assert(false);
		}
	}

	normalizeProbabilities(arr, size);

	  float zero = 0.0, one = 0.9999999999999;
	  float selection = random.randRange(zero, one);

	  int index = 0;
	  double prob_start = 0;
	  double prob_end = arr[index];

	  while(true)
	  {
		  if(prob_start <= selection  && prob_end >= selection)
		  {
			 return index;
		  }
		  index++;
		  if(index == NUM_OF_ACTIONS)
		  {
			  assert(false);
		  }
		  prob_start = prob_end;
		  prob_end += arr[index];
	  }
}

void  LearningAI :: update_qvalues(int olds,int news,int action,double reward)
{
	/*double best_qval, qval;
	int newAct;
	best_qval = best_qvalue(news,newAct);
	fprintf(logs, "Previous Q value: State %d Action %d : %f \n",olds, action, qValues[olds][action] );
	qval = qValues[olds][action];	
	qValues[olds][action] =   (1 - BETA)*qval + BETA*(reward  + GAMMA*best_qval);    */
	
	double qval = qValues[olds][action];
	qValues[olds][action] =   (1 - BETA)*qval + BETA*(reward);

	fprintf(logs, "Updated Q value : State %d Action %d : %f \n",olds, action, qValues[olds][action] );
}

int LearningAI ::  semi_uniform_choose_action  (int & state)
{
	   double best_val = -1E+10;
	  int act;
	  
	  float rand_value; 
	  float range_start = 0.0;
	  float range_end = 1.0; 
	  best_val=best_qvalue(state, act);
	  rand_value = random.randRange(range_start,  range_end);
	  //printf("%d\t%d\t%f\n", state, act, rand_value);
	  if (rand_value > (1.0 - EXPLORATION_THRESHOLD))
	  {			
			switch(state)
			{
			case stateGatherResource:
				act = random.randRange(actHarvestGold,actProduceWorker);
				break;
			case stateProduceUnit:
				act = random.randRange(actProduceWorker, actBuildFarm);
				break;
			case stateRepair:
				act = actRepairDamagedUnit;// random.randRange(,	NUM_OF_ACTIONS-1);
				break;
			case stateUpgrade: 
				act = actUpgrade ; //random.randRange(,NUM_OF_ACTIONS-1);
				break;
			case stateAttack:
				act = random.randRange(actSendScoutPatrol,actAttack);
				break;
			case stateDefend:
				act = actDefend ; //random.randRange(,NUM_OF_ACTIONS-1);
				break;
			}
			fprintf(logs, " random action : %d ",  act);
			fflush(logs);			
	  }	  
	  return (act);
} 

void  LearningAI :: update_probable_qvalues(int action, double reward)
{
	/*double best_qval = -1E+10, qval;

	for(int s =0; s< NUM_OF_STATES; s++)
	{
		if(qValues[s][action]*stateProbabs[s] > best_qval)
		{
			best_qval = qValues[s][action]*stateProbabs[s];
		}
	}*/

	for(int s =0; s< NUM_OF_STATES; s++)
	{
		double qval = qValues[s][action];
		qValues[s][action] =   (1 - BETA)*qval + BETA*(reward*stateProbabs[s]);
	}

}

int LearningAI ::  choose_probable_action  ()
{
	  double best_val = -1E+10;
	  int act;

	  float rand_value;
	  float range_start = 0.0;
	  float range_end = 1.0;
	  best_val=probable_qvalue(act);
	  rand_value = random.randRange(range_start,  range_end);
	  if (rand_value > (1.0 - EXPLORATION_THRESHOLD))
	  {
			act = random.randRange(0, NUM_OF_ACTIONS-1);
			fprintf(logs, " random action : %d ",  act);
			fflush(logs);
	  }
	  return (act);
}



#define DEFEND_SUCCESS_REWARD 200
#define KILL_REWARD 100
#define ARMY_READY_REWARD 100
#define ARMY_BUILD_SLOW_PENALTY 0
#define ATTACK_SLOW_PENALTY 0
#define ARMY_BACKWARD_PENALTY 0
#define REPAIR_DAMAGE_UNIT_REWARD 20
#define EMERGENCY_WORKERS_REWARD  25
#define MORE_WORKERS_REWARD  15
#define MORE_WARRIORS_REWARD 50
#define EMERGENCY_BUILDINGS_REWARD 30
#define MORE_BUILDINGS_REWARD 20
#define MORE_UPGRADES_REWARD 25
#define MORE_RESOURCE_REWARD 0.05f
#define EMERGENCY_RESOURCE_REWARD 0.15f
#define MIN_WORKERS 4
#define MAX_WORKERS 15
#define MIN_BUILDINGS 5
#define MAX_BUILDINGS 30
#define MIN_RESOURCE 1000
#define MAX_RESOURCE 10000
#define ACTION_FAILED_PENALTY 0

float LearningAI :: getImmediateReward(int state, int action, bool actionSucceeded)
{
	float reward = 0;
	if(!actionSucceeded)
	{
		reward = ACTION_FAILED_PENALTY;    
		fprintf(logs, "Action failed!! %d \n", lastAct);		
	}
	else
		reward = immediateReward[state][action];

	return reward;
}

#define RESOURCE_REWARD 0.01
#define RESOURCE_WORKER_CONNECT 1
#define MILITARY_EXTRA 30
#define MILITARY_REWARD 0.55
#define MILITARY_PRODUCTION_RATIO 8
#define ATTACK_FAST_INCENTIVE MILITARY_REWARD*5
#define ATTACK_INCENTIVE MILITARY_REWARD*10
#define SCOUT_RATIO 20
#define DEFENCE_INCENTIVE MILITARY_REWARD*5
#define DEFEND_FAST_INCENTIVE  MILITARY_REWARD*2.5
#define DEFEND_BUILDING_RATIO  8

void LearningAI::back_update_reward(Snapshot* lastSnapshot, Snapshot* newSnapshot)
{
	int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(ucBuilding) && aiInterface->getMyUnit(i)->getType()->getMaxHp() > 6000){
			++count;
		}
	}
	if(count > 0)
	{
		double produceWorkerReward = 0;
		for( int i = 0; i < NUM_OF_RESOURCES; i++ )
		{
			int resourceDelta = (newSnapshot->resourcesAmount[i] - lastSnapshot->resourcesAmount[i])/10;
			int resourceSpent = action->resourceSpent[i];
			int resourceProduced = resourceDelta + resourceSpent;
			if(resourceProduced < 0){
				resourceProduced = 0;
			}
			double reward = (((double)resourceSpent - (double)resourceProduced) + resourceExtra[i]) * RESOURCE_REWARD;
			produceWorkerReward += reward * RESOURCE_WORKER_CONNECT;
			back_update_qvalues(reward, i);
		}
		action->clearResourceSpent();

		//For worker
		back_update_qvalues(produceWorkerReward, actProduceWorker);
	}

	double military_reward = (lastSnapshot->noOfWarriors - newSnapshot->noOfWarriors + MILITARY_EXTRA)* MILITARY_REWARD;

	if(!lastSnapshot->beingAttacked && !isBeingAttackedIntm && !newSnapshot->readyForAttack){
		military_reward += ATTACK_FAST_INCENTIVE;
	}

	back_update_qvalues(military_reward, actProduceWarrior);
	back_update_qvalues(military_reward / MILITARY_PRODUCTION_RATIO, actBuildWarriorProducerBuilding);

	double defend_incentive = isBeingAttackedIntm ? DEFENCE_INCENTIVE : 0;

	if(newSnapshot->beingAttacked){

		defend_incentive += DEFEND_FAST_INCENTIVE * lastAttackFrame/MAX_Q_LEN;
	}

	back_update_qvalues(defend_incentive, actDefend);
	back_update_qvalues(defend_incentive/DEFEND_BUILDING_RATIO, actBuildDefensiveBuilding);


	double attact_incentive = newSnapshot->readyForAttack ? ATTACK_INCENTIVE : 0;
	back_update_qvalues(attact_incentive, actAttack);
	back_update_qvalues(attact_incentive/SCOUT_RATIO, actSendScoutPatrol);



	back_update_qvalues(0.0, actRepairDamagedUnit);
	back_update_qvalues(0.0, actUpgrade);
	back_update_qvalues(0.0, actBuildResourceProducerBuilding);
	back_update_qvalues(0.0, actBuildFarm);

	//END
	for(int s=0; s<NUM_OF_STATES; s++){
		sumStateProbabs[s] = 0;
	}
	state_action_list.clear();
	qLength = 0;
}

float LearningAI :: getReward(Snapshot* preSnapshot, Snapshot* newSnapshot, bool actionSucceed)
{
    float reward = 0;
/*    if( actionSucceed == false )
	{
		reward += ACTION_FAILED_PENALTY;    
		fprintf(logs, "Action failed!! %d \n", lastAct);
	}
	*/
    if( preSnapshot ->beingAttacked && !newSnapshot ->beingAttacked ) 
	{		
		reward += DEFEND_SUCCESS_REWARD;
		fprintf(logs, "Reward  after DEFEND_SUCCESS_REWARD : %f \n", reward);
    }
    else if( !preSnapshot ->beingAttacked && newSnapshot ->beingAttacked && newSnapshot ->readyForAttack ) {
	reward += ATTACK_SLOW_PENALTY;
	fprintf(logs, "Reward  after  ATTACK_SLOW_PENALTY: %f \n", reward);
    }
    else if( !preSnapshot ->beingAttacked && newSnapshot ->beingAttacked && !newSnapshot ->readyForAttack ) 
	{
		reward += ARMY_BUILD_SLOW_PENALTY;
		fprintf(logs, "Reward  after  ARMY_BUILD_SLOW_PENALTY : %f \n", reward);
    }

    if( !preSnapshot ->readyForAttack && newSnapshot ->readyForAttack ) 
	{
		reward += ARMY_READY_REWARD;
		fprintf(logs, "Reward  after  ARMY_READY_REWARD : %f \n", reward);
    }
    else if( preSnapshot ->readyForAttack && !newSnapshot ->readyForAttack ) 
	{
		reward += ARMY_BACKWARD_PENALTY;
		fprintf(logs, "ARMY_BACKWARD_PENALTY: %f\n ", reward);
    }

    reward += ( preSnapshot->damagedUnitCount / (newSnapshot ->damagedUnitCount +1 )) * REPAIR_DAMAGE_UNIT_REWARD;
		fprintf(logs, "Reward by damanged unit count : %f \n", reward);

    if( newSnapshot->noOfWorkers < MIN_WORKERS )	
	reward += ( newSnapshot->noOfWorkers / (preSnapshot->noOfWorkers +1 )) * EMERGENCY_WORKERS_REWARD;
    else if( newSnapshot->noOfWorkers <  MAX_WORKERS ) 
	reward += ( newSnapshot->noOfWorkers / (preSnapshot->noOfWorkers +1) )* MORE_WORKERS_REWARD;
    else
	reward += ( MAX_WORKERS / (newSnapshot->noOfWorkers +1 ) )* MORE_WORKERS_REWARD;

	fprintf(logs, "Reward by worker  unit count : %f \n", reward);
    reward += ( newSnapshot->noOfWarriors / (preSnapshot->noOfWarriors +1) ) * MORE_WARRIORS_REWARD;
	fprintf(logs, "Reward by warrior unit count : %f \n", reward);
    reward += ( newSnapshot->noOfKills / (preSnapshot->noOfKills  +1)) * KILL_REWARD;
	fprintf(logs, "Reward by no of Kills count : %f \n", reward);

    if( newSnapshot->noOfBuildings < MIN_BUILDINGS )
	reward += ( newSnapshot->noOfBuildings /( preSnapshot->noOfBuildings +1 ) )* EMERGENCY_BUILDINGS_REWARD;
    else if( newSnapshot->noOfBuildings < MAX_BUILDINGS )
	reward += ( newSnapshot->noOfBuildings / (preSnapshot->noOfBuildings +1) )* MORE_BUILDINGS_REWARD;
    else
	reward += ( MAX_BUILDINGS /(newSnapshot->noOfBuildings +1)) * MORE_BUILDINGS_REWARD;
	fprintf(logs, "Reward by no  of buildings count : %f \n", reward);

    for( int i = 0; i < NUM_OF_RESOURCES; i++ ) 
	{
		if(newSnapshot->resourcesAmount[ i ]  > preSnapshot->resourcesAmount[ i ])
		{
			if( newSnapshot->resourcesAmount[ i ] < MIN_RESOURCE ) 		
				reward += ( newSnapshot->resourcesAmount[ i ] - preSnapshot->resourcesAmount[ i ] ) * EMERGENCY_RESOURCE_REWARD;
			else if( newSnapshot->resourcesAmount[ i ] < MAX_RESOURCE )
				reward += ( newSnapshot->resourcesAmount[ i ] - preSnapshot->resourcesAmount[ i ] ) * MORE_RESOURCE_REWARD;
//			else
//				reward += ( MAX_RESOURCE - preSnapshot->resourcesAmount[ i ] ) * MORE_RESOURCE_REWARD;
		}
	fprintf(logs, "Reward by no  of resources count : %f \n", reward);
    }

    reward += (float)( newSnapshot->upgradeCount /(preSnapshot->upgradeCount+1) ) * MORE_UPGRADES_REWARD;
	fprintf(logs, "Reward by upgrade count : %f \n", reward);
	fprintf(logs,"REWARD is %f \n", reward);
    return reward;
}

void LearningAI:: updateLoop()
{

}

}}//end namespace


