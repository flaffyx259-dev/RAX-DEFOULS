#include "includes.h"

Movement g_movement{ };;

float Movement::GetMaxSpeed() {
	if (!g_cl.m_weapon || !g_cl.m_weapon_info)
		return 250.f;

	float max_speed = g_cl.m_local->m_bIsScoped() ? g_cl.m_weapon_info->m_max_player_speed_alt : g_cl.m_weapon_info->m_max_player_speed;

	if (g_cl.m_cmd->m_buttons & IN_DUCK)
	{
		max_speed *= 0.34f;
	}

	if (g_cl.m_local->m_fFlags() & FL_ONGROUND)
		max_speed *= g_cl.m_local->m_flVelocityModifier();

	return std::min(max_speed, 250.f);
}

void Movement::JumpRelated() {
	if (g_cl.m_local->m_MoveType() == MOVETYPE_NOCLIP)
		return;

	const RestoreVars* vars = g_restore_data.GetVars(g_cl.m_cmd->m_command_number - 1);
	if (!vars)
		return;

	if ((g_cl.m_cmd->m_buttons & IN_JUMP) && !(vars->m_fFlags & FL_ONGROUND)) {
		// bhop.
		if (g_menu.main.movement.bhop.get())
			g_cl.m_cmd->m_buttons &= ~IN_JUMP;

		// duck jump ( crate jump ).
		if (g_menu.main.movement.airduck.get())
			g_cl.m_cmd->m_buttons |= IN_DUCK;
	}
}

void Rotate(float angle)
{
	const auto rot = math::deg_to_rad(g_cl.m_cmd->m_view_angles.y - angle);
	const auto forward = std::cos(rot) * g_cl.m_cmd->m_forward_move - std::sin(rot) * g_cl.m_cmd->m_side_move;
	g_cl.m_cmd->m_side_move = std::sin(rot) * g_cl.m_cmd->m_forward_move + std::cos(rot) * g_cl.m_cmd->m_side_move;
	g_cl.m_cmd->m_forward_move = forward;
}

void Movement::Strafe() {
	if (!g_menu.main.movement.autostrafe.get())
		return;

	// don't strafe while noclipping or on ladders..
	if (g_cl.m_local->m_MoveType() == MOVETYPE_NOCLIP || g_cl.m_local->m_MoveType() == MOVETYPE_LADDER)
		return;

	// disable strafing while pressing shift.
	// don't strafe if not holding primary jump key.
	if (g_cl.PredictLand(1) || (g_cl.m_local->m_fFlags() & FL_ONGROUND))
		return;

	const vec3_t& velocity = g_cl.m_local->m_vecVelocity();

	float yaw = std::remainderf(g_cl.m_view_angles.y, 360.f);

	float offset = 0.f;
	if (g_cl.m_cmd->m_buttons & IN_MOVELEFT)
		offset += 90.f;
	if (g_cl.m_cmd->m_buttons & IN_MOVERIGHT)
		offset -= 90.f;
	if (g_cl.m_cmd->m_buttons & IN_FORWARD)
		offset *= .5f;
	else if (g_cl.m_cmd->m_buttons & IN_BACK)
		offset = (-offset * .5f) + 180.f;

	yaw += offset;

	float velocity_angle = math::rad_to_deg(std::atan2f(velocity.y, velocity.x));
	if (velocity_angle < 0.f)
		velocity_angle += 360.f;

	velocity_angle -= roundf(velocity_angle / 360.f) * 360.f;

	const auto speed = velocity.length_2d();
	const auto ideal = speed > 0.f ? std::clamp(math::rad_to_deg(std::atan2f(15.f, speed)), 0.f, 45.f) : 0.f;

	const auto correct = (100.f) * .01f * (ideal * 2.0f);
	g_cl.m_cmd->m_forward_move = 0.f;
	const auto velocity_delta = math::NormalizedAngle(yaw - velocity_angle);

	if ((fabsf(velocity_delta) > 170.f || velocity_delta > correct) && speed > 80.f)
	{
		yaw = correct + velocity_angle;
		g_cl.m_cmd->m_side_move = -450.f;
		Rotate(math::NormalizedAngle(yaw));
		return;
	}

	m_invert = !m_invert;
	if (-correct <= velocity_delta || speed <= 80.f)
	{
		if (m_invert)
		{
			yaw -= ideal;
			g_cl.m_cmd->m_side_move = -450.f;
		}
		else
		{
			yaw += ideal;
			g_cl.m_cmd->m_side_move = 450.f;
		}
		Rotate(math::NormalizedAngle(yaw));
	}
	else
	{
		yaw = velocity_angle - correct;
		g_cl.m_cmd->m_side_move = 450.f;
		Rotate(math::NormalizedAngle(yaw));
	}
}

void Movement::FixMove(CUserCmd* cmd, const ang_t& wish_angles) {
	vec3_t  move, dir;
	float   delta, len;
	ang_t   move_angle;

	// roll nospread fix.
	if (!(g_cl.m_local->m_fFlags() & FL_ONGROUND) && cmd->m_view_angles.z != 0.f)
		cmd->m_side_move = 0.f;

	// convert movement to vector.
	move = { cmd->m_forward_move, cmd->m_side_move, 0.f };

	// get move length and ensure we're using a unit vector ( vector with length of 1 ).
	len = move.normalize();
	if (!len)
		return;

	// convert move to an angle.
	math::VectorAngles(move, move_angle);

	// calculate yaw delta.
	delta = (cmd->m_view_angles.y - wish_angles.y);

	// accumulate yaw delta.
	move_angle.y += delta;

	// calculate our new move direction.
	// dir = move_angle_forward * move_length
	math::AngleVectors(move_angle, &dir);

	// scale to og movement.
	dir *= len;

	// strip old flags.
	g_cl.m_cmd->m_buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);

	// fix ladder and noclip.
	if (g_cl.m_local->m_MoveType() == MOVETYPE_LADDER) {
		// invert directon for up and down.
		if (cmd->m_view_angles.x >= 45.f && wish_angles.x < 45.f && std::abs(delta) <= 65.f)
			dir.x = -dir.x;

		// write to movement.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if (cmd->m_forward_move > 200.f)
			cmd->m_buttons |= IN_FORWARD;

		else if (cmd->m_forward_move < -200.f)
			cmd->m_buttons |= IN_BACK;

		if (cmd->m_side_move > 200.f)
			cmd->m_buttons |= IN_MOVERIGHT;

		else if (cmd->m_side_move < -200.f)
			cmd->m_buttons |= IN_MOVELEFT;
	}

	// we are moving normally.
	else {
		// we must do this for pitch angles that are out of bounds.
		if (cmd->m_view_angles.x < -90.f || cmd->m_view_angles.x > 90.f)
			dir.x = -dir.x;

		// set move.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		if (g_menu.main.antiaim.invert_leg_movement.get()) {
			// set new button flags.
			if (cmd->m_forward_move > 0.f)
				cmd->m_buttons |= IN_BACK;

			else if (cmd->m_forward_move < 0.f)
				cmd->m_buttons |= IN_FORWARD;

			if (cmd->m_side_move > 0.f)
				cmd->m_buttons |= IN_MOVELEFT;

			else if (cmd->m_side_move < 0.f)
				cmd->m_buttons |= IN_MOVERIGHT;
		}
		else {
			if (cmd->m_forward_move > 0.f)
				cmd->m_buttons |= IN_FORWARD;

			else if (cmd->m_forward_move < 0.f)
				cmd->m_buttons |= IN_BACK;

			if (cmd->m_side_move > 0.f)
				cmd->m_buttons |= IN_MOVERIGHT;

			else if (cmd->m_side_move < 0.f)
				cmd->m_buttons |= IN_MOVELEFT;
		}
	}
}

void Movement::QuickStop() {
	g_cl.m_cmd->m_buttons &= ~IN_SPEED;

	float speed = g_cl.m_local->m_vecVelocity().length_2d();
	if (g_cl.m_local->m_fFlags() & FL_ONGROUND) {
		const float& max_speed = GetMaxSpeed();
		const float& friction = g_csgo.sv_friction->GetFloat() * g_cl.m_local->m_surfaceFriction();

		if (speed <= max_speed * friction * g_csgo.m_globals->m_interval) {
			g_cl.m_cmd->m_forward_move = 0.f;
			g_cl.m_cmd->m_side_move = 0.f;
			return;
		}
	}

	// convert velocity to angular momentum.
	ang_t angle;
	math::VectorAngles(g_cl.m_local->m_vecVelocity(), angle);

	// fix direction by factoring in where we are looking.
	angle.y = g_cl.m_view_angles.y - angle.y;

	// convert corrected angle back to a direction.
	vec3_t direction;
	math::AngleVectors(angle, &direction);

	direction *= -speed;

	g_cl.m_cmd->m_forward_move = direction.x;
	g_cl.m_cmd->m_side_move = direction.y;
}

void Movement::AutoStop() {
	// this will get called after prediction so we need to use unpredicted data.
	const RestoreVars* vars = g_restore_data.GetVars(g_cl.m_cmd->m_command_number - 1);

	g_cl.m_cmd->m_buttons &= ~IN_SPEED;

	float speed = vars->m_vecVelocity.length_2d();
	const float& max_speed = GetMaxSpeed();

	if (speed <= max_speed * 0.34f) {
		vec2_t cmd_speed = { g_cl.m_cmd->m_forward_move, g_cl.m_cmd->m_side_move };

		float len = cmd_speed.length();
		if (len > 0.0001f) {
			vec2_t clamped = cmd_speed.normalized() * std::min(max_speed * 0.34f, len);
			g_cl.m_cmd->m_forward_move = clamped.x;
			g_cl.m_cmd->m_side_move = clamped.y;

			g_input_pred.repredict();
		}
		return;
	}

	// convert velocity to angular momentum.
	ang_t angle;
	math::VectorAngles(vars->m_vecVelocity, angle);

	// fix direction by factoring in where we are looking.
	angle.y = g_cl.m_view_angles.y - angle.y;

	// convert corrected angle back to a direction.
	vec3_t direction;
	math::AngleVectors(angle, &direction);

	direction *= -speed;

	g_cl.m_cmd->m_forward_move = direction.x;
	g_cl.m_cmd->m_side_move = direction.y;

	g_input_pred.repredict();
}

void Movement::FakeWalk() {
	if (!m_fakewalk || !g_cl.m_state)
		return;

	if (!(g_cl.m_local->m_fFlags() & FL_ONGROUND)) {
		QuickStop();
		return;
	}

	g_cl.m_cmd->m_buttons &= ~IN_SPEED;

	if (g_hvh.m_desync) {
		float real_yaw = math::NormalizedAngle(g_cl.m_state->m_foot_yaw);
		float fake_yaw = math::NormalizedAngle(g_fake_state.m_foot_yaw);

		if (float foot_delta = math::NormalizedAngle(fake_yaw - real_yaw); fabs(foot_delta) > 60.f)
			return;

		if (g_cl.m_state->m_vel_xy > 0.1f)
			QuickStop();

		return;
	}

	vec3_t velocity = math::Approach(g_cl.m_local->m_vecAbsVelocity(), g_cl.m_state->m_vel, ((g_cl.m_curtime + g_csgo.m_globals->m_interval) - g_cl.m_state->m_last_update) * 2000);

	if (g_csgo.m_cl->m_choked_commands <= 0 || g_hvh.m_updated || g_cl.m_state->m_vel.length_2d() > 0.1f) {
		QuickStop();
		return;
	}

	float max_speed = GetMaxSpeed();
	float surface_friction = g_csgo.sv_friction->GetFloat() * g_cl.m_local->m_surfaceFriction();
	float speed_drop_per_tick = max_speed * surface_friction * g_csgo.m_globals->m_interval;

	int    ticks_to_stop = static_cast<int>(roundf(velocity.length_2d() / speed_drop_per_tick));
	int    ticks_until_update = game::TIME_TO_TICKS(g_cl.m_body_update - (g_cl.m_curtime + g_csgo.m_globals->m_interval)) - 3;
	int    ticks_left = std::max(g_cl.m_max_lag - g_csgo.m_cl->m_choked_commands, 0);

	g_cl.m_charged_ticks = std::min(ticks_left, ticks_until_update);

	if ((g_cl.m_charged_ticks - ticks_to_stop) <= ticks_to_stop) {
		QuickStop();
		return;
	}
}

void Movement::AutoPeek() {
	// ts is off.
	if (!m_auto_peek) {
		m_retreat_origin = g_cl.m_local->m_vecOrigin();
		return;
	}

	if (!(g_cl.m_local->m_fFlags() & FL_ONGROUND))
		return;

	if (m_done_retreating)
		return;

	if ((m_retreat_origin - g_cl.m_local->m_vecOrigin()).length_2d() <= 10.f) {
		QuickStop();
		m_done_retreating = true;
		return;
	}

	ang_t angle;
	math::VectorAngles(m_retreat_origin - g_cl.m_local->m_vecOrigin(), angle);

	angle.y = g_cl.m_view_angles.y - angle.y;

	vec3_t direction;
	math::AngleVectors(angle, &direction);

	direction *= 450.f;

	g_cl.m_cmd->m_forward_move = direction.x;
	g_cl.m_cmd->m_side_move = direction.y;

}

void Movement::HandleWalk() {
	m_stopping = !(g_cl.m_buttons & IN_JUMP) && (g_cl.m_local->m_fFlags() & FL_ONGROUND) && g_cl.m_cmd->m_side_move == 0.f && g_cl.m_cmd->m_forward_move == 0.f;

	m_auto_peek = g_input.GetKeyState(g_menu.main.movement.autopeek.get());
	m_fakewalk = g_input.GetKeyState(g_menu.main.movement.fakewalk.get());

	AutoPeek();
	if (m_stopping) {
		QuickStop();
		g_hvh.MicroMovement();
		return;
	}

	FakeWalk();
}