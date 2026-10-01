#include "includes.h"

RebuiltAnimState g_fake_state;
RebuiltAnimState g_rendered_state;
RebuiltAnimState g_states[64];

//const char* pose_name_list[] = {
//	"lean_yaw",
//	"speed",
//	"ladder_speed",
//	"ladder_yaw",
//	"move_yaw",
//	"body_yaw",
//	"body_pitch",
//	"death_yaw",
//	"stand",
//	"jump_fall",
//	"aim_blend_stand_idle",
//	"aim_blend_crouch_idle",
//	"strafe_yaw",
//	"aim_blend_stand_walk",
//	"aim_blend_stand_run",
//	"aim_blend_crouch_walk",
//	"move_blend_walk",
//	"move_blend_run",
//	"move_blend_crouch",
//};

void RebuiltAnimState::UpdateLayers(Player* player, const ang_t& angle, const float& curtime, bool apply_state, CUserCmd* cmd)
{
	if (!player) {
		Reset();
		return;
	}

	// setup layers that we want to use/fix
	C_AnimationLayer* ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB = &m_layers[AnimLayer::ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];
	C_AnimationLayer* ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL = &m_layers[AnimLayer::ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
	C_AnimationLayer* ANIMATION_LAYER_ADJUST = &m_layers[AnimLayer::ANIMATION_LAYER_ADJUST];
	C_AnimationLayer* ANIMATION_LAYER_MOVEMENT_MOVE = &m_layers[AnimLayer::ANIMATION_LAYER_MOVEMENT_MOVE];
	C_AnimationLayer* ANIMATION_LAYER_MOVEMENT_STRAFECHANGE = &m_layers[AnimLayer::ANIMATION_LAYER_MOVEMENT_STRAFECHANGE];
	C_AnimationLayer* ANIMATION_LAYER_LEAN = &m_layers[AnimLayer::ANIMATION_LAYER_LEAN];
	C_AnimationLayer* ANIMATION_LAYER_ALIVELOOP = &m_layers[AnimLayer::ANIMATION_LAYER_ALIVELOOP];
	C_AnimationLayer* ANIMATION_LAYER_AIMMATRIX = &m_layers[AnimLayer::ANIMATION_LAYER_AIMMATRIX];

	m_increment = std::max(0.f, curtime - m_last_update);

	if (m_increment == 0.0f)
		return;

	C_AnimationLayer* ANIMATION_LAYERS[] = {
		ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
		ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
		ANIMATION_LAYER_ADJUST,
		ANIMATION_LAYER_MOVEMENT_MOVE,
		ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
		ANIMATION_LAYER_LEAN,
		ANIMATION_LAYER_AIMMATRIX,
		ANIMATION_LAYER_ALIVELOOP };

	for (C_AnimationLayer* ANIMATION_LAYER : ANIMATION_LAYERS) {
		ANIMATION_LAYER->m_owner = player;
		ANIMATION_LAYER->m_studio_hdr = player->GetModelPtr();
	}

	// setup ground and flag stuff
	if (m_player != player ||m_spawn_time != player->m_flSpawnTime())
	{
		for (C_AnimationLayer* ANIMATION_LAYER : ANIMATION_LAYERS) {
			ANIMATION_LAYER->m_weight = 0.f;
			ANIMATION_LAYER->m_cycle = 0.f;
			ANIMATION_LAYER->m_playback_rate = 0.f;
		}

		Reset();

		m_spawn_time = player->m_flSpawnTime();

		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_LEAN_YAW].Init(player, "lean_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_SPEED].Init(player, "speed");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_LADDER_SPEED].Init(player, "ladder_speed");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_LADDER_YAW].Init(player, "ladder_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_YAW].Init(player, "move_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_BODY_YAW].Init(player, "body_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_BODY_PITCH].Init(player, "body_pitch");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_DEATH_YAW].Init(player, "death_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_STAND].Init(player, "stand");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_JUMP_FALL].Init(player, "jump_fall");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_IDLE].Init(player, "aim_blend_stand_idle");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_IDLE].Init(player, "aim_blend_crouch_idle");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_STRAFE_DIR].Init(player, "strafe_yaw");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_WALK].Init(player, "aim_blend_stand_walk");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_RUN].Init(player, "aim_blend_stand_run");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_WALK].Init(player, "aim_blend_crouch_walk");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_WALK].Init(player, "move_blend_walk");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_RUN].Init(player, "move_blend_run");
		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK].Init(player, "move_blend_crouch");

		m_player = player;
	}

	SetAnimation(player);

	// copy the angle.
	m_angle = angle;
	m_angle.x = math::NormalizedAngle(m_angle.x + player->m_flThirdpersonRecoil());

	m_duck_amount = std::clamp(math::Approach(std::clamp(player->m_flDuckAmount() + m_duck_additive, 0.f, 1.f), m_duck_amount, m_increment * 6.0f), 0.f, 1.f);

	m_old_weapon = m_weapon;
	m_weapon = player->GetActiveWeapon();

	m_old_pos = m_pos;
	m_pos = player->m_vecAbsOrigin();

	m_abs_vel = player->m_vecAbsVelocity();

	if (m_abs_vel.length_sqr() > 97344.0F)
		m_abs_vel = m_abs_vel.normalized() * 312.0f;

	m_vel_z = m_abs_vel.z;
	m_abs_vel.z = 0.f;

	m_vel = math::Approach(m_abs_vel, m_vel, m_increment * 2000.f);

	m_vel_xy = std::min(m_vel.length_2d(), 260.f);
	m_vel_normalized = m_vel.normalized();

	if (m_vel_xy > 0)
		m_vel_normalized_non_zero = m_vel_normalized;

	m_max_speed = 260.f;
	if (m_weapon) {
		WeaponInfo* weapon_info = m_weapon->GetWpnData();
		if (weapon_info) {
			m_max_speed = player->m_bIsScoped() ? weapon_info->m_max_player_speed_alt : weapon_info->m_max_player_speed;
		}
	}

	m_max_speed = std::max(m_max_speed, 0.001f);

	m_speed_portion_run = std::clamp(m_vel_xy / m_max_speed, 0.f, 1.f);
	m_speed_portion_walk = m_vel_xy / (m_max_speed * 0.52f);
	m_speed_portion_crouch = m_vel_xy / (m_max_speed * 0.34f);

	bool started_moving = false;
	bool stopped_moving = false;

	if (m_vel_xy > 0)
	{
		started_moving = m_duration_moving <= 0;
		m_duration_still = 0;
		m_duration_moving += m_increment;
	}
	else
	{
		stopped_moving = m_duration_still <= 0;
		m_duration_moving = 0;
		m_duration_still += m_increment;
	}

	/* setup velocity */
	if (!m_adjust && stopped_moving && m_ground && !m_ladder && !m_landing && m_stutter_step < 50)
	{
		player->SetLayerSequence(ANIMATION_LAYER_ADJUST, SelectSequenceFromActivity(ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING));
		m_adjust = true;
	}


	if (player->GetLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
		player->GetLayerActivity(ANIMATION_LAYER_ADJUST) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
	{
		if (m_adjust && m_speed_portion_crouch <= 0.25f)
		{
			player->IncrementLayerCycleWeightRateGeneric(m_increment, ANIMATION_LAYER_ADJUST);
			m_adjust = !(player->IsLayerSequenceCompleted(m_increment, ANIMATION_LAYER_ADJUST));
		}
		else
		{
			m_adjust = false;
			float flWeight = ANIMATION_LAYER_ADJUST->m_weight;
			ANIMATION_LAYER_ADJUST->m_weight = math::Approach(0, flWeight, m_increment * 5);

			float flNewRate = (ANIMATION_LAYER_ADJUST->m_weight - flWeight) / m_increment;
			ANIMATION_LAYER_ADJUST->m_weight_delta_rate = flNewRate;
		}
	}

	m_foot_yaw_last = m_foot_yaw;
	m_foot_yaw = std::clamp(m_foot_yaw, -360.f, 360.f);
	float flEyeFootDelta = math::AngleDiff(m_angle.y, m_foot_yaw);

	// narrow the available aim matrix width as speed increases
	m_max_yaw_multiplier = math::Lerp(std::clamp(m_speed_portion_walk, 0.f, 1.f), 1.0f, math::Lerp(m_walk_to_run, 0.8f, 0.5f));

	if (m_duck_amount > 0)
	{
		m_max_yaw_multiplier = math::Lerp(m_duck_amount * std::clamp(m_speed_portion_crouch, 0.f, 1.f), m_max_yaw_multiplier, 0.5f);
	}

	float flTempYawMax = 58.f * m_max_yaw_multiplier;
	float flTempYawMin = -58.f * m_max_yaw_multiplier;

	if (flEyeFootDelta > flTempYawMax)
	{
		m_foot_yaw = m_angle.y - abs(flTempYawMax);
	}
	else if (flEyeFootDelta < flTempYawMin)
	{
		m_foot_yaw = m_angle.y + abs(flTempYawMin);
	}

	m_foot_yaw = math::NormalizedAngle(m_foot_yaw);

	if (m_ground)
	{
		if (m_vel_xy > 0.1f)
		{
			m_foot_yaw = math::ApproachAngle(m_angle.y, m_foot_yaw, m_increment * (30.0f + 20.0f * m_walk_to_run));

			m_body_update = curtime + (1.1f * 0.2f);
			m_body_yaw = m_angle.y;
		}
		else
		{
			m_foot_yaw = math::ApproachAngle(m_body_yaw, m_foot_yaw, m_increment * 100.f);

			if (curtime > m_body_update && std::abs(math::AngleDiff(m_foot_yaw, m_angle.y)) > 35.0f)
			{
				m_body_update = curtime + 1.1f;
				m_body_yaw = m_angle.y;
			}
		}
	}

	if (m_vel_xy <= 1.0f && m_ground && !m_ladder && !m_landing && m_increment > 0 && math::AngleDiff(m_foot_yaw_last, m_foot_yaw) / m_increment > 120.f)
	{
		player->SetLayerSequence(ANIMATION_LAYER_ADJUST, SelectSequenceFromActivity(ACT_CSGO_IDLE_TURN_BALANCEADJUST));
		m_adjust = true;
	}

	if (m_vel_xy > 0 && m_ground)
	{
		// convert horizontal velocity vec to angular yaw
		float flRawYawIdeal = (atan2(-m_vel.y, -m_vel.x) * 180 / math::pi);
		if (flRawYawIdeal < 0)
			flRawYawIdeal += 360;

		m_move_yaw_ideal = math::NormalizedAngle(math::AngleDiff(flRawYawIdeal, m_foot_yaw));
	}

	// delta between current yaw and ideal velocity derived target (possibly negative!)
	m_move_yaw_to_ideal = math::NormalizedAngle(math::AngleDiff(m_move_yaw_ideal, m_move_yaw));

	if (started_moving && m_move_weight <= 0)
	{
		m_move_yaw = m_move_yaw_ideal;

		// select a special starting cycle that's set by the animator in content
		int nMoveSeq = ANIMATION_LAYER_MOVEMENT_MOVE->m_sequence;
		if (nMoveSeq != -1)
		{
			mstudioseqdesc_t* seqdesc = player->GetModelPtr()->pSeqDesc(nMoveSeq);
			if (seqdesc && seqdesc->numanimtags > 0)
			{
				if (std::abs(math::AngleDiff(m_move_yaw, 180)) <= 22.5f) //N
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_N, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, 135)) <= 22.5f) //NE
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NE, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, 90)) <= 22.5f) //E
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_E, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, 45)) <= 22.5f) //SE
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SE, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, 0)) <= 22.5f) //S
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_S, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, -45)) <= 22.5f) //SW
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_SW, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, -90)) <= 22.5f) //W
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_W, 0, 1);
				}
				else if (std::abs(math::AngleDiff(m_move_yaw, -135)) <= 22.5f) //NW
				{
					m_primary_cycle = player->GetFirstSequenceAnimTag(nMoveSeq, ANIMTAG_STARTCYCLE_NW, 0, 1);
				}
			}
		}
	}
	else
	{
		if (ANIMATION_LAYER_MOVEMENT_STRAFECHANGE->m_weight >= 1)
		{
			m_move_yaw = m_move_yaw_ideal;
		}
		else
		{

			float flMoveWeight = math::Lerp(m_duck_amount, std::clamp(m_speed_portion_walk, 0.f, 1.f), std::clamp(m_speed_portion_crouch, 0.f, 1.f));
			float flRatio = math::Bias(flMoveWeight, 0.18f) + 0.1f;

			m_move_yaw = math::NormalizedAngle(m_move_yaw + (m_move_yaw_to_ideal * flRatio));
		}
	}

	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_YAW].SetValue(player, m_move_yaw);

	float flAimYaw = math::AngleDiff(m_angle.y, m_foot_yaw);
	if (flAimYaw >= 0)
	{
		flAimYaw = (flAimYaw / 58.f) * 60.0f;
	}
	else
	{
		flAimYaw = (flAimYaw / -58.f) * -60.0f;
	}

	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_BODY_YAW].SetValue(player, flAimYaw);

	// we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_BODY_PITCH].SetValue(player, math::AngleDiff(m_angle.x, 0));
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_SPEED].SetValue(player, m_speed_portion_walk);
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_STAND].SetValue(player, 1.0f - (m_duck_amount * m_air_smooth));

	/* aim matrix */
	if (m_duck_amount <= 0 || m_duck_amount >= 1) // only transition aim pose when fully ducked or fully standing
	{
		bool is_walking = player->m_bIsWalking();
		bool is_scoped = player->m_bIsScoped();

		float transition_speed = m_increment * (is_scoped ? 4.2f : 0.8f);

		if (is_scoped) // hacky: just tell all the transitions they've been invalid too long so all transitions clear as soon as the player starts scoping
		{
			m_stand_walk_aim.m_duration_state_has_been_invalid = m_stand_walk_aim.m_how_long_to_wait_until_transition_can_blend_out;
			m_stand_run_aim.m_duration_state_has_been_invalid = m_stand_run_aim.m_how_long_to_wait_until_transition_can_blend_out;
			m_crouch_run_aim.m_duration_state_has_been_invalid = m_crouch_run_aim.m_how_long_to_wait_until_transition_can_blend_out;
		}

		m_stand_walk_aim.UpdateTransitionState(is_walking && !is_scoped && m_speed_portion_walk > 0.7f && m_speed_portion_run < 0.7,
			m_increment, transition_speed);

		m_stand_run_aim.UpdateTransitionState(!is_scoped && m_speed_portion_run >= 0.7,
			m_increment, transition_speed);

		m_crouch_run_aim.UpdateTransitionState(!is_scoped && m_speed_portion_crouch >= 0.5,
			m_increment, transition_speed);
	}

	// Set aims to zero weight if they're underneath aims with 100% weight, for animation perf optimization.
	// Also set aims to full weight if their overlapping aims aren't enough to cover them, because cross-fades don't sum to 100% weight.

	float stand_idle_weight = 1;
	float stand_walk_weight = m_stand_walk_aim.m_blend_value;
	float stand_run_weight = m_stand_run_aim.m_blend_value;
	float crouch_idle_weight = 1;
	float crouch_walk_weight = m_crouch_run_aim.m_blend_value;

	if (stand_walk_weight >= 1)
		stand_idle_weight = 0;

	if (stand_run_weight >= 1)
	{
		stand_idle_weight = 0;
		stand_walk_weight = 0;
	}

	if (crouch_walk_weight >= 1)
		crouch_idle_weight = 0;

	if (m_duck_amount >= 1)
	{
		stand_idle_weight = 0;
		stand_walk_weight = 0;
		stand_run_weight = 0;
	}
	else if (m_duck_amount <= 0)
	{
		crouch_idle_weight = 0;
		crouch_walk_weight = 0;
	}

	float flOneMinusDuckAmount = 1.0f - m_duck_amount;

	crouch_idle_weight *= m_duck_amount;
	crouch_walk_weight *= m_duck_amount;
	stand_walk_weight *= flOneMinusDuckAmount;
	stand_run_weight *= flOneMinusDuckAmount;

	// make sure idle is present underneath cross-fades
	if (crouch_idle_weight < 1 && crouch_walk_weight < 1 && stand_walk_weight < 1 && stand_run_weight < 1)
		stand_idle_weight = 1;

	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_IDLE].SetValue(player, stand_idle_weight);
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_WALK].SetValue(player, stand_walk_weight);
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_STAND_RUN].SetValue(player, stand_run_weight);
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_IDLE].SetValue(player, crouch_idle_weight);
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_WALK].SetValue(player, crouch_walk_weight);

	char szTransitionStandAimMatrix[64];
	std::sprintf(szTransitionStandAimMatrix, "%s_aim", m_weapon->GetWeaponPrefix());
	int nSeqStand = player->LookupSequence(szTransitionStandAimMatrix);

	player->UpdateAnimLayer(ANIMATION_LAYER_AIMMATRIX, nSeqStand, 0, 1, 0);

	/* move */
	if (m_walk_to_run > 0 && m_walk_to_run < 1)
	{
		//currently transitioning between walk and run
		if (m_walk_to_run_state == 0)
		{
			m_walk_to_run += m_increment * 2.0f;
		}
		else // m_bWalkToRunTransitionState == 1
		{
			m_walk_to_run -= m_increment * 2.0f;
		}
		m_walk_to_run = std::clamp(m_walk_to_run, 0.f, 1.f);
	}

	if (m_vel_xy > 135.2f && m_walk_to_run_state == 1)
	{
		//crossed the walk to run threshold
		m_walk_to_run_state = 0;
		m_walk_to_run = std::max(0.01f, m_walk_to_run);
	}
	else if (m_vel_xy < 135.2f && m_walk_to_run_state == 0)
	{
		//crossed the run to walk threshold
		m_walk_to_run_state = 1;
		m_walk_to_run = std::min(0.99f, m_walk_to_run);
	}

	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_WALK].SetValue(player, (1.0f - m_walk_to_run) * (1.0f - m_duck_amount));
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_RUN].SetValue(player, (m_walk_to_run) * (1.0f - m_duck_amount));
	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK].SetValue(player, m_duck_amount);

	char szWeaponMoveSeq[64];
	std::sprintf(szWeaponMoveSeq, "move_%s", m_weapon->GetWeaponPrefix());

	int nWeaponMoveSeq = player->LookupSequence(szWeaponMoveSeq);
	if (nWeaponMoveSeq == -1)
	{
		nWeaponMoveSeq = player->LookupSequence("move");
	}

	if (player->m_iMoveState() != m_move_state)
	{
		m_stutter_step += 10;
	}

	m_move_state = player->m_iMoveState();
	m_stutter_step = std::clamp(math::Approach(0, m_stutter_step, m_increment * 40), 0.f, 100.f);

	// see: CSGOPlayerAnimState::SetUpMovement
	m_target_move_weight = math::Lerp(m_duck_amount, std::clamp(m_speed_portion_walk, 0.f, 1.f), std::clamp(m_speed_portion_crouch, 0.f, 1.f));

	if (m_move_weight <= m_target_move_weight)
	{
		m_move_weight = m_target_move_weight;
	}
	else
	{
		m_move_weight = math::Approach(m_target_move_weight, m_move_weight, m_increment * math::RemapValClamped(m_stutter_step, 0.0f, 100.0f, 2, 20));
	}

	vec3_t vecMoveYawDir;
	math::AngleVectors(ang_t(0, math::NormalizedAngle(m_foot_yaw + m_move_yaw + 180), 0), &vecMoveYawDir);
	float flYawDeltaAbsDot = abs(m_vel_normalized_non_zero.dot(vecMoveYawDir));
	m_move_weight *= math::Bias(flYawDeltaAbsDot, 0.2f);

	float flMoveWeightWithAirSmooth = m_move_weight * m_air_smooth;

	// dampen move weight for landings
	flMoveWeightWithAirSmooth *= std::max((1.0f - ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight), 0.55f);

	float flMoveCycleRate = 0;
	if (m_vel_xy > 0)
	{
		flMoveCycleRate = player->GetSequenceCycleRate(nWeaponMoveSeq);
		float flSequenceGroundSpeed = std::max(player->GetSequenceMoveDist(nWeaponMoveSeq, m_poses) / (1.0f / flMoveCycleRate), 0.001f);
		flMoveCycleRate *= m_vel_xy / flSequenceGroundSpeed;

		flMoveCycleRate *= math::Lerp(m_walk_to_run, 1.0f, 0.85f);
	}

	float flLocalCycleIncrement = (flMoveCycleRate * m_increment);
	m_primary_cycle = math::ClampCycle(m_primary_cycle + flLocalCycleIncrement);

	flMoveWeightWithAirSmooth = std::clamp(flMoveWeightWithAirSmooth, 0.f, 1.f);
	player->UpdateAnimLayer(ANIMATION_LAYER_MOVEMENT_MOVE, nWeaponMoveSeq, flLocalCycleIncrement, flMoveWeightWithAirSmooth, m_primary_cycle);

	/* strafe change */
	vec3_t vecForward;
	vec3_t vecRight;
	math::AngleVectors(ang_t(0, m_foot_yaw, 0), &vecForward, &vecRight);
	vecRight = vecRight.normalized();

	float flVelToRightDot = m_vel_normalized_non_zero.dot(vecRight);
	float flVelToForwardDot = m_vel_normalized_non_zero.dot(vecForward);

	// We're interested in if the player's desired direction (indicated by their held buttons) is opposite their current velocity.
	// This indicates a strafing direction change in progress.

	if (cmd)
	{
		bool moveRight = (cmd->m_buttons & (IN_MOVERIGHT)) != 0;
		bool moveLeft = (cmd->m_buttons & (IN_MOVELEFT)) != 0;
		bool moveForward = (cmd->m_buttons & (IN_FORWARD)) != 0;
		bool moveBackward = (cmd->m_buttons & (IN_BACK)) != 0;

		bool bStrafeRight = (m_speed_portion_walk >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f);
		bool bStrafeLeft = (m_speed_portion_walk >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f);
		bool bStrafeForward = (m_speed_portion_walk >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f);
		bool bStrafeBackward = (m_speed_portion_walk >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f);

		player->m_bStrafing() = (bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward);
	}

	if (player->m_bStrafing())
	{
		if (!m_strafe_changed)
		{
			m_strafe_duration = 0;
		}

		m_strafe_changed = true;

		m_strafe_weight = math::Approach(1, m_strafe_weight, m_increment * 20);
		m_strafe_cycle = math::Approach(0, m_strafe_cycle, m_increment * 10);

		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_STRAFE_DIR].SetValue(player, math::NormalizedAngle(m_move_yaw));
	}
	else if (m_strafe_weight > 0)
	{
		m_strafe_duration += m_increment;

		if (m_strafe_duration > 0.08f)
			m_strafe_weight = math::Approach(0, m_strafe_weight, m_increment * 5);

		m_strafe_sequence = player->LookupSequence("strafe");
		float flRate = player->GetSequenceCycleRate(m_strafe_sequence);
		m_strafe_cycle = std::clamp(m_strafe_cycle + m_increment * flRate, 0.f, 1.f);
	}

	if (m_strafe_weight <= 0)
	{
		m_strafe_changed = false;
	}

	bool previous_ground_state = m_ground;
	m_ground = player->m_fFlags() & FL_ONGROUND;

	bool landed = previous_ground_state != m_ground && m_ground;
	bool jumped = previous_ground_state != m_ground && !m_ground;

	float flDistanceFell = 0;
	if (jumped)
	{
		m_ground_height = m_pos.z;
	}

	if (landed)
	{
		flDistanceFell = abs(m_ground_height - m_pos.z);
		float flDistanceFallNormalizedBiasRange = math::Bias(math::RemapValClamped(flDistanceFell, 12.0f, 72.0f, 0.0f, 1.0f), 0.4f);

		//Msg( "Fell %f units, ratio is %f. ", flDistanceFell, flDistanceFallNormalizedBiasRange );
		//Msg( "Fell for %f secs, multiplier is %f\n", m_flDurationInAir, m_flLandAnimMultiplier );

		m_landing_multiplier = std::clamp(math::Bias(m_duration_in_air, 0.3f), 0.1f, 1.0f);
		m_duck_additive = std::max(m_landing_multiplier, flDistanceFallNormalizedBiasRange);

		//Msg( "m_flDuckAdditional is %f\n", m_flDuckAdditional );
	}
	else
	{
		m_duck_additive = math::Approach(0, m_duck_additive, m_increment * 2);
	}

	m_air_smooth = math::Approach(m_ground ? 1 : 0, m_air_smooth, math::Lerp(m_duck_amount, 8.f, 16.f) * m_increment);
	m_air_smooth = std::clamp(m_air_smooth, 0.f, 1.f);

	m_strafe_weight *= (1.0f - m_duck_amount);
	m_strafe_weight *= m_air_smooth;
	m_strafe_weight = std::clamp(m_strafe_weight, 0.f, 1.f);

	if (m_strafe_sequence != -1)
		player->UpdateAnimLayer(ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, m_strafe_sequence, 0, m_strafe_weight, m_strafe_cycle);

	//ladders
	bool previously_on_ladder = m_ladder;
	m_ladder = !m_ground && player->m_MoveType() == MOVETYPE_LADDER;
	bool started_laddering = (!previously_on_ladder && m_ladder);
	bool stopped_laddering = (previously_on_ladder && !m_ladder);

	if (m_ladder_weight > 0 || m_ladder)
	{
		if (started_laddering)
		{
			player->SetLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectSequenceFromActivity(ACT_CSGO_CLIMB_LADDER));
		}

		if (std::abs(m_vel_z) > 100)
		{
			m_ladder_speed = math::Approach(1, m_ladder_speed, m_increment * 10.0f);
		}
		else
		{
			m_ladder_speed = math::Approach(0, m_ladder_speed, m_increment * 10.0f);
		}
		m_ladder_speed = std::clamp(m_ladder_speed, 0.f, 1.f);

		if (m_ladder)
		{
			m_ladder_weight = math::Approach(1, m_ladder_weight, m_increment * 5.0f);
		}
		else
		{
			m_ladder_weight = math::Approach(0, m_ladder_weight, m_increment * 10.0f);
		}
		m_ladder_weight = std::clamp(m_ladder_weight, 0.f, 1.f);

		float flLadderClimbCycle = ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_cycle;
		flLadderClimbCycle += (m_pos.z - m_old_pos.z) * math::Lerp(m_ladder_speed, 0.010f, 0.004f);

		m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_LADDER_SPEED].SetValue(player, m_ladder_speed);

		if (player->GetLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) == ACT_CSGO_CLIMB_LADDER)
		{
			ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = m_ladder_weight;
		}

		ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_cycle = flLadderClimbCycle;

		// fade out jump if we're climbing
		if (m_ladder)
		{
			float flIdealJumpWeight = 1.0f - m_ladder_weight;
			if (ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight > flIdealJumpWeight)
			{
				ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight = flIdealJumpWeight;
			}
		}
	}
	else
	{
		m_ladder_speed = 0;
	}

	if (m_ground)
	{
		if (!m_landing && (landed || stopped_laddering))
		{
			player->SetLayerSequence(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectSequenceFromActivity(m_duration_in_air > 1 ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT));
			ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = 0.f;
			m_landing = true;
		}
		m_duration_in_air = 0;

		if (m_landing && player->GetLayerActivity(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) != ACT_CSGO_CLIMB_LADDER)
		{
			m_jumping = false;

			player->IncrementLayerCycle(m_increment, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false);
			player->IncrementLayerCycle(m_increment, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

			m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_JUMP_FALL].SetValue(player, 0.f);

			if (player->IsLayerSequenceCompleted(m_increment, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB))
			{
				m_landing = false;
				ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = 0.f;
				//SetLayerRate( ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 1.0f );
				ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight = 0.f;
				m_landing_multiplier = 1.0f;
			}
			else
			{
				float flLandWeight = player->GetLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB) * m_landing_multiplier;

				// if we hit the ground crouched, reduce the land animation as a function of crouch, since the land animations move the head up a bit ( and this is undesirable )
				flLandWeight *= std::clamp((1.0f - m_duck_amount), 0.2f, 1.0f);

				ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = flLandWeight;

				// fade out jump because land is taking over
				float flCurrentJumpFallWeight = ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight;
				if (flCurrentJumpFallWeight > 0)
				{
					flCurrentJumpFallWeight = math::Approach(0, flCurrentJumpFallWeight, m_increment * 10.0f);
					ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight = flCurrentJumpFallWeight;
				}
			}
		}

		if (!m_landing && !m_jumping && m_ladder_weight <= 0)
		{
			ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = 0.f;
		}
	}
	else if (!m_ladder)
	{
		m_landing = false;

		// we're in the air
		if (jumped || stopped_laddering)
		{
			// If entered the air by jumping, then we already set the jump activity.
			// But if we're in the air because we strolled off a ledge or the floor collapsed or something,
			// we need to set the fall activity here.
			if (!m_jumping)
			{
				player->SetLayerSequence(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, SelectSequenceFromActivity(ACT_CSGO_FALL));
			}
			m_duration_in_air = 0;
		}

		m_duration_in_air += m_increment;

		player->IncrementLayerCycle(m_increment, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false);

		// increase jump weight
		float flJumpWeight = ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight;
		float flNextJumpWeight = player->GetLayerIdealWeightFromSeqCycle(ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL);
		if (flNextJumpWeight > flJumpWeight)
		{
			ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL->m_weight = flNextJumpWeight;
		}

		// bash any lingering land weight to zero
		float flLingeringLandWeight = ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight;
		if (flLingeringLandWeight > 0)
		{
			flLingeringLandWeight *= math::smoothstep_bounds(0.2f, 0.0f, m_duration_in_air);
			ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB->m_weight = flLingeringLandWeight;
		}

		m_pose_cache[PLAYER_POSE_PARAM_JUMP_FALL].SetValue(player, std::clamp(math::smoothstep_bounds(0.72f, 1.52f, m_duration_in_air), 0.f, 1.f));
	}

	/* aliveloop */
	if (player->GetLayerActivity(ANIMATION_LAYER_ALIVELOOP) != ACT_CSGO_ALIVE_LOOP)
	{
		// first time init
		player->SetLayerSequence(ANIMATION_LAYER_ALIVELOOP, SelectSequenceFromActivity(ACT_CSGO_ALIVE_LOOP));
		ANIMATION_LAYER_ALIVELOOP->m_cycle = g_csgo.RandomFloat(0, 1);
		float flNewRate = player->GetSequenceCycleRate(ANIMATION_LAYER_ALIVELOOP->m_sequence);
		flNewRate *= g_csgo.RandomFloat(0.8f, 1.1f);
		ANIMATION_LAYER_ALIVELOOP->m_playback_rate = flNewRate;
	}
	else
	{
		if (m_weapon && m_weapon != m_old_weapon)
		{
			//re-roll act on weapon change
			float flRetainCycle = ANIMATION_LAYER_ALIVELOOP->m_cycle;
			player->SetLayerSequence(ANIMATION_LAYER_ALIVELOOP, SelectSequenceFromActivity(ACT_CSGO_ALIVE_LOOP));
			ANIMATION_LAYER_ALIVELOOP->m_cycle = flRetainCycle;
		}
		else if (player->IsLayerSequenceCompleted(m_increment, ANIMATION_LAYER_ALIVELOOP))
		{
			float flNewRate = player->GetSequenceCycleRate(ANIMATION_LAYER_ALIVELOOP->m_sequence);
			flNewRate *= g_csgo.RandomFloat(0.8f, 1.1f);
			ANIMATION_LAYER_ALIVELOOP->m_playback_rate = flNewRate;
		}
		else
		{
			float flWeightOutPoseBreaker = math::RemapValClamped(m_speed_portion_run, 0.55f, 0.9f, 1.0f, 0.0f);
			ANIMATION_LAYER_ALIVELOOP->m_weight = flWeightOutPoseBreaker;
		}
	}

	player->IncrementLayerCycle(m_increment, ANIMATION_LAYER_ALIVELOOP, true);

	/* setup lean */

	// lean the body into velocity derivative (acceleration) to simulate maintaining a center of gravity
	float flInterval = curtime - m_vel_test_time;
	if (flInterval > 0.025f)
	{
		flInterval = std::min(flInterval, 0.1f);
		m_vel_test_time = curtime;

		m_target_acceleration = (player->m_vecVelocity() - m_vel_last) / flInterval;
		m_target_acceleration.z = 0;

		m_vel_last = player->m_vecVelocity();
	}

	m_acceleration = math::Approach(m_target_acceleration, m_acceleration, m_increment * 800.0f);

	ang_t temp;
	math::VectorAngles(m_acceleration, temp);

	m_acceleration_weight = std::clamp((m_acceleration.length() / 260.f) * m_speed_portion_run, 0.f, 1.f);
	m_acceleration_weight *= (1.0f - m_ladder_weight);

	m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_LEAN_YAW].SetValue(player, math::NormalizedAngle(m_foot_yaw - temp.y));

	if (ANIMATION_LAYER_LEAN->m_sequence <= 0)
	{
		player->SetLayerSequence(ANIMATION_LAYER_LEAN, player->LookupSequence("lean"));
	}

	ANIMATION_LAYER_LEAN->m_weight = m_acceleration_weight;

	m_last_update = curtime;

	player->GetPoseParameters(m_poses);

	/* apply changes to player animstate aswell */
	if (apply_state)
		Correct(player->m_PlayerAnimState());
}

void RebuiltAnimState::SetAnimation(Player* player) const {
	if (m_layers[0].m_owner != player)
		return;

	if (player == g_cl.m_local)
		player->SetAnimLayers(g_cl.m_layers);

	for (int i = 0; i < 13; i++) {
		if (m_layers[i].m_owner != player)
			continue;

		player->m_AnimOverlay()[i] = m_layers[i];
	}

	player->SetPoseParameters(m_poses);
	player->SetAbsAngles({ 0, m_foot_yaw, 0 });
}

void RebuiltAnimState::DoAnimStateEvent(CUserCmd* cmd, int event) {
	UpdateActivityModifiers();

	switch (event) {
	case PLAYERANIMEVENT_THROW_GRENADE_UNDERHAND:
	case PLAYERANIMEVENT_FIRE_GUN_PRIMARY:
	case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT:
	case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_SPECIAL1:
	case PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT_SPECIAL1:
	case PLAYERANIMEVENT_FIRE_GUN_SECONDARY:
	case PLAYERANIMEVENT_FIRE_GUN_SECONDARY_SPECIAL1:
	case PLAYERANIMEVENT_GRENADE_PULL_PIN:
	case PLAYERANIMEVENT_SILENCER_ATTACH:
	case PLAYERANIMEVENT_SILENCER_DETACH:
	case PLAYERANIMEVENT_RELOAD:
	case PLAYERANIMEVENT_RELOAD_START:
	case PLAYERANIMEVENT_RELOAD_LOOP:
	case PLAYERANIMEVENT_RELOAD_END:
	case PLAYERANIMEVENT_CATCH_WEAPON:
	case PLAYERANIMEVENT_CLEAR_FIRING:
	case PLAYERANIMEVENT_DEPLOY:
		break;
	case PLAYERANIMEVENT_JUMP:
		m_jumping = true;
		g_cl.m_local->SetLayerSequence(&m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL], SelectSequenceFromActivity(ACT_CSGO_FALL));
		break;
	}
}

bool animstate_pose_param_cache_t::Init(Player* pPlayer, const char* szPoseParamName)
{
	if (!szPoseParamName)
		return false;

	m_name = szPoseParamName;
	m_index = pPlayer->LookupPoseParameter(pPlayer->GetModelPtr(), szPoseParamName);
	if (m_index != -1)
		m_init = true;

	return m_init;
}

void animstate_pose_param_cache_t::SetValue(Player* player, float flValue) {
	auto hdr = player->GetModelPtr();
	if (!hdr)
		return;

	if (!m_init)
		Init(player, m_name);

	if (m_index == -1)
		return;

	auto pose_param = pPoseParameter(hdr, m_index);
	if (!pose_param)
		return;

	if (!m_init)
		return;

	if (pose_param->m_loop)
	{
		float wrap = (pose_param->m_start + pose_param->m_end) / 2.0 + pose_param->m_loop / 2.0;
		float shift = pose_param->m_loop - wrap;

		flValue = flValue - pose_param->m_loop * floor((flValue + shift) / pose_param->m_loop);
	}

	player->m_flPoseParameter()[m_index] = (flValue - pose_param->m_start) / (pose_param->m_end - pose_param->m_start);
}