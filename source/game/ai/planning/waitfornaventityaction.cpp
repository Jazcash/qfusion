#include "planninglocal.h"
#include "../bot.h"

void WaitForNavEntityActionRecord::Activate() {
	BotActionRecord::Activate();
	Self()->GetMiscTactics().shouldMoveCarefully = true;
	Self()->GetMiscTactics().PreferAttackRatherThanRun();
	Self()->SetNavTarget( navEntity );
	Self()->SetCampingSpot( AiCampingSpot( navEntity->Origin(), GOAL_PICKUP_ACTION_RADIUS, 0.5f ) );
}

void WaitForNavEntityActionRecord::Deactivate() {
	BotActionRecord::Deactivate();
	Self()->ResetCampingSpot();
	Self()->ResetNavTarget();
}

AiActionRecord::Status WaitForNavEntityActionRecord::UpdateStatus( const WorldState &currWorldState ) {
	if( currWorldState.HasJustPickedGoalItemVar() ) {
		Debug( "Goal item has been just picked up\n" );
		return COMPLETED;
	}

	const auto &currSelectedNavEntity = Self()->GetSelectedNavEntity();
	if( !navEntity->IsBasedOnNavEntity( currSelectedNavEntity.GetNavEntity() ) ) {
		Debug( "Nav entity does no longer match current selected nav entity\n" );
		return INVALID;
	}
	if( !navEntity->SpawnTime() ) {
		Debug( "Illegal nav entity spawn time (looks like it has been invalidated)\n" );
		return INVALID;
	}
	// Wait duration is too long (more than it was estimated)
	const auto waitDuration = (uint64_t)( navEntity->SpawnTime() - level.time );
	if( waitDuration > navEntity->MaxWaitDuration() ) {
		constexpr auto *format = "Wait duration %" PRIu64 " is too long "
								 "(the maximum allowed value for a nav entity is %" PRIu64 ")\n";
		Debug( format, waitDuration, navEntity->MaxWaitDuration() );
		return INVALID;
	}
	if( currWorldState.DistanceToNavTarget() > GOAL_PICKUP_ACTION_RADIUS ) {
		Debug( "Distance to the item is too large to wait for it\n" );
		return INVALID;
	}
	if( currWorldState.HasThreateningEnemyVar() ) {
		Debug( "The bot has a threatening enemy\n" );
		return INVALID;
	}

	return VALID;
}

PlannerNode *WaitForNavEntityAction::TryApply( const WorldState &worldState ) {
	if( worldState.NavTargetOriginVar().Ignore() ) {
		Debug( "Nav target is ignored in the given world state\n" );
		return nullptr;
	}

	if( worldState.DistanceToNavTarget() > GOAL_PICKUP_ACTION_RADIUS ) {
		Debug( "Distance to goal item nav target is too large to wait for an item in the given world state\n" );
		return nullptr;
	}

	if( worldState.HasJustPickedGoalItemVar().Ignore() ) {
		Debug( "Has bot picked a goal item is ignored in the given world state\n" );
		return nullptr;
	}
	if( worldState.HasJustPickedGoalItemVar() ) {
		Debug( "Bot has just picked a goal item in the given world state\n" );
		return nullptr;
	}

	if( worldState.GoalItemWaitTimeVar().Ignore() ) {
		Debug( "Goal item wait time is not specified in the given world state\n" );
		return nullptr;
	}
	if( worldState.GoalItemWaitTimeVar() == 0 ) {
		Debug( "Goal item wait time is zero in the given world state\n" );
		return nullptr;
	}

	const auto &itemNavEntity = Self()->GetSelectedNavEntity();
	PlannerNodePtr plannerNode = NewNodeForRecord( pool.New( Self(), itemNavEntity.GetNavEntity() ) );
	if( !plannerNode ) {
		return nullptr;
	}

	plannerNode.Cost() = worldState.GoalItemWaitTimeVar();

	plannerNode.WorldState() = worldState;
	plannerNode.WorldState().HasJustPickedGoalItemVar().SetValue( true ).SetIgnore( false );
	plannerNode.WorldState().BotOriginVar().SetValue( itemNavEntity.GetNavEntity()->Origin() );
	plannerNode.WorldState().BotOriginVar().SetSatisfyOp( OriginVar::SatisfyOp::EQ, 12.0f );
	plannerNode.WorldState().ResetTacticalSpots();

	return plannerNode.PrepareActionResult();
}