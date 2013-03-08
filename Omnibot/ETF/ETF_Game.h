////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 00:12:05 -0500 (Sat, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __ETF_GAME_H__
#define __ETF_GAME_H__

class Waypoint;
class gmMachine;
class gmTableObject;

#include "IGame.h"
#include "TF_Game.h"

// class: ETF_Game
//		Game Type for Open Fortress.
class ETF_Game : public TF_Game
{
public:
	bool Init();

	//void AddBot(const std::string &_name, int _team, int _class, const std::string _profile, bool _createnow);

	virtual Client *CreateGameClient();

	int GetVersionNum() const;
	const char *GetDLLName() const;
	const char *GetModSubFolder() const;
	const char *GetGameName() const;
	const char *GetNavSubfolder() const;
	const char *GetScriptSubfolder() const;
	const char *GetGameDatabaseAbbrev() const { return "etf"; }
	virtual eNavigatorID GetDefaultNavigator() const { return NAVID_NAVMESH; }

	void GetTeamEnumeration(const IntEnum *&_ptr, int &num);

	ETF_Game() { }
	virtual ~ETF_Game() {}
protected:

	void GetGameVars(GameVars &_gamevars);

	// Script support.
	void InitScriptBinds(gmMachine *_machine);
	void InitScriptEntityFlags(gmMachine *_machine, gmTableObject *_table);

	static const float ETF_GetEntityClassTraceOffset(const int _class, const BitFlag64 &_entflags);
	static const float ETF_GetEntityClassAimOffset(const int _class, const BitFlag64 &_entflags);
};

#endif
