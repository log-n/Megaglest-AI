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

#ifndef _GLEST_GAME_BATTLEEND_H_
#define _GLEST_GAME_BATTLEEND_H_

#include "program.h"
#include "stats.h"
#include "leak_dumper.h"
#include "game_settings.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class BattleEnd
//
///	ProgramState representing the end of the game
// =====================================================

class Automation: public ProgramState{
private:
	GraphicButton buttonExit;
	Program *program;
	GameSettings gameSettings;
	int numOfGames;
	Stats stats;
	int count;

public:
	Automation(Program *program, GameSettings *gameSettings, int numOfGames);
	~Automation();
	virtual void update();
	virtual void render();
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseMove(int x, int y, const MouseState *ms);
private:
	bool isFinished();
	void LogStats(FILE* log);
	void SaveStats();
};

}}//end namespace

#endif
