#include "Learning_AI.h"
#include "ai_interface.h"
#include "ai.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"

namespace Glest { namespace Game {

Snapshot::Snapshot(AiInterface * aiInterface , FILE * logs)
{
	this->logs = logs;
    this->aiInterface = aiInterface; 
    beingAttacked = 0 ;
    readyForAttack = 0;
    damagedUnitCount = 0; 
    noOfWorkers = 0;
    noOfWarriors = 0;
    noOfBuildings = 0; 
    for( int i = 0; i < NUM_OF_RESOURCES; i++ ) {
	resourcesAmount[ i ] = 0;    
    }
    /*
    resourcesType[ 0 ] = "gold";
    resourcesType[ 1 ] = "wood";
    resourcesType[ 2 ] = "stone";
    resourcesType[ 3 ] = "food";
    resourcesType[ 4 ] = "energy";
    resourcesType[ 5 ] = "housing";*/
    upgradeCount = 0;
    noOfKills = 0;
	fprintf(logs, "out of Snapshot constructor\n");
	fflush(logs);
	aiInterface->printLog(4, "out of Snapshot constructor at line \n" );//+intToStr( __LINE__ )+ " in file " + intToStr(__FILE__));
}

Snapshot &Snapshot::operator =( const Snapshot &s ) {
    aiInterface = s.aiInterface ; 
    beingAttacked = s.beingAttacked ;
    readyForAttack = s.readyForAttack;
    damagedUnitCount = s.damagedUnitCount; 
    noOfWorkers = s.noOfWorkers;
    noOfWarriors = s.noOfWarriors;
    noOfBuildings = s.noOfBuildings; 
    for( int i = 0; i < NUM_OF_RESOURCES; i++ ) {
	resourcesAmount[ i ] = s.resourcesAmount[ i ] ;    
    }
    upgradeCount = s.upgradeCount;
    noOfKills = s.noOfKills;
	
    return *this;
}

int Snapshot::getKills()
{
    int count = 0 ; 
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		const Unit *u= aiInterface->getMyUnit(i);
		count += u->getKills();
	}
	noOfKills  = count;
	fprintf(logs," No of kills : %d \n", noOfKills);
	fflush(logs);
	return count ;	
}


bool Snapshot::CheckIfBeingAttacked(Vec2i &pos, Field &field)
{
	fprintf(logs, "In check if being attacked....\n");
	if (aiInterface == NULL)
			fprintf(logs, " aiInterface is NULL\n");
	fflush(logs);
    int count= aiInterface->onSightUnitCount();
    const Unit *unit;
	int radius = baseRadius;
    for(int i=0; i<count; ++i){
        unit= aiInterface->getOnSightUnit(i);
        if(!aiInterface->isAlly(unit) && unit->isAlive()){
            pos= unit->getPos();
			field= unit->getCurrField();
            if(pos.dist(aiInterface->getHomeLocation())<radius)
	    {
                aiInterface->printLog(4, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");
				fprintf(logs," Being attacked at pos  %d   %d \n"  , pos.x ,pos.y);
				fflush(logs);
				beingAttacked = 1;
                return true;
            }
        }
    }
	aiInterface->printLog(4, "Not  Being attacked  \n");
	fprintf(logs," Not  Being attacked  \n");
	fflush(logs);
    beingAttacked = 0;
    return false;

}


bool Snapshot::CheckIfStableBase(){

    if(getCountOfClass(ucWarrior)>minWarriors)
    {
        aiInterface->printLog(4, "Base is stable\n");
		fprintf(logs,"Base is stable\n");
		fflush(logs);
	readyForAttack =  1;
        return true;
    }
    else{
        aiInterface->printLog(4, "Base is not stable\n");
		fprintf(logs,"Base is not stable\n");
		fflush(logs);

	readyForAttack = 0;
        return false;
    }
}

int Snapshot::countDamagedUnits( )
{
    int count = 0 ; 
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *u= aiInterface->getMyUnit(i);
		if(u->getHpRatio()<1.f)
		{
		    count++;
		}
	}
	damagedUnitCount  = count;
	aiInterface->printLog(4, "Damaged Unit count :  " + intToStr(damagedUnitCount)+"\n");
	fprintf(logs,"Damaged Unit count :  %d \n" , damagedUnitCount);
	fflush(logs);
	return count ;	
}

int Snapshot::getCountOfType(const UnitType *ut){
    int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(ut == aiInterface->getMyUnit(i)->getType()){
			count++;
		}
	}
	
    return count;
}

int Snapshot::getCountOfClass(UnitClass uc){
    int count= 0;
    for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(uc)){
            ++count;
		}
    }
	aiInterface->printLog(4, "Unit count  for unit " +  intToStr(uc ) + "  :  " + intToStr(count)+"\n");
	fprintf(logs, "Unit count  for unit %d is %d \n", uc , count);
	fflush(logs);
    return count;
}

float Snapshot::getRatioOfClass(UnitClass uc){
	if(aiInterface->getMyUnitCount()==0){
		return 0;
	}
	else{
		return static_cast<float>(getCountOfClass(uc))/aiInterface->getMyUnitCount();
	}
}
int Snapshot:: getUpgradeCount()
{
    upgradeCount = aiInterface->getMyUpgradeCount();
	aiInterface->printLog(4, " Upgrade count    " +  intToStr(upgradeCount  ) +"\n");
	fprintf(logs, "Upgrade count  : %d \n",upgradeCount);
	fflush(logs);
    return upgradeCount;
}

void Snapshot:: getResourceStatus()
{
	for(int i = 0 ; i < NUM_OF_RESOURCES; i++)
	{
		resourcesAmount[i] = 0;
	}
    const TechTree *tt= aiInterface->getTechTree();
	for(int unitIndex = 0 ; unitIndex < aiInterface->getMyUnitCount() ; unitIndex++ )
	{
		const Unit *unit = aiInterface->getMyUnit(unitIndex);

		for(int i = 0; i < tt->getResourceTypeCount(); ++i)
		{
			const ResourceType *rt= tt->getResourceType(i);
			const Resource *r= aiInterface->getResource(rt);
			int amt = r->getAmount();
			string resource_name = rt->getName ();  
			if(resource_name == "gold" )
			{
					resourcesAmount[gold] += amt ; 
			}
			else if(resource_name == "wood"  )
			{
					resourcesAmount[wood] += amt ; 
			}
			else if(resource_name == "stone"  )
			{
					resourcesAmount[stone] += amt ;
			}
			else if(resource_name == "food"  )
			{
					resourcesAmount[food] += amt ;
			}
			else if(resource_name == "energy"  )
			{
					resourcesAmount[energy] += amt ;
			}
			else if(resource_name == "housing"  )
			{
					resourcesAmount[housing] += amt ;
			}								
		}
	}
}

Actions::Actions(AiInterface * aiInterface , int startLoc , FILE * logs)
{
	this->logs = logs;
	this->aiInterface = aiInterface;
	this->startLoc = startLoc;
}


bool Actions::selectAction(int actionNumber)
{	
	fprintf(logs, "Selected action : %d\n", actionNumber);
	fflush(logs);
	
	switch(actionNumber)
	{
			case actSendScoutPatrol:
					return sendScoutPatrol();
					break;

			case actAttack:
				{
					Vec2i  pos;
					Field field;
					CheckAttackPosition(pos, field, INT_MAX);
					return  massiveAttack(pos, field);
					break;
				}

			case actDefend:
				{
					Vec2i  pos;
					Field field;
					CheckAttackPosition(pos, field, villageRadius + 10);
					return massiveAttack(pos, field);
					break;
				}

			case actHarvestGold: 
					return harvest(gold) ;
					break ;

			case actHarvestWood:
					return harvest(wood) ;
					break;

			case actHarvestStone:
					return harvest(stone) ;
					break;

			case actHarvestFood:
					return harvest(food) ;
					break;

			case actHarvestEnergy: 
					return harvest(energy) ;
					break;

			case actHarvestHousing:
					return harvest(housing) ;
					break;    

			case actRepairDamagedUnit:
					return repairDamagedUnit();
					break;

			case actUpgrade:
					return upgrade();
					break;

			case actProduceWorker:
					return produceOneUnit(ucWorker);    
					break;	

			case actProduceWarrior:
					return produceOneUnit(ucWarrior);    
					break;

			case actBuildDefensiveBuilding:
					return buildDefensiveBuilding();
					break;

			case actBuildWarriorProducerBuilding:
					return buildWarriorProducerBuilding();    
					break;

			case actBuildResourceProducerBuilding:
					return buildResourceProducerBuilding();
					break;

			case actBuildFarm:
					return buildFarm();
					break;
					
			default:
				return false;
	}
}

bool Actions::CheckAttackPosition(Vec2i &pos, Field &field, int radius)
{
    int count= aiInterface->onSightUnitCount();
    const Unit *unit;
	
    for(int i=0; i<count; ++i){
        unit= aiInterface->getOnSightUnit(i);
        if(!aiInterface->isAlly(unit) && unit->isAlive()){
            pos= unit->getPos();
			field= unit->getCurrField();
            if(pos.dist(aiInterface->getHomeLocation())<radius)
	    {
                aiInterface->printLog(2, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");			
                return true;
            }
        }
    }   
    return false;
}

bool Actions::canProduceUnit(const UnitType *ut)
{
	if(aiInterface->reqsOk(ut))
	{
		//if unit doesnt meet resources retry
		if(aiInterface->checkCosts(ut))
		{
			return true;
		}
	}
	return false;
}

bool Actions::findAbleUnit(int *unitIndex, CommandClass ability, bool idleOnly){
	vector<int> units;

	*unitIndex= -1;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *unit= aiInterface->getMyUnit(i);
		if(unit->getType()->hasCommandClass(ability)){
			if(!idleOnly || !unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop){
				units.push_back(i);
			}
		}
	}
	if(units.empty()){
		return false;
	}

	else{
		*unitIndex= units[random.randRange(0, units.size()-1)];
		return true;
	}
}




bool Actions::sendScoutPatrol()
{
    Vec2i pos;
    int unit;

	startLoc= (startLoc+1) % aiInterface->getMapMaxPlayers();
	pos= aiInterface->getStartLocation(startLoc); 
	
		bool flag = findAbleUnit(&unit, ccAttack, true)		;
		if(flag == false)
			return flag ;
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		aiInterface->giveCommand(unit, ccAttack, pos);
		aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x)+","+intToStr(pos.y)+"\n");
		fprintf(logs, "Scout patrol sent to: %d , %d\n " ,pos.x, pos.y);
		return true;
}


bool Actions::massiveAttack(const Vec2i &pos, Field field)
{
	int producerWarriorCount=0;
	int maxProducerWarriors=random.randRange(1,11);
	int unitCount = aiInterface->getMyUnitCount();
	int cnt =0 ;
    for(int i = 0; i < unitCount; ++i) 
	{    	
        const Unit *unit= aiInterface->getMyUnit(i);
		const AttackCommandType *act= unit->getType()->getFirstAttackCommand(field);

		if(!unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop)
		{		
			bool alreadyAttacking= (unit->getCurrSkill()->getClass() == scAttack);
			if(!alreadyAttacking && act!=NULL ) 
			{				
				aiInterface->giveCommand(i, act, pos);				
				cnt ++;
			}
			if(alreadyAttacking)
				cnt++;
		}
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	aiInterface->printLog(2, "Massive attack to pos: "+ intToStr(pos.x)+", "+intToStr(pos.y)+"\n");
	fprintf(logs	, "Massive attack to pos:  %d, %d \n",pos.x ,pos.y);
	if(cnt == 0)
		return false;
	else
		return true;
}

bool Actions::returnBase(int unitIndex) {
    Vec2i pos;
    CommandResult r;
    int fi;

    fi= aiInterface->getFactionIndex();
    pos= Vec2i(random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) +
		 aiInterface->getHomeLocation();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    r= aiInterface->giveCommand(unitIndex, ccMove, pos);
	return true;
//    aiInterface->printLog(1, "Order return to base pos:" + intToStr(pos.x)+", "+intToStr(pos.y)+": "+rrToStr(r)+"\n");
}

bool Actions::harvest(int resourceNumber) 
{	
	fprintf(logs, "Inside harvest %d\n", resourceNumber);
	fflush(logs);
    const TechTree *tt= aiInterface->getTechTree();	
	int unitIndex ;	
	bool flag = findAbleUnit(&unitIndex, ccHarvest, true);
	if(flag == false)
	{
		fprintf(logs, "No free Unit \n");
		 return false;
	}
	const Unit *unit = aiInterface->getMyUnit(unitIndex);
	fprintf(logs, "UnitIndex for harvest %d\n", unitIndex);
	fflush(logs);
	const ResourceType *rt= tt->getResourceType(resourceNumber);

	if(rt != NULL) 
	{
		fprintf(logs, "rt not null\n");
		const HarvestCommandType *hct= aiInterface->getMyUnit(unitIndex)->getType()->getFirstHarvestCommand(rt,aiInterface->getMyUnit(unitIndex)->getFaction());
		if(hct != NULL)
		{
			fprintf(logs, "htc is not null\n");
		}
		Vec2i resPos;
		if(hct != NULL && aiInterface->getNearestSightedResource(rt, aiInterface->getHomeLocation(), resPos, false)) 
		{
			resPos= resPos+Vec2i(random.randRange(-2, 2), random.randRange(-2, 2));
			aiInterface->giveCommand(unitIndex, hct, resPos);
			fprintf(logs, "Command sent to AI for harvest \n");
			fflush(logs);
//			aiInterface->printLog(4, "Order harvest pos:" + intToStr(resPos.x)+", "+intToStr(resPos.y)+": "+rrToStr(r)+"\n");
		}
		else
			return false;
	}
	else
			return false;
	return true;
}


bool Actions::repairDamagedUnit() 
{
    bool flag = false;

    //find a repairer and issue command
    for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
    {
		const Unit *u= aiInterface->getMyUnit(i);
		if(u->getHpRatio()<1.f)
		{	
				const RepairCommandType *rct= static_cast<const RepairCommandType *>(u->getType()->getFirstCtOfClass(ccRepair));
				if(rct!=NULL && (u->getCurrSkill()->getClass()==scStop || u->getCurrSkill()->getClass()==scMove))
				{
						if(rct->isRepairableUnitType(u->getType()))
						{
	    						aiInterface->giveCommand(i, rct, u->getPos());
								//aiInterface->printLog(3, "Repairing order issued");	
								fprintf(logs, "Command sent to AI for repair \n");

								flag = true;
						}
				}
		}
	}	
	return flag;   
}


bool Actions::produceOneUnit(UnitClass unitClass)
{
	//for each unit, produce it if possible
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j)
		{
			const CommandType *ct= ut->getCommandType(j);

			//if the command is produce
			if(ct->getClass()==ccProduce || ct->getClass()==ccMorph)
			{
				const UnitType *producedUnit= static_cast<const UnitType*>(ct->getProduced());
				bool produceIt= false;

				//if the unit is from the right class
				if(producedUnit->isOfClass(unitClass))
				{
					if(aiInterface->reqsOk(ct) && aiInterface->reqsOk(producedUnit))
					{
						aiInterface->giveCommand(i, ct);
						fprintf(logs, "Command sent to AI for produce unit \n");

						return true;
					}
				}
			}	
		}
	}
	return false;
}

bool Actions::upgrade()
{
	  bool flag = false;
		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
		{
			//for each command
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);

				//if the command is upgrade
				if(ct->getClass()==ccUpgrade)
				{
					const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
					const UpgradeType *producedUpgrade= uct->getProducedUpgrade();
					if(aiInterface->reqsOk(uct))
					{
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							aiInterface->giveCommand(i, uct);
							flag = true;			
					}
				}
			}
		}
		return flag;
}

bool Actions::findPosForBuilding(const UnitType* building, const Vec2i &searchPos, Vec2i &outPos){

	const int spacing= 1;

    for(int currRadius=0; currRadius<maxBuildRadius; ++currRadius){
        for(int i=searchPos.x-currRadius; i<searchPos.x+currRadius; ++i){
            for(int j=searchPos.y-currRadius; j<searchPos.y+currRadius; ++j){
                outPos= Vec2i(i, j);
                if(aiInterface->isFreeCells(outPos-Vec2i(spacing), building->getSize()+spacing*2, fLand)){
                    return true;
                }
            }
        }
    }

    return false;

}
 
Vec2i Actions:: getRandomHomePosition()
{
	return Vec2i(random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) ;
}

bool Actions::buildDefensiveBuilding()
{
	//for each unit
	fprintf(logs, "In build Defensive Building...");
	fflush(logs);
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		const Unit * unit = aiInterface->getMyUnit(i);
		if(!unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop)
		{	
			fprintf(logs, "unit %d is free for building...\n", i);
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);
				//if the command is build
				if(ct->getClass()==ccBuild)    
				{
					const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);

					//for each building
					for(int k=0; k<bct->getBuildingCount(); ++k)
					{
						const UnitType *building= bct->getBuilding(k);
						if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
						{
							if(building->hasSkillClass(scAttack))
							{
								if(aiInterface->checkCosts(building) == false) 
								{					    
									fprintf(logs, "constly... to build..\n");
										return false;
								}
								Vec2i pos;
								Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
			    				if(findPosForBuilding(building, searchPos, pos)) 
								{
									aiInterface->giveCommand(i, bct, pos,building);
									fprintf(logs, "Command sent to AI for build defensive building \n");

									return true;
								}

							}
						}
					}
				}
			}
		}
		else
		{
			fprintf(logs, "Unit %d already has a command...\n", i);
			fflush(logs);
			continue;
		}
	}
	return false;
}

bool Actions::isWarriorProducer(const UnitType *building){
	for(int i= 0; i < building->getCommandTypeCount(); i++){
		const CommandType *ct= building->getCommandType(i);
		if(ct->getClass() == ccProduce){
			const UnitType *ut= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

			if(ut->isOfClass(ucWarrior)){
				return true;
			}
		}
	}
	return false;
}

bool  Actions::buildWarriorProducerBuilding()
{
	//for each unit
		fprintf(logs, "In buildWarriorProducerBuilding...");
		fflush(logs);

	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		const Unit * unit = aiInterface->getMyUnit(i);
		
		if(!unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop)
		{
		
			fprintf(logs, "unit %d is free for command... \n", i);
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);
				//if the command is build
				if(ct->getClass()==ccBuild)    
				{
					const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);

					//for each building
					for(int k=0; k<bct->getBuildingCount(); ++k)
					{
						const UnitType *building= bct->getBuilding(k);
						if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
						{
							if(isWarriorProducer(building))
							{
								if(aiInterface->checkCosts(building) == false) 
								{					    
									fprintf(logs, "costly... \n");
										return false;
								}
								Vec2i pos;
								Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
				    			if(findPosForBuilding(building, searchPos, pos)) 
								{
									aiInterface->giveCommand(i, bct, pos,building);
									fprintf(logs, "Command sent to AI for warrior producer building \n");

									return true;
								}

							}
						}
					}
				}
			}
		}
		else
		{
			fprintf(logs, "Unit already has command... \n");
			fflush(logs);
			continue;	
		}
	}
	return false;
}


bool Actions::isResourceProducer(const UnitType *building){
	for(int i= 0; i<building->getCostCount(); i++){
		if(building->getCost(i)->getAmount()<0){
			return true;
		}
	}
	return false;
}

bool Actions::buildResourceProducerBuilding()
{
	//for each unit
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		const Unit * unit = aiInterface->getMyUnit(i);

		if(!unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop)
		{			
			fprintf(logs, "unit %d is free for command... \n", i);
			//for each command
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);
				//if the command is build
				if(ct->getClass()==ccBuild)    
				{
					const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);

					//for each building
					for(int k=0; k<bct->getBuildingCount(); ++k)
					{
						const UnitType *building= bct->getBuilding(k);
						if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
						{
							if(isResourceProducer(building))
							{
								if(aiInterface->checkCosts(building) == false) 
								{					    
									fprintf(logs, "costly...\n", i);
										return false;
								}
								Vec2i pos;
								Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
				    				if(findPosForBuilding(building, searchPos, pos)) 
								{
									aiInterface->giveCommand(i, bct, pos,building);
									fprintf(logs, "Command sent to AI for Resource producer building \n");

									return true;
								}

							}
						}
					}
				}
			}
		}
		else
		{
			fprintf(logs, "Unit already has command... \n");
			fflush(logs);
			continue;	
		}
	}
	return false;
}



bool Actions::buildFarm()
{
    const UnitType *farm;// = NULL;
	//for all units
	for(int i=0; i<aiInterface->getMyFactionType()->getUnitTypeCount(); ++i)
	{
		const UnitType *ut= aiInterface->getMyFactionType()->getUnitType(i);
		//for all production commands
		for(int j=0; j<ut->getCommandTypeCount(); ++j)
		{
			const CommandType *ct= ut->getCommandType(j);
			if(ct->getClass()==ccProduce)
			{
				const UnitType *producedType= static_cast<const ProduceCommandType*>(ct)->getProducedUnit();

				//for all resources
				for(int k=0; k<producedType->getCostCount(); ++k)
				{
					const Resource *r= producedType->getCost(k);

					//find a food producer in the farm produced units
					if(r->getAmount()<0 && r->getType()->getClass()==rcConsumable)
					{
						farm = ut;
					}
					else
					{
						farm = NULL;
					}
				}
			}
		}
	}

	if(farm == NULL)
	{
		return false;
	}

	//for each unit
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		//for each command
		const UnitType *ut= aiInterface->getMyUnit(i)->getType();
		const Unit * unit = aiInterface->getMyUnit(i);
		if(!unit->anyCommand() || unit->getCurrCommand()->getCommandType()->getClass()==ccStop)
		{
			fprintf(logs, "unit %d is free for command... \n", i);
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);
				//if the command is build
				if(ct->getClass()==ccBuild)    
				{
					const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
					//for each building
					for(int k=0; k<bct->getBuildingCount(); ++k)
					{
						const UnitType *building= bct->getBuilding(k);
						if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
						{
							if(farm == building)
							{
								if(aiInterface->checkCosts(building) == false) 
								{					    
									fprintf(logs, "costly..... \n", i);
										return false;
								}
								Vec2i pos;
								Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
				    				if(findPosForBuilding(building, searchPos, pos)) 
								{
									aiInterface->giveCommand(i, bct, pos,building);
									fprintf(logs, "Command sent to AI for build farm \n");

									return true;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			fprintf(logs, "Unit already has command... \n");
			fflush(logs);
			continue;	
		}
	}
	return false;
}

void LearningAI::init(AiInterface *aiInterface, int useStartLocation) 
{
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
	lastSnapshot = takeSnapshot();
	getStateProbabilityDistribution(lastSnapshot);
	lastAct =semi_uniform_choose_action  (lastState);
	lastActionSucceed = action->selectAction(lastAct);
	//best_qvalue(int &state, int &best_action);

	interval = 5000;
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
		
		if((aiInterface->getTimer() % (interval * GameConstants::updateFps / 1000)) == 0) 
		{
//		printQvalues();
			aiInterface->printLog(4, " Inside update  :  " + intToStr(aiInterface->getTimer() )+"\n");
			fprintf(logs," Inside update  %d \n", aiInterface->getTimer() );
			fflush(logs);
			Snapshot * newSnapshot = takeSnapshot();
			float reward = getReward(lastSnapshot, newSnapshot,  lastActionSucceed)	 ;		
			free(lastSnapshot);
			lastSnapshot = newSnapshot;
			getStateProbabilityDistribution(newSnapshot);
			int newState ;
			int newAct =semi_uniform_choose_action  (newState);
			fprintf(logs, "after semi_uniform_choose_action\n");
			fflush(logs);
			update_qvalues(lastState, newState,lastAct,reward);
			fprintf(logs, "after update qvals\n");
			fflush(logs);
			lastActionSucceed  = action->selectAction(newAct);
			fprintf(logs, "after selectAction\n");
			fflush(logs);
			lastState = newState;
			lastAct = newAct;
			aiInterface->printLog(4, " state :  " + intToStr( lastState )+ " Action : " + intToStr( lastAct )+"\n");
			fprintf(logs," state  :  %d action : %d \n",  lastState, lastAct);
			fprintf(logs," out of update ...\n");
			fflush(logs);
		}
}

Snapshot* LearningAI :: takeSnapshot()
{
	//try
	{
		Field field ;
		Vec2i  pos;
		Snapshot *s = new  Snapshot(aiInterface , logs);
		s->getUpgradeCount();
		s->CheckIfBeingAttacked(pos, field);
		s->CheckIfStableBase();
		s->countDamagedUnits( );
		s->getKills();
		s->noOfWorkers = s->getCountOfClass(ucWorker);
		s->noOfWarriors = s->getCountOfClass(ucWarrior);
		s->noOfBuildings = s->getCountOfClass(ucBuilding);
		s->getResourceStatus();
		aiInterface->printLog(4, "Out of take snapshot \n");
		return s;
	}
/*	catch
	{
			fprintf(logs, "Exception in update function ");
			
	}*/
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

	stateProbabs[stateGatherResource] =(maxWorkers/(currSnapshot-> noOfWorkers +1) )* featureWeights[featureNoOfWorkers] + (maxResource/resourceAmt)  * featureWeights[featureRsourcesAmountWood];
		
	 //stateproduceUnit
	float totalWeight = (maxWorkers/(currSnapshot-> noOfWorkers  +1))* featureWeights[featureNoOfWorkers]  + (maxWarriors/(currSnapshot-> noOfWarriors+1)) * featureWeights[featureNoOfWarriors]  +  (maxBuildings /(currSnapshot-> noOfBuildings +1) ) * featureWeights[featureNoOfBuildings] ; 
	stateProbabs[stateProduceUnit] = totalWeight;
	
	//stateRepair
	stateProbabs[stateRepair] = currSnapshot-> damagedUnitCount * featureWeights[featureDamagedUnitCount]  ;
	
	//stateUpgrade
	int totalUnits = currSnapshot-> noOfWorkers   +  currSnapshot-> noOfWarriors  + currSnapshot-> noOfBuildings  ;
	 stateProbabs[stateUpgrade] = totalUnits * featureWeights[featureUpgradeCount] ;

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
		for(int i =0 ; i < length ; i++)
			values[i] = values[i]/sum;
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
	featureWeights[featureUpgradeCount]  = 10 ;
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

LearningAI :: ~LearningAI()
{
	FILE *  fp = fopen("Q_values.txt" , "w");
	for(int i = 0 ; i < NUM_OF_STATES ; i++)
	{
		for(int j = 0 ; j < NUM_OF_ACTIONS; j++)
		{
			fprintf(fp, "%lf\n ", qValues[i][j]);
		}
	}
	fclose(fp);
}

void LearningAI ::  init_qvalues()
{
		FILE *  fp = fopen("Q_values.txt" , "r");
		fprintf(logs, "Q values read from file..\n");
		if(fp != NULL)
		{
			rewind (fp);
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
  
	for (s=0;s< NUM_OF_STATES;s++)
		for (a=0;a< NUM_OF_ACTIONS ;a++)
			qValues[s][a] = -1 * INT_MAX;;

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

  fprintf(logs, "\nIn best Value !!! state %d Action %d\n", state , best_action);

  fflush(logs);

  return (qValues[state][best_act]);
}

void  LearningAI :: update_qvalues(int olds,int news,int action,double reward)
{
	double best_qval, qval;
	int newAct;
	best_qval = best_qvalue(news,newAct);
	fprintf(logs, "Previous Q value: State %d Action %d : %f \n",olds, action, qValues[olds][action] );
	qval = qValues[olds][action];	
	qValues[olds][action] =   (1 - BETA)*qval + BETA*(reward  + GAMMA*best_qval);      
	
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

#define DEFEND_SUCCESS_REWARD 200
#define KILL_REWARD 100
#define ARMY_READY_REWARD 100
#define ARMY_BUILD_SLOW_PENALTY -40
#define ATTACK_SLOW_PENALTY -20
#define ARMY_BACKWARD_PENALTY -40
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
#define ACTION_FAILED_PENALTY -50

float LearningAI :: getReward(Snapshot* preSnapshot, Snapshot* newSnapshot, bool actionSucceed)
{
    float reward = 0;
    if( actionSucceed == false )
	{
		reward += ACTION_FAILED_PENALTY;    
		fprintf(logs, "Action failed!! %d \n", lastAct);
	}
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
			else
				reward += ( MAX_RESOURCE - preSnapshot->resourcesAmount[ i ] ) * MORE_RESOURCE_REWARD;
		}
	fprintf(logs, "Reward by no  of resources count : %f \n", reward);
    }

    reward += (float)( newSnapshot->upgradeCount /(preSnapshot->upgradeCount+1) ) * MORE_UPGRADES_REWARD;
	fprintf(logs, "Reward by upgrade count : %f \n", reward);
	fprintf(logs,"REWARD is %f \n", reward);
    return reward;
}
}}//end namespace


