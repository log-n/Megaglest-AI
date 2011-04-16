// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "automation.h"

#include "main_menu.h"
#include "program.h"
#include "core_data.h"
#include "lang.h"
#include "util.h"
#include "renderer.h"
#include "main_menu.h"
#include "sound_renderer.h"
#include "components.h"
#include "metrics.h"
#include "stats.h"
#include "auto_test.h"
#include "game.h"
#include "logger.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class BattleEnd
// =====================================================

Automation::Automation(Program *program, GameSettings *gameSettings, const int numOfGames):
		ProgramState(program)
{
	count = 0;
	this->gameSettings = *gameSettings;
	this->numOfGames = numOfGames;
	this->program = program;

	const Metrics &metrics= Metrics::getInstance();
	Lang &lang= Lang::getInstance();
	int buttonWidth = 125;
	int xLocation = (metrics.getVirtualW() / 2) - (buttonWidth / 2);
	buttonExit.init(xLocation, 80, buttonWidth);
	buttonExit.setText(lang.get("Exit"));
}

bool Automation::isFinished()
{
	return count >= numOfGames;
}

Automation::~Automation() {
}

void Automation::update()
{
	if(!isFinished()){
		Game *game = new Game(program, &gameSettings, true);

		int X = 0;
		int Y = 0;
		SDL_GetMouseState(&X,&Y);
		game->setStartXY(X,Y);
		Logger::getInstance().setProgress(0);
		Logger::getInstance().setState("");


		SDL_PumpEvents();

		showCursor(true);
		SDL_PumpEvents();
		sleep(0);


		game->load();
		game->init();

		this->render();

		this->stats = *(game->runFast());
		count++;
		delete game;
	}
}

void Automation::render(){
	if(count == 0){
		return;
	}

	Renderer &renderer= Renderer::getInstance();
	TextRenderer2D *textRenderer= renderer.getTextRenderer();
	Lang &lang= Lang::getInstance();

	renderer.clearBuffers();
	renderer.reset2d();
	renderer.renderBackground(CoreData::getInstance().getBackgroundTexture());

	textRenderer->begin(CoreData::getInstance().getMenuFontNormal());
	textRenderer->render((intToStr(count) + " out of " + intToStr(numOfGames) + " Games.").c_str(), 100, 70);

	int lm= 20;
	int bm= 100;

	for(int i=0; i<stats.getFactionCount(); ++i){

		int textX= lm+160+i*100;
		int team= stats.getTeam(i) + 1;
		int kills= stats.getKills(i);
		int deaths= stats.getDeaths(i);
		int unitsProduced= stats.getUnitsProduced(i);
		int resourcesHarvested= stats.getResourcesHarvested(i);

		int score= kills*100 + unitsProduced*50 + resourcesHarvested/10;
		string controlString;

		if(stats.getPersonalityType(i) == fpt_Observer) {
			controlString= GameConstants::OBSERVER_SLOTNAME;
		}
		else {
			switch(stats.getControl(i)) {
			case ctCpuEasy:
				controlString= lang.get("CpuEasy");
				break;
			case ctCpu:
				controlString= lang.get("Cpu");
				break;
			case ctCpuUltra:
				controlString= lang.get("CpuUltra");
				break;
			case ctCpuMega:
				controlString= lang.get("CpuMega");
				break;
			case ctNetwork:
				controlString= lang.get("Network");
				break;
			case ctHuman:
				controlString= lang.get("Human");
				break;
			case ctLearningAI:
				controlString= "Learning AI";
				break;

			case ctNetworkCpuEasy:
				controlString= lang.get("NetworkCpuEasy");
				break;
			case ctNetworkCpu:
				controlString= lang.get("NetworkCpu");
				break;
			case ctNetworkCpuUltra:
				controlString= lang.get("NetworkCpuUltra");
				break;
			case ctNetworkCpuMega:
				controlString= lang.get("NetworkCpuMega");
				break;

			default:
				assert(false);
			};
		}

		if(stats.getControl(i)!=ctHuman && stats.getControl(i)!=ctNetwork ){
			controlString+=" x "+floatToStr(stats.getResourceMultiplier(i),1);
		}

		Vec3f color = stats.getPlayerColor(i);

		if(stats.getPlayerName(i) != "") {
			textRenderer->render(stats.getPlayerName(i).c_str(), textX, bm+400, false, &color);
		}
		else {
			textRenderer->render((lang.get("Player")+" "+intToStr(i+1)).c_str(), textX, bm+400,false, &color);
		}

		if(stats.getPersonalityType(i) == fpt_Observer) {
			textRenderer->render(lang.get("GameOver").c_str(), textX, bm+360);
		}
		else {
			textRenderer->render(stats.getVictory(i)? lang.get("Victory").c_str(): lang.get("Defeat").c_str(), textX, bm+360);
		}
		textRenderer->render(controlString, textX, bm+320);
		textRenderer->render(stats.getFactionTypeName(i), textX, bm+280);
		textRenderer->render(intToStr(team).c_str(), textX, bm+240);
		textRenderer->render(intToStr(kills).c_str(), textX, bm+200);
		textRenderer->render(intToStr(deaths).c_str(), textX, bm+160);
		textRenderer->render(intToStr(unitsProduced).c_str(), textX, bm+120);
		textRenderer->render(intToStr(resourcesHarvested).c_str(), textX, bm+80);
		textRenderer->render(intToStr(score).c_str(), textX, bm+20);
	}

	textRenderer->render(lang.get("Result"), lm, bm+360);
	textRenderer->render(lang.get("Control"), lm, bm+320);
	textRenderer->render(lang.get("Faction"), lm, bm+280);
	textRenderer->render(lang.get("Team"), lm, bm+240);
	textRenderer->render(lang.get("Kills"), lm, bm+200);
	textRenderer->render(lang.get("Deaths"), lm, bm+160);
	textRenderer->render(lang.get("UnitsProduced"), lm, bm+120);
	textRenderer->render(lang.get("ResourcesHarvested"), lm, bm+80);
	textRenderer->render(lang.get("Score"), lm, bm+20);

	textRenderer->end();

	textRenderer->begin(CoreData::getInstance().getMenuFontVeryBig());

	string header = stats.getDescription() + " - ";

	if(stats.getVictory(stats.getThisFactionIndex())){
		header += lang.get("Victory");
	}
	else{
		header += lang.get("Defeat");
	}

	textRenderer->render(header, lm+250, bm+550);

	textRenderer->end();

	if(isFinished()){
		renderer.renderButton(&buttonExit);
	}

	renderer.swapBuffers();
}

void Automation::mouseMove(int x, int y, const MouseState *ms){

	if(isFinished()){
		buttonExit.mouseMove(x, y);
	}
}

void Automation::mouseDownLeft(int x, int y){

	if(isFinished() && buttonExit.mouseClick(x,y)) {
		program->setState(new MainMenu(program));
	}
}

}}//end namespace
