
#include "Learning_AI.h"
#include "ai_interface.h"
#include "ai.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"
#include <cassert>
#include <time.h>

namespace Glest { namespace Game {

Actions::Actions(AiInterface * aiInterface , int startLoc , FILE * logs)
{
	this->logs = logs;
	this->aiInterface = aiInterface;
	this->startLoc = startLoc;
	clearResourceSpent();
	random.init(time(NULL));
}


bool Actions::selectAction(int actionNumber)
{
	fprintf(logs, "Selected action : %d\n", actionNumber);
	fflush(logs);

	bool ret = switchAction(actionNumber);

	return ret;
}


bool Actions:: switchAction(int actionNumber)
{
	switch(actionNumber)
	{
			case actSendScoutPatrol:
					return sendScoutPatrol();
					break;

			case actAttack:
				{
					Vec2i  pos;
					Field field;
					if(CheckAttackPosition(pos, field, INT_MAX)){
						return  massiveAttack(pos, field);
					}
					else{
						return false;
					}
					break;
				}

			case actDefend:
				{
					Vec2i  pos;
					Field field;
					if(CheckAttackPosition(pos, field, villageRadius + 10)){
						return massiveAttack(pos, field);
					}
					else{
						return false;
					}
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
					//return harvest(food) ;
					return buildFarm();
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

void Actions::UpdateSpentResource(const ProducibleType *pt)
{
	for(int i=0; i<pt->getCostCount(); ++i)
	{
		const ResourceType *rt= pt->getCost(i)->getType();
		int cost= pt->getCost(i)->getAmount();
		const Resource *r= aiInterface->getResource(rt);
		string resource_name = rt->getName ();

		if(cost < 0){
			cost *= -1;
		}

		if(resource_name == "gold" )
		{
			resourceSpent[gold] += cost ;
		}
		else if(resource_name == "wood"  )
		{
			resourceSpent[wood] += cost ;
		}
		else if(resource_name == "stone"  )
		{
			resourceSpent[stone] += cost ;
		}
		else if(resource_name == "food"  )
		{
			resourceSpent[food] += cost ;
		}
		else if(resource_name == "energy"  )
		{
			resourceSpent[energy] += cost ;
		}
		else if(resource_name == "housing"  )
		{
			resourceSpent[housing] += cost ;
		}

		if(cost < 0)
		{
			printf("Negative resource value ");
		}
	}

	for(int i=0; i< NUM_OF_RESOURCES; i++)
	{
		if(resourceSpent[i] > 0)
			printf("\t\t\t\t\t\t%d\t%d\n", i, resourceSpent[i]);
	}
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
		CommandResult result = aiInterface->giveCommand(unit, ccAttack, pos);
		if(result == crSuccess){
			aiInterface->printLog(2, "Scout patrol sent to: " + intToStr(pos.x)+","+intToStr(pos.y)+"\n");
			fprintf(logs, "Scout patrol sent to: %d , %d\n " ,pos.x, pos.y);
		}
		return false;
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
    CommandResult result;
    int fi;

    fi= aiInterface->getFactionIndex();
    pos= Vec2i(random.randRange(-villageRadius, villageRadius), random.randRange(-villageRadius, villageRadius)) +
		 aiInterface->getHomeLocation();

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    result= aiInterface->giveCommand(unitIndex, ccMove, pos);

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
			CommandResult result = aiInterface->giveCommand(unitIndex, hct, resPos);

			if(result == crSuccess){
				//assert(false);
				return true;
			}

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
	    						CommandResult result = aiInterface->giveCommand(i, rct, u->getPos());
								//aiInterface->printLog(3, "Repairing order issued");
								fprintf(logs, "Command sent to AI for repair \n");
								if(result == crSuccess){
									return true;
									//TODO ... add cost
								}
						}
				}
		}
	}
	return false;
}


bool Actions::produceOneUnit(UnitClass unitClass)
{
	//for each unit, produce it if possible
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		//for each command
		const Unit * u = aiInterface->getMyUnit(i);
		if(u->getCommandSize() > 0){
			continue;
		}
		const UnitType *ut= u->getType();
		for(int j=0; j<ut->getCommandTypeCount(); ++j)
		{
			const CommandType *ct= ut->getCommandType(j);
			vector<const CommandType *> possibleCmds;

			//if the command is produce
			if(ct->getClass()==ccProduce || ct->getClass()==ccMorph)
			{
				const UnitType *producedUnit= static_cast<const UnitType*>(ct->getProduced());
				bool produceIt= false;
				//if the unit is from the right class
				if(producedUnit->isOfClass(unitClass))
				{
					if(aiInterface->reqsOk(ct) && aiInterface->reqsOk(producedUnit) && aiInterface->checkCosts(producedUnit))
					{
						possibleCmds.push_back(ct);
					}
				}
			}
			if(possibleCmds.size() > 0)
			{
				int randIndex = random.randRange(0, possibleCmds.size()-1);
				CommandResult result = aiInterface->giveCommand(i, possibleCmds.at(randIndex));
				fprintf(logs, "Command sent to AI for produce unit \n");

				if(result == crSuccess){
					UpdateSpentResource(static_cast<const UnitType*>(ct->getProduced()));
					return true;
				}
			}
		}
	}
	return false;
}

bool Actions::upgrade()
{
	  int cnt = 0;
		//for each unit
		for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
		{
			//for each command
			fprintf(logs, "Upgrade for unit %d \n", i);
			const UnitType *ut= aiInterface->getMyUnit(i)->getType();
			cnt = 0 ;
			for(int j=0; j<ut->getCommandTypeCount(); ++j)
			{
				const CommandType *ct= ut->getCommandType(j);

				//if the command is upgrade
				if(ct->getClass()==ccUpgrade)
				{
					cnt ++;
					const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(ct);
					const UpgradeType *producedUpgrade= uct->getProducedUpgrade();
					if(aiInterface->reqsOk(uct))
					{
							SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
							CommandResult result = aiInterface->giveCommand(i, uct);
							if(result == crSuccess){
								UpdateSpentResource(producedUpgrade);
								return true;
							}
					}
					else
					{
						fprintf(logs, " Request command not ok...\n");
					}
				}			
			}
		if(cnt == 0 )
			fprintf(logs, "No unit for upgrade available... !\n");
		}
		
		return false;
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
				for(int j=0; j<ut->getCommandTypeCount(); ++j)
				{
					fprintf(logs, "unit %d is free for building...\n", i);
					const CommandType *ct= ut->getCommandType(j);
					//if the command is build
					if(ct->getClass()==ccBuild)
					{
						fprintf(logs, "unit %d is free for building...\n", i);
						const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
						//for each building
						for(int k=0; k<bct->getBuildingCount(); ++k)
						{
							const UnitType *building= bct->getBuilding(k);
							if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
							{
								fprintf(logs, "%d building can be built....  \n", k);
								if(building->hasSkillClass(scAttack))
								{
									fprintf(logs, "%d is Defensive building  \n", k);
									if(aiInterface->checkCosts(building) == false)
									{
										fprintf(logs, "constly... to build..\n");
											return false;
									}
									Vec2i pos;
									Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
			    					if(findPosForBuilding(building, searchPos, pos))
									{
										CommandResult result = aiInterface->giveCommand(i, bct, pos,building);

										if(result == crSuccess){
											UpdateSpentResource(building);
											fprintf(logs, "Command sent to AI for build defensive building \n");
											return true;
										}
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
			bool currentlyHarvesting = (unit->getCurrSkill()->getClass() == scHarvest);
			if(currentlyHarvesting)
			{

				for(int j=0; j<ut->getCommandTypeCount(); ++j)
				{
					fprintf(logs, "unit %d is free for building...\n", i);
					const CommandType *ct= ut->getCommandType(j);
					//if the command is build
					if(ct->getClass()==ccBuild)
					{
						fprintf(logs, "unit %d is free for building...\n", i);
						const BuildCommandType *bct= static_cast<const BuildCommandType*>(ct);
						//for each building
						for(int k=0; k<bct->getBuildingCount(); ++k)
						{
							const UnitType *building= bct->getBuilding(k);
							if(aiInterface->reqsOk(bct) && aiInterface->reqsOk(building))
							{
								fprintf(logs, "%d building can be built....  \n", k);
								if(building->hasSkillClass(scAttack))
								{
									fprintf(logs, "%d is Defensive building  \n", k);
									if(aiInterface->checkCosts(building) == false)
									{
										fprintf(logs, "constly... to build..\n");
											return false;
									}
									Vec2i pos;
									Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
			    					if(findPosForBuilding(building, searchPos, pos))
									{
										CommandResult result = aiInterface->giveCommand(i, bct, pos,building);
										if(result == crSuccess){
											UpdateSpentResource(building);
											fprintf(logs, "Command sent to AI for build defensive building \n");
											return true;
										}
									}

								}
							}
						}
					}
				}			
			}
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
									fprintf(logs, "Warrior prodecer building!!!\n");
									if(aiInterface->checkCosts(building) == false)
									{
										fprintf(logs, "costly... \n");
											return false;
									}
									Vec2i pos;
									Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
				    				if(findPosForBuilding(building, searchPos, pos))
									{
										CommandResult result = aiInterface->giveCommand(i, bct, pos,building);
										if(result == crSuccess){
											UpdateSpentResource(building);
											fprintf(logs, "Command sent to AI for warrior producer building \n");
											return true;
										}
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
			bool currentlyHarvesting = (unit->getCurrSkill()->getClass() == scHarvest);
			if(currentlyHarvesting)
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
									fprintf(logs, "Warrior prodecer building!!!\n");
									if(aiInterface->checkCosts(building) == false)
									{
										fprintf(logs, "costly... \n");
											return false;
									}
									Vec2i pos;
									Vec2i searchPos= getRandomHomePosition() +  aiInterface->getHomeLocation();
				    				if(findPosForBuilding(building, searchPos, pos))
									{
										CommandResult result = aiInterface->giveCommand(i, bct, pos,building);
										if(result == crSuccess){
											UpdateSpentResource(building);
											fprintf(logs, "Command sent to AI for warrior producer building \n");
											return true;
										}
									}
								}
							}
						}
					}
				}
			}
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
									CommandResult result =aiInterface->giveCommand(i, bct, pos,building);
									if(result == crSuccess){
										UpdateSpentResource(building);
										fprintf(logs, "Command sent to AI for Resource producer building \n");
										return true;
									}
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

const UnitType* Actions::getFarm()
{
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
						return ut;
					}
				}
			}
		}
	}

	return NULL;
}

bool Actions::buildFarm()
{
    const UnitType *farm = getFarm();
	//for all units

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
									CommandResult result = aiInterface->giveCommand(i, bct, pos,building);

									if(result == crSuccess){
										UpdateSpentResource(building);
										fprintf(logs, "Command sent to AI for build farm \n");
										return true;
									}
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

void Actions:: clearResourceSpent()
{
	for(int i = 0 ; i < NUM_OF_RESOURCES; i++)
	{
		resourceSpent[i] = 0;
	}
}

void Actions:: saveResourceAmount(int resourcesAmount[NUM_OF_RESOURCES])
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

	for(int i = 0 ; i < NUM_OF_RESOURCES; i++)
	{
			if(resourcesAmount[i] > 0)
				printf("%d\t%d\n", i, resourcesAmount[i]);
	}
}

}}
