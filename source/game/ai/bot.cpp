#include "bot.h"
#include "navigation/AasWorld.h"
#include "navigation/NavMeshManager.h"
#include <algorithm>

#ifndef _MSC_VER
// Allow getting an address of not initialized yet field movementModule.movementState.entityPhysicsState.
// Saving this address for further use is legal, the field is not going to be used right now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

Bot::Bot( edict_t *self_, float skillLevel_ )
	: Ai( self_
		, &botPlanner
		, AiAasRouteCache::NewInstance( &travelFlags[0] )
		, &movementModule.movementState.entityPhysicsState
		, PREFERRED_TRAVEL_FLAGS
		, ALLOWED_TRAVEL_FLAGS )
	, weightConfig( self_ )
	, perceptionManager( self_ )
	, threatTracker( self_, this, skillLevel_ )
	, botPlanner( this, skillLevel_ )
	, skillLevel( skillLevel_ )
	, selectedEnemies( self_ )
	, lostEnemies( self_ )
	, weaponsUsageModule( this )
	, tacticalSpotsCache( self_ )
	, roamingManager( self_ )
	, grabItemGoal( this )
	, killEnemyGoal( this )
	, runAwayGoal( this )
	, reactToDangerGoal( this )
	, reactToThreatGoal( this )
	, reactToEnemyLostGoal( this )
	, attackOutOfDespairGoal( this )
	, roamGoal( this )
	, genericRunToItemAction( this )
	, pickupItemAction( this )
	, waitForItemAction( this )
	, killEnemyAction( this )
	, advanceToGoodPositionAction( this )
	, retreatToGoodPositionAction( this )
	, steadyCombatAction( this )
	, gotoAvailableGoodPositionAction( this )
	, attackFromCurrentPositionAction( this )
	, attackAdvancingToTargetAction( this )
	, genericRunAvoidingCombatAction( this )
	, startGotoCoverAction( this )
	, takeCoverAction( this )
	, startGotoRunAwayTeleportAction( this )
	, doRunAwayViaTeleportAction( this )
	, startGotoRunAwayJumppadAction( this )
	, doRunAwayViaJumppadAction( this )
	, startGotoRunAwayElevatorAction( this )
	, doRunAwayViaElevatorAction( this )
	, stopRunningAwayAction( this )
	, dodgeToSpotAction( this )
	, turnToThreatOriginAction( this )
	, turnToLostEnemyAction( this )
	, startLostEnemyPursuitAction( this )
	, stopLostEnemyPursuitAction( this )
	, movementModule( this )
	, vsayTimeout( level.time + 10000 )
	, lastTouchedTeleportAt( 0 )
	, lastTouchedJumppadAt( 0 )
	, lastTouchedElevatorAt( 0 )
	, lastKnockbackAt( 0 )
	, similarWorldStateInstanceId( 0 )
	, lastItemSelectedAt( 0 )
	, noItemAvailableSince( 0 )
	, lastBlockedNavTargetReportedAt( 0 )
	, keptInFovPoint( self_ )
	, lastChosenLostOrHiddenEnemy( nullptr )
	, lastChosenLostOrHiddenEnemyInstanceId( 0 )
	, selectedNavEntity( nullptr, 0, 0, 0 )
	, itemsSelector( self ) {
	self->r.client->movestyle = GS_CLASSICBUNNY;
	// Enable skimming for bots (since it is useful and should not be noticed from a 3rd person POV).
	self->r.client->ps.pmove.stats[PM_STAT_FEATURES] &= PMFEAT_CORNERSKIMMING;
	SetTag( self->r.client->netname );
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

Bot::~Bot() {
	AiAasRouteCache::ReleaseInstance( routeCache );

	if( navMeshQuery ) {
		AiNavMeshManager::Instance()->FreeQuery( navMeshQuery );
	}
}



void Bot::UpdateKeptInFovPoint() {
	if( GetMiscTactics().shouldRushHeadless ) {
		keptInFovPoint.Deactivate();
		return;
	}

	if( selectedEnemies.AreValid() ) {
		Vec3 origin( selectedEnemies.ClosestEnemyOrigin( self->s.origin ) );
		if( !GetMiscTactics().shouldKeepXhairOnEnemy ) {
			if( !selectedEnemies.HaveQuad() && !selectedEnemies.HaveCarrier() ) {
				float distanceThreshold = 768.0f + 1024.0f * selectedEnemies.MaxThreatFactor();
				distanceThreshold *= 0.5f + 0.5f * self->ai->botRef->GetEffectiveOffensiveness();
				if( origin.SquareDistanceTo( self->s.origin ) > distanceThreshold * distanceThreshold ) {
					return;
				}
			}
		}

		keptInFovPoint.Update( origin, selectedEnemies.InstanceId() );
		return;
	}

	unsigned timeout = GetMiscTactics().shouldKeepXhairOnEnemy ? 2000 : 1000;
	if( GetMiscTactics().willRetreat ) {
		timeout = ( timeout * 3u ) / 2u;
	}

	if( const Enemy *lostOrHiddenEnemy = threatTracker.ChooseLostOrHiddenEnemy( timeout ) ) {
		if( !lastChosenLostOrHiddenEnemy ) {
			lastChosenLostOrHiddenEnemyInstanceId++;
		} else if( lastChosenLostOrHiddenEnemy->ent != lostOrHiddenEnemy->ent ) {
			lastChosenLostOrHiddenEnemyInstanceId++;
		}

		Vec3 origin( lostOrHiddenEnemy->LastSeenOrigin() );
		if( !GetMiscTactics().shouldKeepXhairOnEnemy ) {
			float distanceThreshold = 384.0f;
			if( lostOrHiddenEnemy->ent ) {
				distanceThreshold += 1024.0f * selectedEnemies.ComputeThreatFactor( lostOrHiddenEnemy->ent );
			}
			distanceThreshold *= 0.5f + 0.5f * self->ai->botRef->GetEffectiveOffensiveness();
			if( origin.SquareDistanceTo( self->s.origin ) > distanceThreshold * distanceThreshold ) {
				lastChosenLostOrHiddenEnemy = nullptr;
				return;
			}
		}

		lastChosenLostOrHiddenEnemy = lostOrHiddenEnemy;
		keptInFovPoint.Update( origin, lastChosenLostOrHiddenEnemyInstanceId );
		return;
	}

	lastChosenLostOrHiddenEnemy = nullptr;

	if( const auto *hurtEvent = threatTracker.GetValidHurtEvent() ) {
		keptInFovPoint.Activate( hurtEvent->possibleOrigin, (unsigned)hurtEvent->lastHitTimestamp );
		return;
	}

	keptInFovPoint.Deactivate();
}

void Bot::TouchedOtherEntity( const edict_t *entity ) {
	if( !entity->classname ) {
		return;
	}

	// Cut off string comparisons by doing these cheap tests first

	// Only triggers are interesting for following code
	if( entity->r.solid != SOLID_TRIGGER ) {
		return;
	}
	// Items should be handled by TouchedNavEntity() or skipped (if it is not a current nav entity)
	if( entity->item ) {
		return;
	}

	if( !Q_stricmp( entity->classname, "trigger_push" ) ) {
		lastTouchedJumppadAt = level.time;
		movementModule.ActivateJumppadState( entity );
		return;
	}

	if( !Q_stricmp( entity->classname, "trigger_teleport" ) ) {
		lastTouchedTeleportAt = level.time;
		return;
	}

	if( !Q_stricmp( entity->classname, "func_plat" ) ) {
		lastTouchedElevatorAt = level.time;
		return;
	}
}

bool Bot::HasJustPickedGoalItem() const {
	if( lastNavTargetReachedAt < prevThinkAt ) {
		return false;
	}
	if( !lastReachedNavTarget ) {
		return false;
	}
	if( !lastReachedNavTarget->IsBasedOnNavEntity( prevSelectedNavEntity ) ) {
		return false;
	}
	return true;
}

void Bot::CheckTargetProximity() {
	if( !NavTargetAasAreaNum() ) {
		return;
	}

	if( !IsCloseToNavTarget( 128.0f ) ) {
		return;
	}

	// Save the origin for the roaming manager to avoid its occasional modification in the code below
	if( !TryReachNavTargetByProximity() ) {
		return;
	}

	OnNavTargetTouchHandled();
}

const SelectedNavEntity &Bot::GetOrUpdateSelectedNavEntity() {
	if( selectedNavEntity.IsValid() && !selectedNavEntity.IsEmpty() ) {
		return selectedNavEntity;
	}

	// Force an update using the currently selected nav entity
	// (it's OK if it's not valid) as a reference info for selection
	ForceSetNavEntity( itemsSelector.SuggestGoalNavEntity( selectedNavEntity ) );
	// Return the modified selected nav entity
	return selectedNavEntity;
}

void Bot::ForceSetNavEntity( const SelectedNavEntity &selectedNavEntity_ ) {
	// Use direct access to the field to skip assertion
	this->prevSelectedNavEntity = this->selectedNavEntity.navEntity;
	this->selectedNavEntity = selectedNavEntity_;

	if( !selectedNavEntity.IsEmpty() ) {
		self->ai->botRef->lastItemSelectedAt = level.time;
	} else if( self->ai->botRef->lastItemSelectedAt >= self->ai->botRef->noItemAvailableSince ) {
		self->ai->botRef->noItemAvailableSince = level.time;
	}
}

Vec3 Bot::GetNewViewAngles( const vec3_t oldAngles, const Vec3 &desiredDirection,
							unsigned frameTime, float angularSpeedMultiplier ) const {
	// A hack for evil hard bots aiming
	if( self->ai->botRef->Skill() > 0.33f ) {
		return Ai::GetNewViewAngles( oldAngles, desiredDirection, frameTime, angularSpeedMultiplier );
	}

	const auto &selectedEnemies = GetSelectedEnemies();
	if( !selectedEnemies.AreValid() || !selectedEnemies.ArePotentiallyHittable() ) {
		return Ai::GetNewViewAngles( oldAngles, desiredDirection, frameTime, angularSpeedMultiplier );
	}

	if( GetMiscTactics().shouldKeepXhairOnEnemy ) {
		angularSpeedMultiplier *= 1.0f + 0.33f * ( Skill() - 0.33f );
	}

	// Another hack for shooting while falling
	if( EntityPhysicsState()->IsHighAboveGround() ) {
		// It is intended for crazy EB/RL/SW shots, not for aimbot LG tracking while falling
		if( !IsBuiltinWeaponContinuousFire( self->r.client->ps.stats[STAT_WEAPON] ) ) {
			const float oldPitchSpeed = angularViewSpeed[PITCH];
			const_cast<float *>( angularViewSpeed )[PITCH] *= 1.33f;
			Vec3 result( Ai::GetNewViewAngles( oldAngles, desiredDirection, frameTime, angularSpeedMultiplier ) );
			const_cast<float *>( angularViewSpeed )[PITCH] = oldPitchSpeed;
			return result;
		}
	}

	return Ai::GetNewViewAngles( oldAngles, desiredDirection, frameTime, angularSpeedMultiplier );
}

void Bot::EnableAutoAlert( const AiAlertSpot &alertSpot, AlertCallback callback, void *receiver ) {
	// First check duplicate ids. Fail on error since callers of this method are internal.
	for( unsigned i = 0; i < alertSpots.size(); ++i ) {
		if( alertSpots[i].id == alertSpot.id ) {
			FailWith( "Duplicated alert spot (id=%d)\n", alertSpot.id );
		}
	}

	if( alertSpots.size() == alertSpots.capacity() ) {
		FailWith( "Can't add an alert spot (id=%d)\n: too many spots", alertSpot.id );
	}

	alertSpots.emplace_back( AlertSpot( alertSpot, callback, receiver ) );
}

void Bot::DisableAutoAlert( int id ) {
	for( unsigned i = 0; i < alertSpots.size(); ++i ) {
		if( alertSpots[i].id == id ) {
			alertSpots.erase( alertSpots.begin() + i );
			return;
		}
	}

	FailWith( "Can't find alert spot by id %d\n", id );
}

void Bot::CheckAlertSpots( const StaticVector<uint16_t, MAX_CLIENTS> &visibleTargets ) {
	float scores[MAX_ALERT_SPOTS];

	edict_t *const gameEdicts = game.edicts;
	// First compute scores (good for instruction cache)
	for( unsigned i = 0; i < alertSpots.size(); ++i ) {
		float score = 0.0f;
		const auto &alertSpot = alertSpots[i];
		const float squareRadius = alertSpot.radius * alertSpot.radius;
		const float invRadius = 1.0f / alertSpot.radius;
		for( uint16_t entNum: visibleTargets ) {
			edict_t *ent = gameEdicts + entNum;
			float squareDistance = DistanceSquared( ent->s.origin, alertSpot.origin.Data() );
			if( squareDistance > squareRadius ) {
				continue;
			}
			float distance = Q_RSqrt( squareDistance + 0.001f );
			score += 1.0f - distance * invRadius;
			// Put likely case first
			if( !( ent->s.effects & EF_CARRIER ) ) {
				score *= alertSpot.regularEnemyInfluenceScale;
			} else {
				score *= alertSpot.carrierEnemyInfluenceScale;
			}
		}
		// Clamp score by a max value
		clamp_high( score, 3.0f );
		// Convert score to [0, 1] range
		score /= 3.0f;
		// Get a square root of score (values closer to 0 gets scaled more than ones closer to 1)
		// Note: preserving zero value is important, otherwise an infinite alert is observed.
		if( score ) {
			score = 1.0f / sqrtf( score );
		}
		// Sanitize
		clamp( score, 0.0f, 1.0f );
		scores[i] = score;
	}

	// Then call callbacks
	const int64_t levelTime = level.time;
	for( unsigned i = 0; i < alertSpots.size(); ++i ) {
		auto &alertSpot = alertSpots[i];
		uint64_t nonReportedFor = (uint64_t)( levelTime - alertSpot.lastReportedAt );
		if( nonReportedFor >= 1000 ) {
			alertSpot.lastReportedScore = 0.0f;
		}

		// Since scores are sanitized, they are in range [0.0f, 1.0f], and abs(scoreDelta) is in range [-1.0f, 1.0f];
		float scoreDelta = scores[i] - alertSpot.lastReportedScore;
		if( scoreDelta >= 0 ) {
			if( nonReportedFor >= 1000 - scoreDelta * 500 ) {
				alertSpot.Alert( this, scores[i] );
			}
		} else {
			if( nonReportedFor >= 500 - scoreDelta * 500 ) {
				alertSpot.Alert( this, scores[i] );
			}
		}
	}
}

void Bot::ChangeWeapons( const SelectedWeapons &selectedWeapons_ ) {
	if( selectedWeapons_.BuiltinFireDef() != nullptr ) {
		self->r.client->ps.stats[STAT_PENDING_WEAPON] = selectedWeapons_.BuiltinWeaponNum();
	}
	if( selectedWeapons_.ScriptFireDef() != nullptr ) {
		GT_asSelectScriptWeapon( self->r.client, selectedWeapons_.ScriptWeaponNum() );
	}
}

//==========================================
// BOT_DMclass_VSAYmessages
//==========================================
void Bot::SayVoiceMessages() {
	if( GS_MatchState() != MATCH_STATE_PLAYTIME ) {
		return;
	}

	if( self->snap.damageteam_given > 25 ) {
		if( rand() & 1 ) {
			if( rand() & 1 ) {
				G_BOTvsay_f( self, "oops", true );
			} else {
				G_BOTvsay_f( self, "sorry", true );
			}
		}
		return;
	}

	if( vsayTimeout > level.time ) {
		return;
	}

	if( GS_MatchDuration() && game.serverTime + 4000 > GS_MatchEndTime() ) {
		vsayTimeout = game.serverTime + ( 1000 + ( GS_MatchEndTime() - game.serverTime ) );
		if( rand() & 1 ) {
			G_BOTvsay_f( self, "goodgame", false );
		}
		return;
	}

	vsayTimeout = (int64_t)( level.time + ( ( 8 + random() * 12 ) * 1000 ) );

	// the more bots, the less vsays to play
	if( random() > 0.1 + 1.0f / game.numBots ) {
		return;
	}

	if( GS_TeamBasedGametype() && !GS_InvidualGameType() ) {
		if( self->health < 20 && random() > 0.3 ) {
			G_BOTvsay_f( self, "needhealth", true );
			return;
		}

		if( ( self->s.weapon == 0 || self->s.weapon == 1 ) && random() > 0.7 ) {
			G_BOTvsay_f( self, "needweapon", true );
			return;
		}

		if( self->r.client->resp.armor < 10 && random() > 0.8 ) {
			G_BOTvsay_f( self, "needarmor", true );
			return;
		}
	}

	// NOT team based here

	if( random() > 0.2 ) {
		return;
	}

	switch( (int)brandom( 1, 8 ) ) {
		default:
			break;
		case 1:
			G_BOTvsay_f( self, "roger", false );
			break;
		case 2:
			G_BOTvsay_f( self, "noproblem", false );
			break;
		case 3:
			G_BOTvsay_f( self, "yeehaa", false );
			break;
		case 4:
			G_BOTvsay_f( self, "yes", false );
			break;
		case 5:
			G_BOTvsay_f( self, "no", false );
			break;
		case 6:
			G_BOTvsay_f( self, "booo", false );
			break;
		case 7:
			G_BOTvsay_f( self, "attack", false );
			break;
		case 8:
			G_BOTvsay_f( self, "ok", false );
			break;
	}
}


//==========================================
// BOT_DMClass_BlockedTimeout
// the bot has been blocked for too long
//==========================================
void Bot::OnBlockedTimeout() {
	self->health = 0;
	blockedTimeoutAt = level.time + BLOCKED_TIMEOUT;
	self->die( self, self, self, 100000, vec3_origin );
	G_Killed( self, self, self, 999, vec3_origin, MOD_SUICIDE );
	self->nextThink = level.time + 1;
}

//==========================================
// BOT_DMclass_DeadFrame
// ent is dead = run this think func
//==========================================
void Bot::GhostingFrame() {
	selectedEnemies.Invalidate();

	lastChosenLostOrHiddenEnemy = nullptr;

	botPlanner.ClearGoalAndPlan();

	movementModule.Reset();

	blockedTimeoutAt = level.time + BLOCKED_TIMEOUT;
	self->nextThink = level.time + 100;

	// Release this quite huge object while it is not really needed.
	// We have decided avoid its preallocation and ignore allocation failures.
	if( navMeshQuery ) {
		AiNavMeshManager::Instance()->FreeQuery( navMeshQuery );
		navMeshQuery = nullptr;
	}

	// wait 4 seconds after entering the level
	if( self->r.client->level.timeStamp + 4000 > level.time || !level.canSpawnEntities ) {
		return;
	}

	if( self->r.client->team == TEAM_SPECTATOR ) {
		// try to join a team
		// note that G_Teams_JoinAnyTeam is quite slow so only call it per frame
		if( !self->r.client->queueTimeStamp && self == level.think_client_entity ) {
			G_Teams_JoinAnyTeam( self, false );
		}

		if( self->r.client->team == TEAM_SPECTATOR ) { // couldn't join, delay the next think
			self->nextThink = level.time + 2000 + (int)( 4000 * random() );
		} else {
			self->nextThink = level.time + 1;
		}
		return;
	}

	BotInput botInput;
	botInput.isUcmdSet = true;
	// ask for respawn if the minimum bot respawning time passed
	if( level.time > self->deathTimeStamp + 3000 ) {
		botInput.SetAttackButton( true );
	}

	CallGhostingClientThink( botInput );
}

void Bot::CallGhostingClientThink( const BotInput &input ) {
	usercmd_t ucmd;
	// Shut an analyzer up
	memset( &ucmd, 0, sizeof( usercmd_t ) );
	input.CopyToUcmd( &ucmd );
	// set approximate ping and show values
	ucmd.serverTimeStamp = game.serverTime;
	ucmd.msec = (uint8_t)game.frametime;
	self->r.client->r.ping = 0;

	ClientThink( self, &ucmd, 0 );
}

void Bot::OnRespawn() {
	ResetNavigation();
}

void Bot::Think() {
	// Call superclass method first
	Ai::Think();

	if( IsGhosting() ) {
		return;
	}

	UpdateKeptInFovPoint();

	// TODO: Let the weapons usage module decide?
	if( CanChangeWeapons() ) {
		weaponsUsageModule.Think( botPlanner.cachedWorldState );
		ChangeWeapons( weaponsUsageModule.GetSelectedWeapons() );
	}
}

//==========================================
// BOT_DMclass_RunFrame
// States Machine & call client movement
//==========================================
void Bot::Frame() {
	// Call superclass method first
	Ai::Frame();

	if( IsGhosting() ) {
		GhostingFrame();
	} else {
		ActiveFrame();
	}
}

void Bot::ActiveFrame() {
	//get ready if in the game
	if( GS_MatchState() <= MATCH_STATE_WARMUP && !IsReady() && self->r.client->teamstate.timeStamp + 4000 < level.time ) {
		G_Match_Ready( self );
	}

	// Always calls Frame() and calls Think() if needed
	perceptionManager.Update();
	// Same as for the perception manager
	threatTracker.Update();

	weaponsUsageModule.Frame( botPlanner.cachedWorldState );

	BotInput botInput;
	// Might modify botInput
	movementModule.Frame( &botInput );

	roamingManager.CheckSpotsProximity();
	CheckTargetProximity();

	// Might modify botInput
	if( ShouldAttack() ) {
		weaponsUsageModule.TryFire( &botInput );
	}

	// Apply modified botInput
	movementModule.ApplyInput( &botInput );
	CallActiveClientThink( botInput );

	SayVoiceMessages();
}

void Bot::CallActiveClientThink( const BotInput &input ) {
	usercmd_t ucmd;
	// Shut an analyzer up
	memset( &ucmd, 0, sizeof( usercmd_t ) );
	input.CopyToUcmd( &ucmd );

	//set up for pmove
	for( int i = 0; i < 3; i++ )
		ucmd.angles[i] = (short)ANGLE2SHORT( self->s.angles[i] ) - self->r.client->ps.pmove.delta_angles[i];

	VectorSet( self->r.client->ps.pmove.delta_angles, 0, 0, 0 );

	// set approximate ping and show values
	ucmd.msec = (uint8_t)game.frametime;
	ucmd.serverTimeStamp = game.serverTime;

	ClientThink( self, &ucmd, 0 );
	self->nextThink = level.time + 1;
}

void Bot::OnMovementToNavTargetBlocked() {
	if( !selectedNavEntity.IsValid() || selectedNavEntity.IsEmpty() ) {
		return;
	}

	// If a new nav target is set in blocked state, the bot remains blocked
	// for few millis since the ground acceleration is finite.
	// Prevent classifying just set nav targets as ones that have led to blocking.
	if( level.time - lastBlockedNavTargetReportedAt < 400 ) {
		return;
	}

	lastBlockedNavTargetReportedAt = level.time;

	// Force replanning
	botPlanner.ClearGoalAndPlan();

	const auto *navEntity = selectedNavEntity.GetNavEntity();
	if( !navEntity ) {
		// It is very likely that the nav entity was based on a tactical spot.
		// Disable all nearby tactical spots for the origin
		roamingManager.DisableSpotsInRadius( navEntity->Origin(), 144.0f );
		selectedNavEntity.InvalidateNextFrame();
		return;
	}

	itemsSelector.MarkAsDisabled( *navEntity, 4000 );
	selectedNavEntity.InvalidateNextFrame();
}
