#include "includes.h"

Shots g_shots{ };

void Shots::HandleMisses() {
	if (!g_cl.m_local)
		return;

	for (ShotRecord& shot : g_shots.m_shots) {
		// the impact was not matched yet.
		if (shot.m_status == ShotStatus::STATUS_INVALID)
			continue;

		// we already logged this shot.
		if (shot.m_logged)
			continue;

		// mark as logged.
		shot.m_logged = true;

		// the target died before our impact registered.
		if (shot.m_status == ShotStatus::STATUS_MISS_DEATH) {
			g_notify.add(XOR("missed shot due to target death\n"));
			continue;
		}

		// we missed due to spread
		if (shot.m_status == ShotStatus::STATUS_MISS_SPREAD) {
			g_notify.add(XOR("missed shot due to spread\n"));
			continue;
		}

		// we hit the target.
		if (shot.m_status == ShotStatus::STATUS_HIT) {
			if (shot.m_hitgroup != shot.m_target_hitgroup) {
				if (shot.m_hitgroup != shot.m_simulated_hitgroup) {
					if (shot.m_record.m_predicted) {
						g_notify.add(XOR("missed target point due to spread and extrapolation\n"));
						continue;
					}

					g_notify.add(XOR("missed target point due to spread and fake angles\n"));
					continue;
				}

				g_notify.add(XOR("missed target point due to spread\n"));
				continue;
			}

			if (shot.m_hitgroup == shot.m_simulated_hitgroup)
				continue;

			if (shot.m_record.m_predicted) {
				g_notify.add(XOR("missed target point due extrapolation\n"));
				continue;
			}

			g_notify.add(XOR("missed target point due fake angles\n"));
			continue;
		}

		AimPlayer* data = &g_aimbot.m_players[shot.m_target_index - 1];
		if (!data)
			continue;

		if (shot.m_record.m_predicted) {
			g_notify.add(XOR("missed shot due to extrapolation\n"));
			continue;
		}

		switch (shot.m_record.m_resolver_mode) {
		case RESOLVE_NONE:
		case RESOLVE_MOVE:
		case RESOLVE_UPDATE:
		case RESOLVE_AIR_UPDATE:
			g_notify.add(XOR("missed shot due to animations\n"));
			break;
		case RESOLVE_STAND:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_STAND];
			break;
		case RESOLVE_STAND_BALANCE:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_STAND_BALANCE];
			break;
		case RESOLVE_STAND_BALANCE2:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_STAND_BALANCE2];
			break;
		case RESOLVE_STAND_NO_DATA:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_STAND_NO_DATA];
			break;
		case RESOLVE_DISTORTION:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_STAND_DISTORTION];
			break;
		case RESOLVE_NOUPDATE:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_NOUPDATE];
			break;
		case RESOLVE_UPDATE_PRED:
			g_notify.add(XOR("missed shot due to flick prediction\n"));
			++data->m_misses[MISS_UPDATE_PRED];
			break;
		case RESOLVE_AIR:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			++data->m_misses[MISS_AIR];
			break;
		default:
			g_notify.add(XOR("missed shot due to fake angles\n"));
			break;
		}
	}
}

void Shots::OnShotFire(PointData* data) {

	ShotRecord& shot = m_shots.emplace_front();

	std::memcpy(&shot.m_record, data->m_record, sizeof(LagRecord));
	shot.m_target_index = data->m_record->m_player->index();
	shot.m_target_hitgroup = data->m_hitgroup;
	shot.m_tick = g_csgo.m_cl->m_server_tick;
	shot.m_lat = g_cl.m_latency[INetChannel::FLOW_INCOMING] + g_cl.m_latency[INetChannel::FLOW_OUTGOING] + game::TICKS_TO_TIME(g_cl.m_charged_ticks);
	shot.m_safety = data->m_safety;

	if (g_cl.m_weapon && g_cl.m_weapon_info)
		shot.m_range = g_cl.m_weapon_info->m_range;

	shot.m_start = g_cl.m_shoot_pos;

	while (m_shots.size() > 32)
		m_shots.pop_back();

	static player_info_t playerinfo;
	//shot logs
	if (g_menu.main.misc.shot_logs.get(0) && g_csgo.m_engine->GetPlayerInfo(shot.m_target_index, &playerinfo)) {
		std::string log = tfm::format(XOR("fired shot at \"%s\" "), playerinfo.m_name);

		static std::string hitbox;

		// little array of resolver modes;
		static std::string resolver_modes[]{ XOR("none"), XOR("move"), XOR("stand"), XOR("stand_no_data"), XOR("distortion"), XOR("no_update"), XOR("pre_update"), XOR("update"), XOR("update_prediction"), XOR("air"), XOR("override") };

		auto& active = g_menu.main.misc.shot_logs.GetActiveIndices();
		for (auto it = active.begin(); it != active.end(); ++it) {
			switch (*it) {
			case 1:
				log.append(tfm::format(XOR("for %i damage "), data->m_damage));
				break;
			case 2:
				switch (data->m_hitgroup) {
				case HITGROUP_GENERIC:
					hitbox = XOR("generic");
					break;
				case HITGROUP_HEAD:
					hitbox = XOR("head");
					break;
				case HITGROUP_CHEST:
				case HITGROUP_STOMACH:
					hitbox = XOR("body");
					break;
				case HITGROUP_LEFTARM:
				case HITGROUP_RIGHTARM:
					hitbox = XOR("arms");
					break;
				case HITGROUP_LEFTLEG:
				case HITGROUP_RIGHTLEG:
					hitbox = XOR("legs");
					break;
				case HITGROUP_GEAR:
					hitbox = XOR("gear");
					break;
				default:
					hitbox = XOR("invalid");
				}

				log.append(tfm::format(XOR("hb: %s "), hitbox));
				break;
			case 3:
				log.append(tfm::format(XOR("hc: %i%% "), static_cast<int>(data->m_hitchance * 100.f)));
				break;
			case 4:
				log.append(tfm::format(XOR("safety: %i%% "), static_cast<int>(data->m_safety * 100.f)));
				break;
			case 5:
				log.append(tfm::format(XOR("choke: %i "), data->m_record->m_choke));
				break;
			case 6:
				log.append(tfm::format(XOR("resolved: %s "), data->m_record->IsResolved() ? XOR("true") : XOR("false")));
				break;
			case 7:
				log.append(tfm::format(XOR("lbu: %s "), data->m_record->IsBodyUpdate() ? XOR("true") : XOR("false")));
				break;
			case 8:
				log.append(tfm::format(XOR("lagcomp: %s "), (!data->m_record->m_lagcomp[0] && !data->m_record->m_lagcomp[1]) ? XOR("true") : XOR("false")));
				break;
			case 9:
				log.append(tfm::format(XOR("mode: %s "), resolver_modes[data->m_record->m_resolver_mode]));
				break;
			}
		}

		log.append(XOR("\n"));
		g_notify.add(log);
	}
}

void Shots::OnImpact(IGameEvent* evt) {
	// screw this.
	if (!evt || !g_cl.m_local)
		return;

	// get attacker, if its not us, screw it.
	const int& attacker = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (attacker != g_csgo.m_engine->GetLocalPlayer())
		return;

	// decode impact coordinates and convert to vec3.
	vec3_t pos = {
		evt->m_keys->FindKey(HASH("x"))->GetFloat(),
		evt->m_keys->FindKey(HASH("y"))->GetFloat(),
		evt->m_keys->FindKey(HASH("z"))->GetFloat()
	};

	if (g_menu.main.visuals.bullet_impacts.get(1))
		g_csgo.m_debug_overlay->AddBoxOverlay(pos, { -2, -2, -2 }, { 2, 2, 2 }, { 0, 0, 0 }, 0, 0, 255, 127, 4.f);

	const int& tick = g_csgo.m_cl->m_server_tick;
	static int last_impact_tick = -1;

	if (tick == last_impact_tick)
		return;

	last_impact_tick = tick;

	// we did not take a shot yet.
	if (m_shots.empty())
		return;

	ShotRecord* current = nullptr;
	for (auto it = m_shots.begin(); it != m_shots.end(); ) {
		// we already scanned this shot.
		if (it->m_status != ShotStatus::STATUS_INVALID) {
			it = m_shots.erase(it);
			continue;
		}

		// this shot falls out of our forgiveness.
		if ((tick - it->m_tick) > game::TIME_TO_TICKS(it->m_lat + 0.2f)) {
			it = m_shots.erase(it);
			continue;
		}

		current = &*it;

		++it;
	}

	if (!current)
		return;

	Player* target = g_csgo.m_entlist->GetClientEntity< Player* >(current->m_target_index);
	if (!target)
		return;

	if (!target->alive()) {
		current->m_status = ShotStatus::STATUS_MISS_DEATH;
		return;
	}

	current->m_record.SetData(target);

	g_aimbot.m_input.m_from = g_cl.m_local;
	g_aimbot.m_input.m_target = target;
	g_aimbot.m_input.m_start = current->m_start;
	g_aimbot.m_input.m_end = current->m_start + (pos - current->m_start) * current->m_range;
	g_aimbot.m_input.m_pen = true;

	// mark as a miss for now.
	if (PenOutput_t* output = &auto_wall::RunPenetration(&g_aimbot.m_input); output->m_damage >= 1) {
		current->m_status = ShotStatus::STATUS_MISS_ANIMATION;
		current->m_simulated_hitgroup = output->m_hitgroup;
		return;
	}

	current->m_status = ShotStatus::STATUS_MISS_SPREAD;
}

std::string hitsounds[] = {
	"buttons/arena_switch_press_02.wav",
	"training/timer_bell.wav"
};

void Shots::OnHurt(IGameEvent* evt) {
	if (!evt || !g_cl.m_local)
		return;

	const int& attacker = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("attacker"))->GetInt());
	const int& victim = g_csgo.m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

	// we were not the attacker or we hurt ourselves.
	if (attacker != g_csgo.m_engine->GetLocalPlayer() || victim == g_csgo.m_engine->GetLocalPlayer())
		return;

	// get hitgroup.
	// players that get naded ( DMG_BLAST ) or stabbed seem to be put as HITGROUP_GENERIC.
	const int& group = evt->m_keys->FindKey(HASH("hitgroup"))->GetInt();

	// invalid hitgroups ( note - dex; HITGROUP_GEAR isn't really invalid, seems to be set for hands and stuff? ).
	if (!game::IsValidHitgroup(group))
		return;

	// get the player that was hurt.
	Player* target = g_csgo.m_entlist->GetClientEntity< Player* >(victim);
	if (!target)
		return;

	// get player info.
	player_info_t info;
	if (!g_csgo.m_engine->GetPlayerInfo(victim, &info))
		return;

	// get player name;
	const std::string& name = std::string(info.m_name).substr(0, 24);

	// get damage reported by the server.
	const int& damage = evt->m_keys->FindKey(HASH("dmg_health"))->GetInt();

	// get remaining hp.
	const int& hp = evt->m_keys->FindKey(HASH("health"))->GetInt();

	//volume
	float volume = g_menu.main.misc.hitsoundvol.get() / 100;

	// hitmarker sound
	if (g_menu.main.misc.hitsound.get() > 0) //if not on "off" position
		g_csgo.m_sound->EmitAmbientSound(hitsounds[g_menu.main.misc.hitsound.get() - 1].c_str(), volume);

	// print this shit.
	if (g_menu.main.misc.notifications.get(1)) {
		std::string out = tfm::format(XOR("hit %s in the %s for %i damage (%i health remaining)\n"), name, m_groups[group], damage, hp);
		g_notify.add(out);
	}

	if (group == HITGROUP_GENERIC)
		return;

	// increment our taps if we killed him with a headshot.
	if (group == HITGROUP_HEAD && hp <= 0)
		++g_cl.m_taps;

	// no impacts to match.
	if (m_shots.empty())
		return;

	const int& tick = g_csgo.m_cl->m_server_tick;

	ShotRecord* current = nullptr;
	for (auto it = m_shots.begin(); it != m_shots.end(); ) {
		// we already scanned this shot.
		if (it->m_status > ShotStatus::STATUS_MISS_DEATH) {
			++it;
			continue;
		}

		// this shot falls out of our forgiveness.
		if ((tick - it->m_tick) > game::TIME_TO_TICKS(it->m_lat + 0.2f)) {
			++it;
			continue;
		}

		current = &*it;

		++it;
	}

	if (!current)
		return;

	current->m_status = ShotStatus::STATUS_HIT;
	current->m_hitgroup = group;

	if (!g_menu.main.players.skeleton.get(3))
		return;

	g_visuals.DrawHitboxMatrix(target, current->m_record.m_bones, g_menu.main.players.hit_color.get(), 4.f);
}