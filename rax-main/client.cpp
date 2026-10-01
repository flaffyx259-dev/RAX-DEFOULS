#include "includes.h"

Client g_cl{ };

// loader will set this fucker.
char username[33] = "\x90\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x90";

// init routine.
ulong_t __stdcall Client::init(void* arg) {
	// stop here if we failed to acquire all the data needed from csgo.
	if (!g_csgo.init())
		return 0;

	g_notify.add(tfm::format(XOR("Welcome %s\n"), g_csgo.m_users[0].m_name));
	return g_csgo.valid_hwid() == g_csgo.m_users[0].m_hwid;
}

void Client::DrawHUD() {
	if (!g_csgo.m_engine->IsInGame()) //if not in game dont run
		return;

	// get time.
	time_t t = std::time(nullptr);
	std::ostringstream time;
	time << std::put_time(std::localtime(&t), ("%H:%M:%S")); //format time

	// get round trip time in milliseconds.
	int ms = std::max(0, (int)std::round(m_latency[INetChannel::FLOW_OUTGOING] * 1000.f));

	// if agartha mode, cheat display name is AgarthaHack, else it is rax
#ifdef _DEV
	std::string name = g_menu.main.misc.agartha.get() ? XOR("AgarthaHack(Dev)") : XOR("rax(dev)");
#else
	std::string name = g_menu.main.misc.agartha.get() ? XOR("AgarthaHack") : XOR("rax");
#endif

	//if agartha mode, taps counter is set to Jews, else it is set to the actual headshot counter
	std::string taps = g_menu.main.misc.agartha.get() ? XOR("Taps: Jews") : tfm::format(XOR("taps: %i"), m_taps);
	// the final complete text string is formatted
	std::string text = tfm::format(XOR("%s | rtt: %ims | %s"), name, ms, taps);

	//RENDERING
	// set fontsize.
	render::FontSize_t size = render::hud.size(text);
	// background.
	render::rect_filled(m_width - size.m_width - 20, 10, size.m_width + 10, size.m_height + 2, { 240, 110, 140, 130 });
	// text.
	render::hud.string(m_width - 15, 10, { 240, 160, 180, 250 }, text, render::ALIGN_RIGHT);
}

void Client::KillFeed() {
	if (!g_menu.main.misc.killfeed.get())
		return;

	if (!g_csgo.m_engine->IsInGame())
		return;

	// get the addr of the killfeed.
	KillFeed_t* feed = (KillFeed_t*)g_csgo.m_hud->FindElement(HASH("SFHudDeathNoticeAndBotStatus"));
	if (!feed)
		return;

	int size = feed->notices.Count();
	if (!size)
		return;

	for (int i{ }; i < size; ++i) {
		NoticeText_t* notice = &feed->notices[i];

		// this is a local player kill, delay it.
		if (notice->fade == 1.5f)
			notice->fade = FLT_MAX;
	}
}

void Client::OnPaint() {
	// update screen size.
	g_csgo.m_engine->GetScreenSize(m_width, m_height);

	// render stuff.
	g_visuals.think();
	g_grenades.paint();
	g_notify.think();

	DrawHUD();
	KillFeed();

	// menu goes last.
	g_gui.think();
}

void Client::OnMapload() {
	// store class ids.
	g_netvars.SetupClassData();

	// createmove will not have been invoked yet.
	// but at this stage entites have been created.
	// so now we can retrive the pointer to the local player.
	m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());

	// world materials.
	Visuals::ModulateWorld();

	// init knife shit.
	g_skins.load();

	g_shots.m_shots.clear();
	m_sequences.clear();
	packet_start::sequences.clear();

	g_restore_data.Reset();
	g_fake_state.Reset();
	g_rendered_state.Reset();
	for (RebuiltAnimState& state : g_states)
		state.Reset();

	for (int i = 0; i < 64; i++) {
		g_aimbot.m_players[i].Reset();
		g_aimbot.m_players[i].m_white_listed = false;

		g_networking.GetUserData(i)->Reset();
	}

	// if the INetChannelInfo pointer has changed, store it for later.
	g_csgo.m_net = g_csgo.m_engine->GetNetChannelInfo();

	static ConVar* r_jiggle_bones = g_csgo.m_cvar->FindVar(HASH("r_jiggle_bones"));
	if (r_jiggle_bones)
		r_jiggle_bones->SetValue(0);

	static ConVar* mat_force_tonemap_scale = g_csgo.m_cvar->FindVar(HASH("mat_force_tonemap_scale"));
	if (mat_force_tonemap_scale)
		mat_force_tonemap_scale->SetValue(1.f);

	m_taps = 0;

	ConVar* p = reinterpret_cast<ConVar*>(g_csgo.m_cvar->GetCommands());
	for (auto c = p->m_next; c != nullptr; c = c->m_next) {
		c->m_flags &= ~FCVAR_DEVELOPMENTONLY;
		c->m_flags &= ~FCVAR_HIDDEN;
		c->m_flags &= ~FCVAR_CHEAT;
	}
}

void Client::StartMove(CUserCmd* cmd) {
	// save some usercmd stuff.
	m_cmd = cmd;
	m_view_angles = cmd->m_view_angles;
	m_buttons = cmd->m_buttons;
	m_tick = g_csgo.m_cl->m_server_tick;
	m_arrival_tick = m_tick + game::TIME_TO_TICKS(g_csgo.m_net->GetLatency(INetChannel::FLOW_OUTGOING));
	m_state = &g_states[m_local->index() - 1];

	if (g_csgo.m_cl->m_choked_commands <= 0)
		g_hvh.m_hide_yaw = false;

	// get local ptr.
	m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());
	if (!m_local)
		return;

	m_max_lag = g_csgo.sv_maxusrcmdprocessticks->GetInt() - 1;
	m_lerp = game::GetClientInterpAmount();
	for (int i = 0; i < 2; i++) {
		m_latency[i] = g_csgo.m_net ? g_csgo.m_net->GetLatency(i) : 0.f;
		m_latency_ticks[i] = game::TIME_TO_TICKS(m_latency[i]);
	}

	m_charged_ticks = 0;

	// processing indicates that the localplayer is valid and alive.
	m_processing = m_local && m_local->alive();
	if (!m_processing)
		return;

	// make sure prediction has ran on all usercommands.
	// because prediction runs on frames, when we have low fps it might not predict all usercommands.
	// also fix the tick being inaccurate.
	g_input_pred.update();

	// ...
	m_shot = false;
}

void Client::BackupPlayers(bool restore) {
	static std::array<BackupRecord, 64> backup;

	if (restore) {
		// restore stuff.
		for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
			Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

			if (!g_aimbot.IsValidTarget(player))
				continue;

			backup[i - 1].restore(player);
		}
		return;
	}

	// backup stuff.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!g_aimbot.IsValidTarget(player))
			continue;

		backup[i - 1].store(player);
	}
}

void Client::SetupShootPos(float pitch) {
	const vec3_t abs_origin = m_local->m_vecAbsOrigin();
	const float  pitch_pose = m_local->m_flPoseParameter()[12];

	BoneArray shot_bones[128];
	SetAnimations(g_cl.m_state);

	m_local->SetAbsOrigin(m_local->m_vecOrigin());

	m_state->m_pose_cache[AnimStatePoseParam::PLAYER_POSE_PARAM_BODY_PITCH].SetValue(m_local, pitch + m_local->m_flThirdpersonRecoil());
	g_bones.Setup(m_local, BONE_USED_BY_ANYTHING, m_curtime, shot_bones);

	m_shoot_pos = m_local->GetEyePos();
	g_cl.m_state->ModifyEyePosition(m_shoot_pos, shot_bones);

	m_local->m_vecAbsOrigin() = abs_origin;
	m_local->m_flPoseParameter()[12] = pitch_pose;
}

bool Client::PredictLand(int ticks) {
	vec3_t                start = g_cl.m_local->m_vecOrigin(), end = start, vel = g_cl.m_local->m_vecVelocity();
	CTraceFilterWorldOnly filter;
	CGameTrace            trace;

	for (int i = 0; i < ticks; i++) {
		vel.z -= (g_csgo.sv_gravity->GetFloat() * g_csgo.m_globals->m_interval);

		end += (vel * g_csgo.m_globals->m_interval);
	}

	// move down.
	end.z -= 2.f;

	g_csgo.m_engine_trace->TraceRay(Ray(start, end), MASK_SOLID, &filter, &trace);

	return trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f;
}

void Client::DoMove() {
	// run movement code before input prediction.
	g_movement.JumpRelated();
	g_movement.Strafe();
	g_movement.HandleWalk();

	// backup strafe angles (we need them for input prediction)
	m_strafe_angles = m_cmd->m_view_angles;

	// predict input.
	g_input_pred.run();

	// restore original angles after input prediction
	m_cmd->m_view_angles = m_view_angles;

	// convert viewangles to directional forward vector.
	math::AngleVectors(m_view_angles, &m_forward_dir);

	m_curtime = game::TICKS_TO_TIME(m_local->m_nTickBase());

	m_anim_velocity = math::Approach(g_cl.m_local->m_vecAbsVelocity(), m_state->m_vel, (m_curtime - m_state->m_last_update) * 2000);

	// store stuff after input pred.
	SetupShootPos(m_view_angles.x);

	// reset shit.
	m_weapon = nullptr;
	m_weapon_info = nullptr;
	m_weapon_id = -1;
	m_weapon_type = WEAPONTYPE_UNKNOWN;
	m_player_fire = m_weapon_fire = false;

	// store weapon stuff.
	m_weapon = m_local->GetActiveWeapon();

	if (m_weapon) {
		m_weapon_info = m_weapon->GetWpnData();
		m_weapon_id = m_weapon->m_iItemDefinitionIndex();
		m_weapon_type = m_weapon_info->m_weapon_type;

		// ensure weapon spread values / etc are up to date.
		if (m_weapon_type != WEAPONTYPE_EQUIPMENT)
			m_weapon->UpdateAccuracyPenalty();

		PenOutput_t output;

		// run autowall once for penetration crosshair if we have an appropriate weapon.
		if (m_weapon_type != WEAPONTYPE_KNIFE && m_weapon_type != WEAPONTYPE_C4 && m_weapon_type != WEAPONTYPE_EQUIPMENT) {
			vec3_t dir;
			math::AngleVectors(g_cl.m_view_angles, &dir);
			dir *= m_weapon_info ? m_weapon_info->m_range : 8912.f;

			PenInput_t input;
			input.m_start = m_shoot_pos;
			input.m_end = m_shoot_pos + dir;
			input.m_from = m_local;
			input.m_target = nullptr;
			input.m_pen = false;

			output = auto_wall::RunPenetration(&input);
		}

		// set pen data for penetration crosshair.
		m_pen_data = output;

		// can the player fire.
		m_player_fire = m_curtime >= m_local->m_flNextAttack() && !g_csgo.m_gamerules->m_bFreezePeriod() && !(g_cl.m_local->m_fFlags() & FL_FROZEN);

		UpdateRevolverCock();
		m_weapon_fire = CanFireWeapon();
	}

	if ((!m_weapon_fire || g_csgo.m_cl->m_choked_commands <= 0) && m_weapon_id != REVOLVER && m_weapon_type != WEAPONTYPE_EQUIPMENT) {
		m_cmd->m_buttons &= ~IN_ATTACK;
		m_cmd->m_buttons &= ~IN_ATTACK2;
	}

	if (g_input.GetKeyState(g_menu.main.misc.last_tick_defuse.get()) && g_visuals.m_c4_planted) {
		float defuse = (m_local->m_bHasDefuser()) ? 5.f : 10.f;
		float remaining = g_visuals.m_planted_c4_explode_time - m_curtime;
		float dt = remaining - defuse - (g_cl.m_latency[INetChannel::FLOW_OUTGOING] / 2.f);

		m_cmd->m_buttons &= ~IN_USE;
		if (dt <= game::TICKS_TO_TIME(2))
			m_cmd->m_buttons |= IN_USE;
	}

	g_grenades.think();

	g_hvh.SendPacket();

	m_arrival_tick += m_charged_ticks + 1;

	g_aimbot.Think();

	if (m_weapon && (m_weapon_fire || m_weapon_type == WEAPONTYPE_EQUIPMENT)) {
		if (m_weapon_type == WEAPONTYPE_EQUIPMENT) {
			m_shot = m_weapon->m_fThrowTime() > 0.f;
			m_state->DoAnimStateEvent(m_cmd, PLAYERANIMEVENT_THROW_GRENADE);
		}
		else {
			if (m_cmd->m_buttons & IN_ATTACK) {
				m_state->DoAnimStateEvent(m_cmd, PLAYERANIMEVENT_FIRE_GUN_PRIMARY);
				m_shot = true;
			}

			if ((m_cmd->m_buttons & IN_ATTACK2) && (m_weapon_id == REVOLVER || m_weapon_type == WEAPONTYPE_KNIFE)) {
				m_shot = true;
			}
		}
	}

	g_hvh.HandleShotChoke();

	g_hvh.AntiAim();
}

void Client::EndMove(CUserCmd* cmd) {
	// if matchmaking mode, anti untrust clamp.
	if (g_menu.main.config.mode.get() == 0)
		m_cmd->m_view_angles.SanitizeAngle();

	// fix our movement.
	g_movement.FixMove(cmd, m_strafe_angles);

	m_origin = m_local->m_vecOrigin();

	// this packet will be sent.
	if (m_packet) {
		g_networking.SendData();

		packet_start::sequences.emplace_back(cmd->m_command_number);

		// we are sending a packet, so this will be reset soon.
		// store the old value.
		m_old_lag = g_csgo.m_cl->m_choked_commands;

		// get radar angles.
		m_radar = cmd->m_view_angles;
		m_radar.normalize();

		// get prevoius origin.
		vec3_t prev = m_net_pos.empty() ? m_origin : m_net_pos.front().m_pos;

		// check if we broke lagcomp.
		m_lagcomp = (m_origin - prev).length_sqr() > 4096.f;

		// save sent origin and time.
		m_net_pos.emplace_front(g_cl.m_curtime, m_origin);
	}
	else {
		INetChannel* nci = g_csgo.m_cl->m_net_channel;
		if (nci) {
			const int choke = nci->m_choked_packets;

			nci->m_choked_packets = 0;
			nci->SendDatagram(nullptr);
			--nci->m_out_seq;

			nci->m_choked_packets = choke;
		}
	}

	RestoreVars* unpredicted_data = g_restore_data.GetVars(m_cmd->m_command_number - 1);

	if ((unpredicted_data->m_fFlags & FL_ONGROUND) && (m_cmd->m_buttons & IN_JUMP))
		m_state->DoAnimStateEvent(m_cmd, PLAYERANIMEVENT_JUMP);

	// update client-side animations.
	if (g_csgo.m_cl->m_choked_commands <= 0)
		UpdateInformation(m_state, m_cmd, m_curtime);

	// restore curtime/frametime
	// and prediction seed/player.
	g_input_pred.restore();

	// store some values for next tick.
	m_old_packet = m_packet;
	m_old_shot = m_shot;

	cmd->m_tick = std::min(cmd->m_tick, m_tick + g_csgo.sv_max_usercmd_future_ticks->GetInt());
}

void Client::OnTick(CUserCmd* cmd) {
	// store some data and update prediction.
	StartMove(cmd);

	// not much more to do here.
	if (!m_processing) {
		g_restore_data.Reset();
		return;
	}

	// save the original state of players.
	BackupPlayers(false);

	// run all movement related code.
	DoMove();

	// store stome additonal stuff for next tick
	// sanetize our usercommand if needed and fix our movement.
	EndMove(cmd);

	// restore the players.
	BackupPlayers(true);
}

void Client::SetAnimations(const RebuiltAnimState* rebuilt_state) const {
	if (!m_local || !m_processing)
		return;

	if (!rebuilt_state || rebuilt_state->m_layers[0].m_owner != m_local)
		return;

	rebuilt_state->SetAnimation(m_local);
}

void Client::HandleRenderedAnimation(const RebuiltAnimState* rebuilt_state) {
	if (!rebuilt_state)
		return;

	if (g_hvh.m_hide_yaw) {
		for (int i = 0; i < 24; i++) {
			if (i < ANIMATION_LAYER_COUNT && i != ANIMATION_LAYER_ADJUST) {
				g_rendered_state.m_layers[i] = rebuilt_state->m_layers[i];
			}

			if (i != POSE_STRAFE_YAW && i != POSE_BODY_YAW)
				g_rendered_state.m_poses[i] = rebuilt_state->m_poses[i];
		}
		return;
	}

	g_rendered_state = *rebuilt_state;
}

void Client::UpdateInformation(RebuiltAnimState* rebuilt_state, CUserCmd* cmd, const float& curtime) {
	CCSGOPlayerAnimState* state = m_local->m_PlayerAnimState();
	if (!state)
		return;

	const float backup_curtime = g_csgo.m_globals->m_curtime;
	const float backup_frametime = g_csgo.m_globals->m_frametime;
	const int backup_eflags = m_local->m_iEFlags();

	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	// current angle will be animated.
	m_angle = cmd->m_view_angles;

	math::clamp(m_angle.x, -90.f, 90.f);
	m_angle.normalize();

	// set lby to predicted value.
	m_local->m_flLowerBodyYawTarget() = rebuilt_state->m_body_yaw;

	// CCSGOPlayerAnimState::Update, bypass already animated checks.
	state->m_last_update_frame = 0;

	m_local->m_iEFlags() &= ~EFL_DIRTY_ABSVELOCITY;
	m_local->m_iEFlags() &= ~EFL_DIRTY_ABSTRANSFORM;

	// call original, bypass hook.
	update_clientside_animation::allow = true;
	m_local->UpdateClientSideAnimation();
	update_clientside_animation::allow = false;

	const RebuiltAnimState prev_state = *rebuilt_state;

	rebuilt_state->UpdateLayers(m_local, m_angle, curtime, true, cmd);

	g_hvh.m_updated = false;

	if (rebuilt_state->m_vel_xy > 0.1f && rebuilt_state->m_ground) {
		g_hvh.m_last_move_time = curtime;
		g_hvh.m_last_moving_yaw = m_angle.y;
		g_hvh.m_updates = 0;
	}
	else if (rebuilt_state->m_body_update != m_body_update) {
		g_hvh.m_last_standing_yaw = m_angle.y;

		if (rebuilt_state->m_body_yaw != m_body)
			g_hvh.m_updates = 0;

		++g_hvh.m_updates;

		g_hvh.m_updated = true;
	}

	m_body_update = rebuilt_state->m_body_update;
	m_body = rebuilt_state->m_body_yaw;
	m_foot_yaw = rebuilt_state->m_foot_yaw;
	m_ground = rebuilt_state->m_ground;
	m_last_update = rebuilt_state->m_last_update;

	g_fake_state.m_body_yaw = rebuilt_state->m_body_yaw;
	g_fake_state.UpdateLayers(m_local, { m_angle.x, rebuilt_state->m_body_yaw, 0 }, curtime, false, cmd);

	g_csgo.m_globals->m_curtime = backup_curtime;
	g_csgo.m_globals->m_frametime = backup_frametime;
	m_local->m_iEFlags() = backup_eflags;

	// set lby to predicted value.
	m_local->m_flLowerBodyYawTarget() = rebuilt_state->m_body_yaw;
}

void Client::print(const std::string text, ...) {
	va_list     list;
	int         size;
	std::string buf;

	if (text.empty())
		return;

	va_start(list, text);

	// count needed size.
	size = std::vsnprintf(0, 0, text.c_str(), list);

	// allocate.
	buf.resize(size);

	// print to buffer.
	std::vsnprintf(buf.data(), size + 1, text.c_str(), list);

	va_end(list);

	g_csgo.m_cvar->ConsoleColorPrintf(g_gui.m_color, g_menu.main.misc.agartha.get() ? XOR("[AgarthaHack] ") : XOR("[rax] ")); //print our mark
	g_csgo.m_cvar->ConsoleColorPrintf(colors::white, buf.c_str()); //print our buffer converted to string
}

bool Client::CanFireWeapon() {
	// the player cant fire.
	if (!m_player_fire)
		return false;

	if (m_weapon_type == WEAPONTYPE_EQUIPMENT)
		return false;

	// if we have no bullets, we cant shoot.
	if (m_weapon_type != WEAPONTYPE_KNIFE && m_weapon->m_iClip1() < 1)
		return false;

	// do we have any burst shots to handle?
	if ((m_weapon_id == GLOCK || m_weapon_id == FAMAS) && m_weapon->m_iBurstShotsRemaining() > 0) {
		// new burst shot is coming out.
		if (m_curtime >= m_weapon->m_fNextBurstShot())
			return true;
	}

	// r8 revolver.
	if (m_weapon_id == REVOLVER) {
		int act = m_weapon->m_Activity();

		// mouse1.
		if (!m_revolver_fire) {
			if ((act == 185 || act == 193) && m_revolver_cock == 0)
				return m_curtime >= m_weapon->m_flNextPrimaryAttack();

			return false;
		}
	}

	// yeez we have a normal gun.
	if (m_curtime >= m_weapon->m_flNextPrimaryAttack())
		return true;

	return false;
}

void Client::UpdateRevolverCock() {
	// default to false.
	m_revolver_fire = false;

	// reset properly.
	if (m_revolver_cock == -1)
		m_revolver_cock = 0;

	// we dont have a revolver.
	// we have no ammo.
	// player cant fire
	// we are waiting for we can shoot again.
	if (m_weapon_id != REVOLVER || m_weapon->m_iClip1() < 1 || !m_player_fire || m_curtime < m_weapon->m_flNextPrimaryAttack()) {
		// reset.
		m_revolver_cock = 0;
		m_revolver_query = 0;
		return;
	}

	// calculate max number of cocked ticks.
	// round to 6th decimal place for custom tickrates..
	int shoot = (int)(0.25f / (std::round(g_csgo.m_globals->m_interval * 1000000.f) / 1000000.f));

	// amount of ticks that we have to query.
	m_revolver_query = shoot - 1;

	// we held all the ticks we needed to hold.
	if (m_revolver_query == m_revolver_cock) {
		// reset cocked ticks.
		m_revolver_cock = -1;

		// we are allowed to fire, yay.
		m_revolver_fire = true;
	}

	else {
		// we still have ticks to query.
		// apply inattack.
		if (g_menu.main.config.mode.get() == 0 && m_revolver_query > m_revolver_cock)
			m_cmd->m_buttons |= IN_ATTACK;

		// count cock ticks.
		// do this so we can also count 'legit' ticks
		// that didnt originate from the hack.
		if (m_cmd->m_buttons & IN_ATTACK)
			m_revolver_cock++;

		// inattack was not held, reset.
		else m_revolver_cock = 0;
	}

	// remove inattack2 if cocking.
	if (m_revolver_cock > 0)
		m_cmd->m_buttons &= ~IN_ATTACK2;
}

void Client::UpdateIncomingSequences() {
	/* update net channel ptr */
	g_csgo.m_net = g_csgo.m_engine->GetNetChannelInfo();

	if (!g_csgo.m_net)
		return;

	if (m_sequences.empty() || g_csgo.m_net->m_in_seq > m_sequences.front().m_seq) {
		// store new stuff.
		m_sequences.emplace_front(g_csgo.m_globals->m_realtime, g_csgo.m_net->m_in_rel_state, g_csgo.m_net->m_in_seq);
	}

	// do not save too many of these.
	while (m_sequences.size() > 2048)
		m_sequences.pop_back();
}