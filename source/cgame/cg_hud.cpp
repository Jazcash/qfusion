/*
Copyright (C) 2006 Pekka Lampila ("Medar"), Damien Deville ("Pb")
and German Garcia Fernandez ("Jal") for Chasseur de bots association.


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "cg_local.h"
#include "../client/client.h"
#include "../ref/frontend.h"
#include "../ui/uisystem.h"

static cvar_t *cg_showHUD;
static cvar_t *cg_specHUD;
static cvar_t *cg_clientHUD;

cvar_t *cg_showminimap;
cvar_t *cg_showitemtimers;

void CG_SC_ResetObituaries() {}
void CG_SC_Obituary() {}
void CG_UpdateHUDPostDraw() {}
void CG_ShowWeaponCross() {}
void CG_ClearHUDInputState() {}
void CG_ClearAwards() {}

void CG_InitHUD() {
	cg_showHUD =        Cvar_Get( "cg_showHUD", "1", CVAR_ARCHIVE );
	cg_clientHUD =      Cvar_Get( "cg_clientHUD", "", CVAR_ARCHIVE );
	cg_specHUD =        Cvar_Get( "cg_specHUD", "", CVAR_ARCHIVE );
}

void CG_ShutdownHUD() {
}

/*
* CG_DrawHUD
*/
void CG_DrawHUD() {
	if( !cg_showHUD->integer ) {
		return;
	}

	// if changed from or to spec, reload the HUD
	if( cg.specStateChanged ) {
		cg_specHUD->modified = cg_clientHUD->modified = true;
		cg.specStateChanged = false;
	}

	cvar_t *hud = ISREALSPECTATOR() ? cg_specHUD : cg_clientHUD;
	if( hud->modified ) {
		// TODO...
		hud->modified = false;
	}
}