#include "Learning_AI.h"
#include "ai_interface.h"
#include "ai.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"
#include <time.h>
#include <cassert>


#define EXPLORATION_THRESHOLD 0.1

#define BETA 0.04
#define  GAMMA 0.05

namespace Glest { namespace Game {

void LearningAI::init(AiInterface *aiInterface, int useStartLocation) 
{
	random.init(time(NULL));
	probSelect = false;
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

	interval = 50;
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
		Snapshot * newSnapshot;
		if((aiInterface->getTimer() % (interval * GameConstants::updateFps / 1000)) == 0)
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

			// push into queue
			QObject next ;
			next.action = lastAct;
			next.state = lastState;
			next.actionSucceeded = lastActionSucceed;
			state_action_list.push_back(next);
			qLength ++;

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
				update_qvalues(lastState, newState,lastAct,reward);
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
		}

		if(qLength !=0 && qLength%100 == 0)
		{
			
			float reward = getReward(lastSnapshot, newSnapshot,  lastActionSucceed)	 ;	
			back_update_qvalues(reward);
			free(lastSnapshot);
			lastSnapshot = newSnapshot;			
		}
}

void LearningAI :: back_update_qvalues(double reward)
{
	double qval, new_qval;	
	
	for(int i = qLength-2 ; i >= 0 ; i--)
	{
		QObject newS = state_action_list.at(i+1);
		QObject oldS = state_action_list.at(i);
		qval = qValues[oldS.state][oldS.action]	;
		new_qval=qValues[newS.state][newS.action]	;

		qValues[oldS.state][oldS.action] =   (1 - BETA)*qval + BETA*(reward  + GAMMA*new_qval); 

		reward = GAMMA*reward;
	}
	state_action_list.clear();
	qLength = 0;
}


Snapshot* LearningAI :: takeSnapshot()
{
	Snapshot *s = new  Snapshot(aiInterface , logs);
	aiInterface->printLog(4, "Out of take snapshot \n");
	return s;
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
	featureWeights[featureBeingAttacked ] =  500;
    //readyForAttack
	featureWeights[featureReadyForAttack]  = 70 ;
	//damagedUnitCount 
	featureWeights[featureDamagedUnitCount 	]  = 3 ;
    //noOfWorkers;    
	featureWeights[featureNoOfWorkers]  = 10 ;
	//noOfWarriors;
	featureWeights[featureNoOfWarriors]  = 15 ;
    //noOfBuildings ;     
	featureWeights[featureNoOfBuildings ]  = 13 ;
	//upgradeCount;
	featureWeights[featureUpgradeCount]  = 5 ;
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
}

LearningAI :: ~LearningAI(){
	printQvalues();
//battleEnd();
}

void LearningAI :: battleEnd()
{
	FILE *  fp = fopen("Q_values.txt" , "w");
	fprintf(fp, "%d\n", GameNumber);
	for(int i = 0 ; i < NUM_OF_STATES ; i++)
	{
		for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
		{
			fprintf(fp, "%lf\n ", qValues[i][j]);
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
			GameNumber++;
			for(int i = 0 ; i < NUM_OF_STATES ; i++)
			{
				for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
				{
					double value;
					//fscanf(fp, "%f\n", &qValues[i][j]);
					fscanf(fp, "%lf", &value);
					immediateReward[i][j] = value;
					fprintf(logs, "Reward value for state %d Action %d : %lf \n", i,j,qValues[i][j]);
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
	immediateReward[stateAttack][actSendScoutPatrol] = 3;
	immediateReward[stateAttack][actAttack] = 5;

	//stateDefend
	immediateReward[stateDefend][actDefend]= 5;

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
			fscanf(fp, "%d", &GameNumber);
			GameNumber++;
			for(int i = 0 ; i < NUM_OF_STATES ; i++)
			{
				for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
				{
					double value;
					//fscanf(fp, "%f\n", &qValues[i][j]);
					fscanf(fp, "%lf", &value);
					qValues[i][j] = value;
					fprintf(logs, "Q value for state %d Action %d : %lf \n", i,j,qValues[i][j]);
					fflush(logs);
				}				
			}
			fclose(fp);
			return;
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

	//stateDefend
	qValues[stateDefend][actDefend]= 1;
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

	  float zero = 0.0, one = 1.0;
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
	  printf("%d\t%d\t%f\n", state, act, rand_value);
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
#define ACTION_FAILED_PENALTY -1

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


