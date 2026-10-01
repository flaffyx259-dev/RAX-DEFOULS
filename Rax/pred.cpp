#include "includes.h"
#include "pred.h"

InputPrediction g_input_pred{ };;
RestoreData g_restore_data{ };

void RestoreData::Reset() {
	RestoreVars* vars;
	for (int i = 0; i < 150; i++) {
		vars = GetVars(i);
		if (!vars)
			continue;

		vars->is_filled = false;
	}
}

void RestoreData::Setup(Player* player, int command)
{
	RestoreVars* vars = GetVars(command);
	if (!vars)
		return;

	vars->m_aimPunchAngle = player->m_aimPunchAngle();
	vars->m_aimPunchAngleVel = player->m_aimPunchAngleVel();
	vars->m_viewPunchAngle = player->m_viewPunchAngle();

	vars->m_vecViewOffset = player->m_vecViewOffset();
	vars->m_vecBaseVelocity = player->m_vecBaseVelocity();
	vars->m_vecVelocity = player->m_vecVelocity();
	vars->m_vecAbsVelocity = player->m_vecAbsVelocity();
	vars->m_vecOrigin = player->m_vecOrigin();
	vars->m_vecAbsOrigin = player->m_vecAbsOrigin();

	vars->m_flFallVelocity = player->m_flFallVelocity();
	vars->m_flVelocityModifier = player->m_flVelocityModifier();
	vars->m_flDuckAmount = player->m_flDuckAmount();
	vars->m_flDuckSpeed = player->m_flDuckSpeed();
	vars->m_fFlags = player->m_fFlags();

	vars->m_surfaceFriction = player->m_surfaceFriction();
	vars->m_GroundEntity = player->m_hGroundEntity();
	vars->m_MoveType = player->m_MoveType();
	vars->m_nTickBase = player->m_nTickBase();

	Weapon* weapon = (Weapon*)player->GetActiveWeapon();
	if (weapon)
	{
		vars->m_fAccuracyPenalty = weapon->m_fAccuracyPenalty();
		vars->m_flRecoilIndex = weapon->m_flRecoilIndex();
	}

	vars->is_filled = true;
}

void RestoreData::Apply(Player* player, int command)
{
	RestoreVars* vars = GetVars(command);
	if (!vars)
		return;

	if (!vars->is_filled)
		return;

	player->m_aimPunchAngle() = vars->m_aimPunchAngle;
	player->m_aimPunchAngleVel() = vars->m_aimPunchAngleVel;
	player->m_viewPunchAngle() = vars->m_viewPunchAngle;

	player->m_vecViewOffset() = vars->m_vecViewOffset;
	player->m_vecBaseVelocity() = vars->m_vecBaseVelocity;
	player->m_vecVelocity() = vars->m_vecVelocity;
	player->m_vecAbsVelocity() = vars->m_vecAbsVelocity;
	player->m_vecOrigin() = vars->m_vecOrigin;
	player->m_vecAbsOrigin() = vars->m_vecAbsOrigin;

	player->m_flFallVelocity() = vars->m_flFallVelocity;
	player->m_flVelocityModifier() = vars->m_flVelocityModifier;
	player->m_flDuckAmount() = vars->m_flDuckAmount;
	player->m_flDuckSpeed() = vars->m_flDuckSpeed;
	player->m_fFlags() = vars->m_fFlags;

	player->m_surfaceFriction() = vars->m_surfaceFriction;
	player->m_hGroundEntity() = vars->m_GroundEntity;
	player->m_MoveType() = vars->m_MoveType;
	player->m_nTickBase() = vars->m_nTickBase;

	Weapon* weapon = (Weapon*)player->GetActiveWeapon();
	if (weapon)
	{
		weapon->m_fAccuracyPenalty() = vars->m_fAccuracyPenalty;
		weapon->m_flRecoilIndex() = vars->m_flRecoilIndex;
	}
}

void InputPrediction::start_command(Player* player, CUserCmd* pCmd)
{
	player->m_pCurrentCommand() = pCmd;
	//m_LastCmd shouldnt be needed if we run engine prediction
	*g_csgo.m_nPredictionRandomSeed = pCmd ? g_cl.m_cmd->m_random_seed : -1;
	g_csgo.m_pPredictionPlayer = g_cl.m_local;
}

void InputPrediction::finish_command(Player* player)
{
	player->m_pCurrentCommand() = NULL;
	*g_csgo.m_nPredictionRandomSeed = NULL;
	g_csgo.m_pPredictionPlayer = NULL;
}

void InputPrediction::run_prethink(Player* player)
{
	if (!player->PhysicsRunThink(0))
		return;

	player->PreThink();
}

void InputPrediction::run_think(Player* player)
{
	int thinktick = player->nextThinkTick();
	if (thinktick <= 0 || thinktick > player->m_nTickBase())
		return;

	player->nextThinkTick() = -1;
	//sub_10179C60(0); // ????
	player->Think();
}

void InputPrediction::update()
{
	static auto cl_move_clamp = pattern::find(g_csgo.m_engine_dll, XOR("B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC")) + 1;
	unsigned long protect = 0;

	VirtualProtect((void*)cl_move_clamp, 4, PAGE_EXECUTE_READWRITE, &protect);
	*(std::uint32_t*)cl_move_clamp = 62;
	VirtualProtect((void*)cl_move_clamp, 4, protect, &protect);

	m_curtime = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;

	m_first_time_predicted = g_csgo.m_prediction->m_first_time_predicted;
	m_in_prediction = g_csgo.m_prediction->m_in_prediction;

	if (g_csgo.m_cl->m_delta_tick > 0)
		g_csgo.m_prediction->Update(g_csgo.m_cl->m_delta_tick, true, g_csgo.m_cl->m_last_command_ack, g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands);

	g_restore_data.Apply(g_cl.m_local, g_cl.m_cmd->m_command_number - 1);
}

void InputPrediction::post_think(Player* player)
{
	static auto PostThinkVPhysics = pattern::find(g_csgo.m_client_dll, "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB ? ? ? ? ? 75 50 8B 0D").as<bool(__thiscall*)(Entity*)>();
	static auto SimulatePlayerSimulatedEntities = pattern::find(g_csgo.m_client_dll, "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86").as<void(__thiscall*)(Entity*)>();

	if (player && player->alive())
	{
		using UpdateCollisionBoundsFn = void(__thiscall*)(void*);
		util::get_method<UpdateCollisionBoundsFn>(player, 329)(player);

		if (player->m_fFlags() & FL_ONGROUND)
			*reinterpret_cast<uintptr_t*>(uintptr_t(player) + 0x3004) = 0;

		if (*reinterpret_cast<int*>(uintptr_t(player) + 0x28AC) == -1)
		{
			using SetSequenceFn = void(__thiscall*)(void*, int);
			util::get_method<SetSequenceFn>(player, 213)(player, 0);
		}

		using StudioFrameAdvanceFn = void(__thiscall*)(void*);
		util::get_method<StudioFrameAdvanceFn>(player, 214)(player);

		PostThinkVPhysics(player);
	}
	SimulatePlayerSimulatedEntities(player);

	player->m_flThirdpersonRecoil() = player->m_aimPunchAngle().x;
}

void InputPrediction::run()
{
	// store before we run initial prediction in case we need to restore.
	m_forward_move = g_cl.m_cmd->m_forward_move;
	m_side_move = g_cl.m_cmd->m_side_move;

	start_command(g_cl.m_local, g_cl.m_cmd);

	g_csgo.m_prediction->m_first_time_predicted = false;
	g_csgo.m_prediction->m_in_prediction = true;

	g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase());
	g_csgo.m_globals->m_frametime = g_csgo.m_prediction->m_engine_paused ? 0.f : g_csgo.m_globals->m_interval;

	g_cl.m_cmd->m_buttons |= g_cl.m_local->m_afButtonForced();

	g_csgo.m_move_helper->SetHost(g_cl.m_local);
	g_csgo.m_game_movement->StartTrackPredictionErrors(g_cl.m_local);

	// Latch in impulse.
	if (g_cl.m_cmd->m_impulse)
	{
		// Discard impulse commands unless the vehicle allows them.
		// FIXME: UsingStandardWeapons seems like a bad filter for this. 
		// The flashlight is an impulse command, for example.
		//if ( !pVehicle || player->UsingStandardWeaponsInVehicle( ) )
		//{
		g_cl.m_local->m_nImpulse() = g_cl.m_cmd->m_impulse;
		//}
	}

	g_cl.m_local->UpdateButtonState(g_cl.m_cmd->m_buttons);

	g_csgo.m_prediction->CheckMovingGround(g_cl.m_local, g_csgo.m_globals->m_frametime);

	run_prethink(g_cl.m_local);
	run_think(g_cl.m_local);

	std::memset(&m_move_data, 0, sizeof(CMoveData));

	g_csgo.m_prediction->SetupMove(g_cl.m_local, g_cl.m_cmd, g_csgo.m_move_helper, &m_move_data);

	g_csgo.m_game_movement->ProcessMovement(g_cl.m_local, &m_move_data);

	g_csgo.m_prediction->FinishMove(g_cl.m_local, g_cl.m_cmd, &m_move_data);
	g_csgo.m_game_movement->Reset();

	post_think(g_cl.m_local);

	finish_command(g_cl.m_local);

	g_cl.m_local->m_vphysicsCollisionState() = 0;

	Weapon* weapon = g_cl.m_local->GetActiveWeapon();
	if (weapon) {
		weapon->UpdateAccuracyPenalty();

		m_accuracy_penalty = weapon->m_fAccuracyPenalty();
		m_inaccuracy = weapon->GetInaccuracy();
		m_spread = weapon->GetSpread();
	}
}

void InputPrediction::repredict()
{
	g_restore_data.Apply(g_cl.m_local, g_cl.m_cmd->m_command_number - 1);

	start_command(g_cl.m_local, g_cl.m_cmd);

	g_csgo.m_prediction->m_first_time_predicted = false;
	g_csgo.m_prediction->m_in_prediction = true;

	g_csgo.m_globals->m_curtime = game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase());
	g_csgo.m_globals->m_frametime = g_csgo.m_prediction->m_engine_paused ? 0.f : g_csgo.m_globals->m_interval;

	g_cl.m_cmd->m_buttons |= g_cl.m_local->m_afButtonForced();

	g_csgo.m_move_helper->SetHost(g_cl.m_local);
	g_csgo.m_game_movement->StartTrackPredictionErrors(g_cl.m_local);

	// Latch in impulse.
	if (g_cl.m_cmd->m_impulse)
	{
		// Discard impulse commands unless the vehicle allows them.
		// FIXME: UsingStandardWeapons seems like a bad filter for this. 
		// The flashlight is an impulse command, for example.
		//if ( !pVehicle || player->UsingStandardWeaponsInVehicle( ) )
		//{
		g_cl.m_local->m_nImpulse() = g_cl.m_cmd->m_impulse;
		//}
	}

	g_cl.m_local->UpdateButtonState(g_cl.m_cmd->m_buttons);

	g_csgo.m_prediction->CheckMovingGround(g_cl.m_local, g_csgo.m_globals->m_frametime);

	run_prethink(g_cl.m_local);
	run_think(g_cl.m_local);

	std::memset(&m_move_data, 0, sizeof(CMoveData));

	g_csgo.m_prediction->SetupMove(g_cl.m_local, g_cl.m_cmd, g_csgo.m_move_helper, &m_move_data);

	g_csgo.m_game_movement->ProcessMovement(g_cl.m_local, &m_move_data);

	g_csgo.m_prediction->FinishMove(g_cl.m_local, g_cl.m_cmd, &m_move_data);
	g_csgo.m_game_movement->Reset();

	post_think(g_cl.m_local);

	finish_command(g_cl.m_local);

	g_cl.m_local->m_vphysicsCollisionState() = 0;

	Weapon* weapon = g_cl.m_local->GetActiveWeapon();
	if (weapon) {
		weapon->UpdateAccuracyPenalty();

		m_accuracy_penalty = weapon->m_fAccuracyPenalty();
		m_inaccuracy = weapon->GetInaccuracy();
		m_spread = weapon->GetSpread();
	}
}

void InputPrediction::restore() const
{
	// restore globals.
	g_csgo.m_globals->m_curtime = m_curtime;
	g_csgo.m_globals->m_frametime = m_frametime;

	g_csgo.m_prediction->m_first_time_predicted = m_first_time_predicted;
	g_csgo.m_prediction->m_in_prediction = m_in_prediction;

	g_csgo.m_game_movement->FinishTrackPredictionErrors(g_cl.m_local);
	g_csgo.m_move_helper->SetHost(nullptr);

	g_restore_data.Apply(g_cl.m_local, g_cl.m_cmd->m_command_number - 1);
}

float InputPrediction::GetRecoveryTime() {
	if (!g_cl.m_weapon || !g_cl.m_weapon_info)
		return 0.f;

	if (g_cl.m_local->m_MoveType() == MOVETYPE_LADDER)
	{
		return g_cl.m_weapon_info->m_recovery_time_stand;
	}

	if (!(g_cl.m_local->m_fFlags() & FL_ONGROUND))	// in air
	{
		return g_cl.m_weapon_info->m_recovery_time_crouch * 4.0f;
	}

	if (g_cl.m_local->m_fFlags() & FL_DUCKING)
	{
		float flRecoveryTime = g_cl.m_weapon_info->m_recovery_time_crouch;
		float flRecoveryTimeFinal = g_cl.m_weapon_info->m_recovery_time_crouch_final;

		if (flRecoveryTimeFinal != -1.0f)	// uninitialized final recovery values are set to -1.0 from the weapon_base prefab in schema
		{
			int nRecoilIndex = g_cl.m_weapon->m_flRecoilIndex();
			flRecoveryTime = math::RemapValClamped(nRecoilIndex, g_cl.m_weapon_info->m_recovery_transition_start_bullet, g_cl.m_weapon_info->m_recovery_transition_end_bullet, flRecoveryTime, flRecoveryTimeFinal);
		}

		return flRecoveryTime;
	}

	float flRecoveryTime = g_cl.m_weapon_info->m_recovery_time_stand;
	float flRecoveryTimeFinal = g_cl.m_weapon_info->m_recovery_time_stand_final;

	if (flRecoveryTimeFinal != -1.0f)	// uninitialized final recovery values are set to -1.0 from the weapon_base prefab in schema
	{
		int nRecoilIndex = g_cl.m_weapon->m_flRecoilIndex();

		flRecoveryTime = math::RemapValClamped(nRecoilIndex, g_cl.m_weapon_info->m_recovery_transition_start_bullet, g_cl.m_weapon_info->m_recovery_transition_end_bullet, flRecoveryTime, flRecoveryTimeFinal);
	}

	return flRecoveryTime;
}

float InputPrediction::CalculateWantedAccuracyPenalty(const bool& primary)
{
	if (!g_cl.m_weapon || !g_cl.m_weapon_info)
		return 0.f;

	float fNewPenalty = 0.f;

	if (g_cl.m_local->m_MoveType() == MOVETYPE_LADDER)
	{
		fNewPenalty += primary ? g_cl.m_weapon_info->m_inaccuracy_ladder : g_cl.m_weapon_info->m_inaccuracy_ladder_alt;
	}
	else if (g_cl.m_local->GetGroundEntity() == nullptr)
	{
		fNewPenalty += primary ? g_cl.m_weapon_info->m_inaccuracy_stand : g_cl.m_weapon_info->m_inaccuracy_stand_alt;
		fNewPenalty += (primary ? g_cl.m_weapon_info->m_inaccuracy_jump : g_cl.m_weapon_info->m_inaccuracy_jump_alt) * g_csgo.weapon_air_spread_scale->GetFloat();
	}
	else if (g_cl.m_local->m_fFlags() & FL_DUCKING)
	{
		fNewPenalty += primary ? g_cl.m_weapon_info->m_inaccuracy_crouch : g_cl.m_weapon_info->m_inaccuracy_crouch_alt;
	}
	else
	{
		fNewPenalty += primary ? g_cl.m_weapon_info->m_inaccuracy_stand : g_cl.m_weapon_info->m_inaccuracy_stand_alt;
	}

	if (g_cl.m_weapon->m_bInReload())
	{
		fNewPenalty += g_cl.m_weapon_info->m_inaccuracy_reload;
	}

	return fNewPenalty;
}

float InputPrediction::CalculateInaccuracy(const float& accuracy_penalty, const float& length2d, const float& z, const bool& primary) {
	if (!g_cl.m_weapon || !g_cl.m_weapon_info)
		return 0.f;

	const float max_speed = g_movement.GetMaxSpeed();

	float inaccuracy_scale = math::RemapValClamped(length2d,
		max_speed * 0.34f,
		max_speed * 0.95f,
		0.0f, 1.0f);

	float wanted_inaccuracy = accuracy_penalty;
	if (inaccuracy_scale > 0.0f) {
		if (g_cl.m_local->m_bIsWalking()) {
			//flMovementInaccuracyScale *= 1.0;	// reduce inaccuracy when walking or slower. This is commented out because at 1.0, it's a noop but preserved in case a different value is desired.
			//flMovementInaccuracyScale = powf( flMovementInaccuracyScale, float( MOVEMENT_WALK_CURVE01_EXPONENT ) );
		}
		else
		{
			inaccuracy_scale = powf(inaccuracy_scale, 0.25f);
		}

		wanted_inaccuracy += inaccuracy_scale * (primary ? g_cl.m_weapon_info->m_inaccuracy_move : g_cl.m_weapon_info->m_inaccuracy_move_alt);
	}

	if (g_cl.m_local->GetGroundEntity() == nullptr)
	{
		float flInaccuracyJumpInitial = g_cl.m_weapon_info->m_inaccuracy_jump_initial * g_csgo.weapon_air_spread_scale->GetFloat();
		static const float kMaxFallingPenalty = 2.0f;	// Accuracy is never worse than 2x starting penalty

		// Use sqrt here to make the curve more "sudden" around the accurate point at the apex of the jump
		float fSqrtMaxJumpSpeed = sqrtf(g_csgo.sv_jump_impulse->GetFloat());
		float fSqrtVerticalSpeed = sqrtf(z);

		float flAirSpeedInaccuracy = math::RemapVal(fSqrtVerticalSpeed,
			fSqrtMaxJumpSpeed * 0.25f,	// Anything less than 6.25% of maximum speed has no additional accuracy penalty for z-motion (6.25% = .25 * .25)
			fSqrtMaxJumpSpeed,			// Penalty at max jump speed
			0.0f,						// No movement-related penalty when close to stopped
			flInaccuracyJumpInitial);	// Movement-penalty at start of jump

		// Clamp to min/max values.  (Don't use RemapValClamped because it makes clamping to > kJumpMovePenalty hard)
		if (flAirSpeedInaccuracy < 0)
			flAirSpeedInaccuracy = 0;
		else if (flAirSpeedInaccuracy > (kMaxFallingPenalty * flInaccuracyJumpInitial))
			flAirSpeedInaccuracy = kMaxFallingPenalty * flInaccuracyJumpInitial;

		// Apply air velocity inaccuracy penalty
		// (There is an additional penalty for being in the air at all applied in UpdateAccuracyPenalty())
		wanted_inaccuracy += flAirSpeedInaccuracy;
	}

	return std::min(wanted_inaccuracy, 1.f);
}
