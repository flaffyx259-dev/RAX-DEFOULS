#include "includes.h"

Visuals g_visuals{ };;

static std::string skyboxes[] = {
	XOR("cs_tibet"),
	XOR("embassy"),
	XOR("italy"),
	XOR("sky_cs15_daylight01_hdr"),
	XOR("sky_csgo_cloudy01"),
	XOR("sky_csgo_night02"),
	XOR("sky_csgo_night02b"),
	XOR("sky_csgo_night_flat"),
	XOR("sky_day02_05_hdr"),
	XOR("sky_day02_05"),
	XOR("sky_l4d_rural02_ldr"),
	XOR("vertigo_hdr"),
	XOR("vertigoblue_hdr"),
	XOR("vertigo"),
	XOR("vietnam"),
	XOR("sky_dust"),
	XOR("jungle"),
	XOR("nukeblank"),
	XOR("office"),
};

void Visuals::ModulateWorld() {
	std::vector< IMaterial* > world, props, sky;

	// iterate material handles.
	for (uint16_t h{ g_csgo.m_material_system->FirstMaterial() }; h != g_csgo.m_material_system->InvalidMaterial(); h = g_csgo.m_material_system->NextMaterial(h)) {
		// get material from handle.
		IMaterial* mat = g_csgo.m_material_system->GetMaterial(h);
		if (!mat)
			continue;

		// store world materials.
		if (FNV1a::get(mat->GetTextureGroupName()) == HASH("World textures")) {
			world.push_back(mat);
			continue;
		}

		// store props.
		if (FNV1a::get(mat->GetTextureGroupName()) == HASH("StaticProp textures")) {
			props.push_back(mat);
			continue;
		}

		// store skybox
		if (FNV1a::get(mat->GetTextureGroupName()) == HASH("SkyBox textures")) {
			sky.push_back(mat);
			continue;
		}
	}
	// night
	Color& ref = g_menu.main.visuals.world_color.get();
	Color& ref2 = g_menu.main.visuals.sky_color.get();

	const int r = ref.r() * (ref.a() / 255.f);
	const int g = ref.g() * (ref.a() / 255.f);
	const int b = ref.b() * (ref.a() / 255.f);
	const int a = ref.a() * (ref.a() / 255.f);

	const int r2 = ref2.r() * (ref2.a() / 255.f);
	const int g2 = ref2.g() * (ref2.a() / 255.f);
	const int b2 = ref2.b() * (ref2.a() / 255.f);

	Color world_clr = { (r * r) / 255, (g * g) / 255, (b * b) / 255, 255 };
	Color props_clr = { r, g, b, 255 };
	Color sky_clr = { r2, g2, b2, 255 };

	const float transparency = 1.f - (g_menu.main.visuals.prop_transparency.get() / 100.f);

	if (g_csgo.r_DrawSpecificStaticProp->GetInt() != 0)
	{
		g_csgo.r_DrawSpecificStaticProp->SetValue(0);
	}

	if (g_menu.main.visuals.world.get(0)) {
		for (const auto& p : props)
		{
			p->ColorModulate(props_clr);
			p->AlphaModulate(transparency);
		}

		for (const auto& w : world)
			w->ColorModulate(world_clr);
	}
	else {
		for (const auto& p : props)
		{
			p->ColorModulate({ 255, 255, 255, 255 });
			p->AlphaModulate(transparency);
		}

		for (const auto& w : world)
			w->ColorModulate({ 255, 255, 255, 255 });
	}

	if (g_menu.main.visuals.world.get(2)) {
		for (const auto& s : sky)
			s->ColorModulate(sky_clr);
	}
	else {
		for (const auto& s : sky)
			s->ColorModulate({ 255, 255, 255, 255 });
	}

	static ConVar* cl_csm_shadows = g_csgo.m_cvar->FindVar(HASH("cl_csm_shadows"));
	if (cl_csm_shadows)
		cl_csm_shadows->SetValue(g_menu.main.visuals.world.get(3) ? 0 : 1);

	static auto load_named_sky = pattern::find(g_csgo.m_engine_dll, XOR("55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45")).as<void(__fastcall*)(const char*)>();

	if (g_menu.main.visuals.sky_box.get() > 0)
		load_named_sky(skyboxes[g_menu.main.visuals.sky_box.get() - 1].c_str());
}

void Visuals::ThirdpersonThink() {
	ang_t                          offset;
	vec3_t                         origin, forward;
	static CTraceFilterSimple_game filter{ };
	CGameTrace                     tr;

	// for whatever reason overrideview also gets called from the main menu.
	if (!g_csgo.m_engine->IsInGame())
		return;

	// check if we have a local player and he is alive.
	bool alive = g_cl.m_local && g_cl.m_local->alive();

	// camera should be in thirdperson.
	if (m_thirdperson) {

		// if alive and not in thirdperson already switch to thirdperson.
		if (alive && !g_csgo.m_input->CAM_IsThirdPerson())
			g_csgo.m_input->CAM_ToThirdPerson();

		// if dead and spectating in firstperson switch to thirdperson.
		else if (g_cl.m_local->m_iObserverMode() == 4) {

			// if in thirdperson, switch to firstperson.
			// we need to disable thirdperson to spectate properly.
			if (g_csgo.m_input->CAM_IsThirdPerson()) {
				g_csgo.m_input->CAM_ToFirstPerson();
				g_csgo.m_input->m_camera_offset.z = 0.f;
			}

			g_cl.m_local->m_iObserverMode() = 5;
		}
	}

	// camera should be in firstperson.
	else if (g_csgo.m_input->CAM_IsThirdPerson()) {
		g_csgo.m_input->CAM_ToFirstPerson();
		g_csgo.m_input->m_camera_offset.z = 0.f;
	}

	// if after all of this we are still in thirdperson.
	if (g_csgo.m_input->CAM_IsThirdPerson()) {
		// get camera angles.
		g_csgo.m_engine->GetViewAngles(offset);

		// get our viewangle's forward directional vector.
		math::AngleVectors(offset, &forward);

		// cam_idealdist convar.
		offset.z = g_csgo.m_cvar->FindVar(HASH("cam_idealdist"))->GetFloat();

		// start pos.
		origin = g_cl.m_shoot_pos - g_cl.m_origin + g_cl.m_local->m_vecAbsOrigin();

		// setup trace filter and trace.
		filter.SetPassEntity(g_cl.m_local);

		g_csgo.m_engine_trace->TraceRay(
			Ray(origin, origin - (forward * offset.z), { -16.f, -16.f, -16.f }, { 16.f, 16.f, 16.f }),
			MASK_NPCWORLDSTATIC,
			(ITraceFilter*)&filter,
			&tr
		);

		// adapt distance to travel time.
		math::clamp(tr.m_fraction, 0.f, 1.f);
		offset.z *= tr.m_fraction;

		// override camera angles.
		g_csgo.m_input->m_camera_offset = { offset.x, offset.y, offset.z };
	}
}

void Visuals::Hitmarker() {
	if (!g_menu.main.misc.hitmarker.get())
		return;

	static auto RenderHitmarker = [&](vec2_t vecCenter, const int nPaddingFromCenter, const int nSize, Color color) {
		render::line(vecCenter - nPaddingFromCenter, vecCenter - nPaddingFromCenter - nSize, color);
		render::line(vecCenter + nPaddingFromCenter, vecCenter + nPaddingFromCenter + nSize, color);

		render::line(vecCenter - vec2_t(-nPaddingFromCenter, nPaddingFromCenter), vecCenter - vec2_t(-nPaddingFromCenter - nSize, nPaddingFromCenter + nSize), color);
		render::line(vecCenter + vec2_t(-nPaddingFromCenter, nPaddingFromCenter), vecCenter + vec2_t(-nPaddingFromCenter - nSize, nPaddingFromCenter + nSize), color);
		};

	const float servertime = game::TICKS_TO_TIME(g_csgo.m_cl->m_server_tick);
	const float duration = g_menu.main.misc.hit_animation_time.get();

	if (duration <= 0.f)
		return;

	const int x = g_cl.m_width / 2,
		y = g_cl.m_height / 2;

	for (ShotRecord& shot : g_shots.m_shots)
	{
		/* check if we hit the target*/
		if (shot.m_status != ShotStatus::STATUS_HIT)
			continue;

		/* check if we should render the impact */
		const float complete[] = {
			(servertime - game::TICKS_TO_TIME(shot.m_tick)),
			(servertime - game::TICKS_TO_TIME(shot.m_tick)) / duration
		};

		if (complete[1] <= 0.f || complete[1] > 1.f)
			continue;

		const int alpha = (1.f - complete[0]) * 240;

		RenderHitmarker(vec2_t(x, y), 4, 4, Color(255, 255, 255, alpha));
		break;
	}
}

void Visuals::NoSmoke() {
	if (!smoke1)
		smoke1 = g_csgo.m_material_system->FindMaterial(XOR("particle/vistasmokev1/vistasmokev1_fire"), XOR("Other textures"));

	if (!smoke2)
		smoke2 = g_csgo.m_material_system->FindMaterial(XOR("particle/vistasmokev1/vistasmokev1_smokegrenade"), XOR("Other textures"));

	if (!smoke3)
		smoke3 = g_csgo.m_material_system->FindMaterial(XOR("particle/vistasmokev1/vistasmokev1_emods"), XOR("Other textures"));

	if (!smoke4)
		smoke4 = g_csgo.m_material_system->FindMaterial(XOR("particle/vistasmokev1/vistasmokev1_emods_impactdust"), XOR("Other textures"));

	if (g_menu.main.visuals.nosmoke.get()) {
		if (!smoke1->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke1->SetFlag(MATERIAL_VAR_NO_DRAW, true);

		if (!smoke2->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke2->SetFlag(MATERIAL_VAR_NO_DRAW, true);

		if (!smoke3->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke3->SetFlag(MATERIAL_VAR_NO_DRAW, true);

		if (!smoke4->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke4->SetFlag(MATERIAL_VAR_NO_DRAW, true);
	}

	else {
		if (smoke1->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke1->SetFlag(MATERIAL_VAR_NO_DRAW, false);

		if (smoke2->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke2->SetFlag(MATERIAL_VAR_NO_DRAW, false);

		if (smoke3->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke3->SetFlag(MATERIAL_VAR_NO_DRAW, false);

		if (smoke4->GetFlag(MATERIAL_VAR_NO_DRAW))
			smoke4->SetFlag(MATERIAL_VAR_NO_DRAW, false);
	}
}

void Visuals::RemoveRecoil(bool store) {
	if (!g_menu.main.visuals.novisrecoil.get())
		return;

	if (!g_cl.m_local || !g_cl.m_local->alive())
		return;

	if (store) {
		m_view_punch = g_cl.m_local->m_viewPunchAngle();
		m_aim_punch = g_cl.m_local->m_aimPunchAngle();

		g_cl.m_local->m_viewPunchAngle() = g_cl.m_local->m_aimPunchAngle() = { 0, 0, 0 };
		return;
	}

	g_cl.m_local->m_viewPunchAngle() = m_view_punch;
	g_cl.m_local->m_viewPunchAngle() = m_aim_punch;
}

void Visuals::RenderOverrideArrow()
{
	g_resolver.Override();

	if (!g_resolver.m_override_data.m_player)
		return;

	// here we do arrows
	vec2_t screen_start, screen_end;

	static Color clr = { 168, 230, 69, 205 };

	if (render::WorldToScreen(g_resolver.m_override_data.m_start, screen_start))
	{
		if (render::WorldToScreen(g_resolver.m_override_data.m_end, screen_end))
		{
			vec2_t delta = screen_end - screen_start;

			ang_t dir;
			math::VectorAngles(g_resolver.m_override_data.m_end - g_resolver.m_override_data.m_start, dir);

			render::line(screen_start, screen_end, clr);

			float ang = math::NormalizedAngle(g_resolver.m_override_data.m_yaw);

			if (fabs(ang) <= 45.f)
				render::triangle(screen_end + vec2_t(0, -17), screen_end + vec2_t(10, 0), screen_end + vec2_t(-10, 0), clr);

			else if (fabs(ang) >= 135.f)
				render::triangle(screen_end + vec2_t(0, 17), screen_end + vec2_t(-10, 0), screen_end + vec2_t(10, 0), clr);

			else if (delta.x < 0)
				render::triangle(screen_end + vec2_t(-17, 0), screen_end + vec2_t(0, -10), screen_end + vec2_t(0, 10), clr);

			else if (delta.x > 0)
				render::triangle(screen_end + vec2_t(17, 0), screen_end + vec2_t(0, 10), screen_end + vec2_t(0, -10), clr);
		}
	}
}

void Visuals::think() {
	// don't run anything if our local player isn't valid.
	if (!g_cl.m_local)
		return;

	if (g_menu.main.visuals.noscope.get()
		&& g_cl.m_local->alive()
		&& g_cl.m_local->GetActiveWeapon()
		&& g_cl.m_local->GetActiveWeapon()->GetWpnData()->m_weapon_type == CSWeaponType::WEAPONTYPE_SNIPER_RIFLE
		&& g_cl.m_local->m_bIsScoped()) {

		// rebuild the original scope lines.
		int w = g_cl.m_width,
			h = g_cl.m_height,
			x = w / 2,
			y = h / 2,
			size = g_csgo.cl_crosshair_sniper_width->GetInt();

		// Here We Use The Euclidean distance To Get The Polar-Rectangular Conversion Formula.
		if (size > 1) {
			x -= (size / 2);
			y -= (size / 2);
		}

		// draw our lines.
		render::rect_filled(0, y, w, size, colors::black);
		render::rect_filled(x, 0, size, h, colors::black);
	}

	// draw esp on ents.
	for (int i{ 1 }; i <= g_csgo.m_entlist->GetHighestEntityIndex(); ++i) {
		Entity* ent = g_csgo.m_entlist->GetClientEntity(i);
		if (!ent)
			continue;

		draw(ent);
	}

	DebugAimbotPoints();

	// draw everything else.
	SpreadCrosshair();
	StatusIndicators();
	Spectators();
	PenetrationCrosshair();
	Hitmarker();
	DrawPlantedC4();

	// resolver stuff.
	RenderOverrideArrow();
}

void Visuals::Spectators() {
	if (!g_menu.main.visuals.spectators.get())
		return;

	std::vector< std::string > spectators{ XOR("spectators") };
	int h = render::menu_shade.m_size.m_height;

	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);
		if (!player)
			continue;

		if (player->m_bIsLocalPlayer())
			continue;

		if (player->dormant())
			continue;

		if (player->m_lifeState() == LIFE_ALIVE)
			continue;

		if (player->GetObserverTarget() != g_cl.m_local)
			continue;

		player_info_t info;
		if (!g_csgo.m_engine->GetPlayerInfo(i, &info))
			continue;

		spectators.push_back(std::string(info.m_name).substr(0, 24));
	}

	size_t total_size = spectators.size() * (h - 1);

	for (size_t i{ }; i < spectators.size(); ++i) {
		const std::string& name = spectators[i];

		render::menu_shade.string(g_cl.m_width - 20, (g_cl.m_height / 2) - (total_size / 2) + (i * (h - 1)),
			{ 255, 255, 255, 179 }, name, render::ALIGN_RIGHT);
	}
}

void Visuals::StatusIndicators() {
	// dont do if dead.
	if (!g_cl.m_processing)
		return;

	int size[] = { 72, 55, 10 };

	int x = g_cl.m_width / 2, y = g_cl.m_height / 2;

	Color color = (g_hvh.m_left + g_hvh.m_right + g_hvh.m_back + g_hvh.m_front) >= 2 ? colors::transparent_red : colors::transparent_light_blue;

	static float alpha = 0;
	static int   flip = 1;

	if (g_hvh.m_left || g_hvh.m_right || g_hvh.m_back || g_hvh.m_front) {
		alpha += g_csgo.m_globals->m_frametime * flip;

		if (alpha >= 200.f) {
			flip = -73;
			alpha = 200;
		}

		if (alpha <= 127.f && flip <= 0)
			flip = 73;

		color.a() = alpha;

		if (g_hvh.m_left)
			render::triangle(vec2_t(x - size[0], y), vec2_t(x - size[1], y - size[2]), vec2_t(x - size[1], y + size[2]), color);

		if (g_hvh.m_right)
			render::triangle(vec2_t(x + size[0], y), vec2_t(x + size[1], y + size[2]), vec2_t(x + size[1], y - size[2]), color);

		if (g_hvh.m_back)
			render::triangle(vec2_t(x, y + size[0]), vec2_t(x - size[2], y + size[1]), vec2_t(x + size[2], y + size[1]), color);

		if (g_hvh.m_front)
			render::triangle(vec2_t(x, y - size[0]), vec2_t(x + size[2], y - size[1]), vec2_t(x - size[2], y - size[1]), color);
	}
	else {
		alpha = 0.f;
		flip = 400;
	}

	// compute hud size.
	// int size = ( int )std::round( ( g_cl.m_height / 17.5f ) * g_csgo.hud_scaling->GetFloat( ) );

	struct Indicator_t { Color color; std::string text; };
	std::vector< Indicator_t > indicators{ };

	// LC
	if (g_menu.main.visuals.indicators.get(1)) {
		if (g_cl.m_local->m_vecVelocity().length_2d() > 270.f || g_cl.m_lagcomp) {
			Indicator_t ind{ };
			ind.color = g_cl.m_lagcomp ? 0xff15c27b : 0xff0000ff;
			ind.text = XOR("LC");

			indicators.push_back(ind);
		}
	}

	// LBY
	if (g_menu.main.visuals.indicators.get(0) && g_cl.m_state) {
		// get the absolute change between current lby and animated angle.
		float change = std::abs(math::NormalizedAngle(g_cl.m_state->m_body_yaw - g_cl.m_state->m_angle.y));

		Indicator_t ind{ };
		ind.color = change > 35.f ? 0xff15c27b : 0xff0000ff;
		ind.text = XOR("LBY");
		indicators.push_back(ind);
	}

	// PING
	if (g_menu.main.visuals.indicators.get(2) && (g_aimbot.m_fake_latency || g_menu.main.misc.secondary_fake_latency.get())) {
		Indicator_t ind{ };
		ind.color = g_aimbot.m_fake_latency ? 0xffffffff : 0xff15c27b;
		ind.text = XOR("PING");

		indicators.push_back(ind);
	}

	// DMG
	if (g_menu.main.visuals.indicators.get(3) && g_aimbot.m_damage_override) {
		Indicator_t ind{ };
		ind.color = 0xffffffff;
		ind.text = XOR("DMG");

		indicators.push_back(ind);
	}

	if (indicators.empty())
		return;

	// iterate and draw indicators.
	for (size_t i{ }; i < indicators.size(); ++i) {
		auto& indicator = indicators[i];

		render::indicator.string(20, g_cl.m_height - 80 - (30 * i), indicator.color, indicator.text);
	}
}

void Visuals::SpreadCrosshair() {
	// dont do if dead.
	if (!g_cl.m_processing)
		return;

	if (!g_menu.main.visuals.spread_xhair.get())
		return;

	// get active weapon.
	Weapon* weapon = g_cl.m_local->GetActiveWeapon();
	if (!weapon)
		return;

	WeaponInfo* data = weapon->GetWpnData();
	if (!data)
		return;

	// do not do this on: bomb, knife and nades.
	CSWeaponType type = data->m_weapon_type;
	if (type == WEAPONTYPE_KNIFE || type == WEAPONTYPE_C4 || type == WEAPONTYPE_EQUIPMENT)
		return;

	// calc radius.
	float radius = ((weapon->GetInaccuracy() + weapon->GetSpread()) * 320.f) / (std::tan(math::deg_to_rad(g_cl.m_local->GetFOV()) * 0.5f) + FLT_EPSILON);

	// scale by screen size.
	radius *= g_cl.m_height * (1.f / 480.f);

	// get color.
	Color col = g_menu.main.visuals.spread_xhair_col.get();

	// modify alpha channel.
	col.a() = 200 * (g_menu.main.visuals.spread_xhair_blend.get() / 100.f);

	int segements = std::max(16, (int)std::round(radius * 0.75f));
	render::circle(g_cl.m_width / 2, g_cl.m_height / 2, radius, segements, col);
}

void Visuals::PenetrationCrosshair() {
	if (!g_menu.main.visuals.pen_crosshair.get() || !g_cl.m_processing)
		return;

	int x = g_cl.m_width / 2;
	int y = g_cl.m_height / 2;

	Color final_color;

	if (g_cl.m_pen_data.m_visible)
		final_color = colors::transparent_yellow;

	else if (g_cl.m_pen_data.m_damage >= 1)
		final_color = colors::transparent_green;

	else
		final_color = colors::transparent_red;

	if (g_menu.main.misc.agartha.get()) {
		std::vector<std::pair<vec2_t, vec2_t>> points;
		for (int i = 0; i < 4; i++) {
			switch (i) {
			case 0:
				points.emplace_back(vec2_t(-1, 0), vec2_t(0, -1));
				break;
			case 1:
				points.emplace_back(vec2_t(0, -1), vec2_t(1, 0));
				break;
			case 2:
				points.emplace_back(vec2_t(1, 0), vec2_t(0, 1));
				break;
			case 3:
				points.emplace_back(vec2_t(0, 1), vec2_t(-1, 0));
				break;
			}
		}

		auto rotate_point = [&](vec2_t& vec, float t) -> void {
			double angle = t * math::pi_2; // if t is [0..1] fraction of a full circle
			vec.x = vec.x * cosf(angle) - vec.y * sinf(angle);
			vec.y = vec.x * sinf(angle) + vec.y * cosf(angle);
			};

		for (std::pair<vec2_t, vec2_t> point : points) {
			rotate_point(point.first, g_csgo.m_globals->m_curtime);
			rotate_point(point.second, g_csgo.m_globals->m_curtime);

			vec2_t first = vec2_t(x, y) + point.first * 6.f;
			vec2_t second = first + point.second * 6.f;

			render::line(vec2_t(x, y), first, final_color);
			render::line(first, second, final_color);
		}
		return;
	}

	// draw small square in center of screen.
	render::rect(x - 1, y - 1, 3, 3, final_color);
}

void Visuals::draw(Entity* ent) {
	if (ent->IsPlayer()) {
		Player* player = ent->as< Player* >();

		// dont draw dead players.
		if (!player->alive())
			return;

		if (player->m_bIsLocalPlayer())
			return;

		// draw player esp.
		DrawPlayer(player);
	}

	else if (g_menu.main.visuals.items.get() && ent->IsBaseCombatWeapon() && !ent->dormant())
		DrawItem(ent->as< Weapon* >());

	else if (g_menu.main.visuals.proj.get())
		DrawProjectile(ent->as< Weapon* >());
}

void Visuals::DrawProjectile(Weapon* ent) {
	vec2_t screen;
	vec3_t origin = ent->GetAbsOrigin();
	if (!render::WorldToScreen(origin, screen))
		return;

	Color col = g_menu.main.visuals.proj_color.get();
	col.a() = 0xb4;

	// draw decoy.
	if (ent->is(HASH("CDecoyProjectile")))
		render::esp_small.string(screen.x, screen.y, col, XOR("DECOY"), render::ALIGN_CENTER);

	// draw molotov.
	else if (ent->is(HASH("CMolotovProjectile")))
		render::esp_small.string(screen.x, screen.y, col, XOR("MOLLY"), render::ALIGN_CENTER);

	else if (ent->is(HASH("CBaseCSGrenadeProjectile"))) {
		const model_t* model = ent->GetModel();

		if (model) {
			// grab modelname.
			std::string name{ ent->GetModel()->m_name };

			if (name.find(XOR("flashbang")) != std::string::npos)
				render::esp_small.string(screen.x, screen.y, col, XOR("FLASH"), render::ALIGN_CENTER);

			else if (name.find(XOR("fraggrenade")) != std::string::npos) {

				// grenade range.
				if (g_menu.main.visuals.proj_range.get(0))
					render::sphere(origin, 350.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get());

				render::esp_small.string(screen.x, screen.y, col, XOR("FRAG"), render::ALIGN_CENTER);
			}
		}
	}

	// find classes.
	else if (ent->is(HASH("CInferno"))) {
		// fire range.
		if (g_menu.main.visuals.proj_range.get(1))
			render::sphere(origin, 150.f, 5.f, 1.f, g_menu.main.visuals.proj_range_color.get());

		render::esp_small.string(screen.x, screen.y, col, XOR("FIRE"), render::ALIGN_CENTER);
	}

	else if (ent->is(HASH("CSmokeGrenadeProjectile")))
		render::esp_small.string(screen.x, screen.y, col, XOR("SMOKE"), render::ALIGN_CENTER);
}

void Visuals::DrawItem(Weapon* item) {
	// we only want to draw shit without owner.
	Entity* owner = g_csgo.m_entlist->GetClientEntityFromHandle(item->m_hOwnerEntity());
	if (owner)
		return;

	// is the fucker even on the screen?
	vec2_t screen;
	vec3_t origin = item->GetAbsOrigin();
	if (!render::WorldToScreen(origin, screen))
		return;

	WeaponInfo* data = item->GetWpnData();
	if (!data)
		return;

	Color col = g_menu.main.visuals.item_color.get();
	col.a() = 0xb4;

	// render bomb in green.
	if (item->is(HASH("CC4")))
		render::esp_small.string(screen.x, screen.y, { 150, 200, 60, 0xb4 }, XOR("BOMB"), render::ALIGN_CENTER);

	// if not bomb
	// normal item, get its name.
	else {
		std::string name{ item->GetLocalizedName() };

		// smallfonts needs uppercase.
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);

		render::esp_small.string(screen.x, screen.y, col, name, render::ALIGN_CENTER);
	}

	if (!g_menu.main.visuals.ammo.get())
		return;

	// nades do not have ammo.
	if (data->m_weapon_type == WEAPONTYPE_EQUIPMENT || data->m_weapon_type == WEAPONTYPE_KNIFE)
		return;

	if (item->m_iItemDefinitionIndex() == 0 || item->m_iItemDefinitionIndex() == C4)
		return;

	std::string ammo = tfm::format(XOR("(%i/%i)"), item->m_iClip1(), item->m_iPrimaryReserveAmmoCount());
	render::esp_small.string(screen.x, screen.y - render::esp_small.m_size.m_height - 1, col, ammo, render::ALIGN_CENTER);
}

void Visuals::OffScreen(Player* player, float alpha) {
	vec3_t view_origin, target_pos, delta;
	vec2_t screen_pos, offscreen_pos;
	float  leeway_x, leeway_y, radius, offscreen_rotation;
	bool   is_on_screen;
	Vertex verts[3];
	Color  color;

	static auto get_offscreen_data = [](const vec3_t& delta, float radius, vec2_t& out_offscreen_pos, float& out_rotation) {
		ang_t  view_angles(g_csgo.m_view_render->m_view.m_angles);
		vec3_t fwd, right, up(0.f, 0.f, 1.f);
		float  front, side, yaw_rad, sa, ca;

		// get viewport angles forward directional vector.
		math::AngleVectors(view_angles, &fwd);

		// convert viewangles forward directional vector to a unit vector.
		fwd.z = 0.f;
		fwd.normalize();

		// calculate front / side positions.
		right = up.cross(fwd);
		front = delta.dot(fwd);
		side = delta.dot(right);

		// setup offscreen position.
		out_offscreen_pos.x = radius * -side;
		out_offscreen_pos.y = radius * -front;

		// get the rotation ( yaw, 0 - 360 ).
		out_rotation = math::rad_to_deg(std::atan2(out_offscreen_pos.x, out_offscreen_pos.y) + math::pi);

		// get needed sine / cosine values.
		yaw_rad = math::deg_to_rad(-out_rotation);
		sa = std::sin(yaw_rad);
		ca = std::cos(yaw_rad);

		// rotate offscreen position around.
		out_offscreen_pos.x = (int)((g_cl.m_width / 2.f) + (radius * sa));
		out_offscreen_pos.y = (int)((g_cl.m_height / 2.f) - (radius * ca));
		};

	if (!g_menu.main.players.offscreen.get())
		return;

	if (!g_cl.m_processing || !g_cl.m_local->enemy(player))
		return;

	// get the player's center screen position.
	target_pos = player->WorldSpaceCenter();
	is_on_screen = render::WorldToScreen(target_pos, screen_pos);

	// give some extra room for screen position to be off screen.
	leeway_x = g_cl.m_width / 18.f;
	leeway_y = g_cl.m_height / 18.f;

	// origin is not on the screen at all, get offscreen position data and start rendering.
	if (!is_on_screen
		|| screen_pos.x < -leeway_x
		|| screen_pos.x >(g_cl.m_width + leeway_x)
		|| screen_pos.y < -leeway_y
		|| screen_pos.y >(g_cl.m_height + leeway_y)) {

		// get viewport origin.
		view_origin = g_csgo.m_view_render->m_view.m_origin;

		// get direction to target.
		delta = (target_pos - view_origin).normalized();

		// note - dex; this is the 'YRES' macro from the source sdk.
		radius = 200.f * (g_cl.m_height / 480.f);

		// get the data we need for rendering.
		get_offscreen_data(delta, radius, offscreen_pos, offscreen_rotation);

		// bring rotation back into range... before rotating verts, sine and cosine needs this value inverted.
		// note - dex; reference: 
		// https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/src_main/game/client/tf/tf_hud_damageindicator.cpp#L182
		offscreen_rotation = -offscreen_rotation;

		// setup vertices for the triangle.
		verts[0] = { offscreen_pos.x, offscreen_pos.y };        // 0,  0
		verts[1] = { offscreen_pos.x - 12.f, offscreen_pos.y + 20.f }; // -1, 1
		verts[2] = { offscreen_pos.x + 12.f, offscreen_pos.y + 20.f }; // 1,  1

		// rotate all vertices to point towards our target.
		verts[0] = render::RotateVertex(offscreen_pos, verts[0], offscreen_rotation);
		verts[1] = render::RotateVertex(offscreen_pos, verts[1], offscreen_rotation);
		verts[2] = render::RotateVertex(offscreen_pos, verts[2], offscreen_rotation);

		// render!
		color = g_menu.main.players.offscreen_color.get(); // damage_data.m_color;
		color.a() *= (alpha >= 1.f) ? alpha : alpha / 2.f;

		render::triangle(verts[0].m_pos, verts[2].m_pos, verts[1].m_pos, color);
	}
}

void Visuals::DrawPlayer(Player* player) {
	constexpr float MAX_DORMANT_TIME = 10.f;
	constexpr float DORMANT_FADE_TIME = MAX_DORMANT_TIME / 2.f;

	Rect		  box;
	player_info_t info;
	Color		  color;

	// get player index.
	int index = player->index();

	// get reference to array variable.
	float& opacity = m_opacities[index - 1];
	bool& draw = m_draw[index - 1];

	// opacity should reach 1 in 300 milliseconds.
	constexpr int frequency = 1.f / 0.3f;

	// the increment / decrement per frame.
	float step = frequency * g_csgo.m_globals->m_frametime;

	// is player enemy.
	bool enemy = player->enemy(g_cl.m_local);
	bool dormant = player->dormant();

	if (g_menu.main.visuals.enemy_radar.get() && enemy && !dormant)
		player->m_bSpotted() = true;

	// we can draw this player again.
	if (!dormant)
		draw = true;

	if (!draw)
		return;

	// if non-dormant	-> increment
	// if dormant		-> decrement
	dormant ? opacity -= step : opacity += step;

	// is dormant esp enabled for this player.
	bool dormant_esp = enemy && g_menu.main.players.dormant.get();

	// clamp the opacity.
	math::clamp(opacity, 0.f, 1.f);
	if (!opacity && !dormant_esp)
		return;

	// stay for x seconds max.
	float dt = g_csgo.m_globals->m_curtime - player->m_flSimulationTime();
	if (dormant && dt > MAX_DORMANT_TIME)
		return;

	// calculate alpha channels.
	int alpha = (int)(255.f * opacity);
	int low_alpha = (int)(179.f * opacity);

	// get color based on enemy or not.
	color = enemy ? g_menu.main.players.box_enemy.get() : g_menu.main.players.box_friendly.get();

	if (dormant && dormant_esp) {
		alpha = 112;
		low_alpha = 80;

		// fade.
		if (dt > DORMANT_FADE_TIME) {
			// for how long have we been fading?
			float faded = (dt - DORMANT_FADE_TIME);
			float scale = 1.f - (faded / DORMANT_FADE_TIME);

			alpha *= scale;
			low_alpha *= scale;
		}

		// override color.
		color = { 112, 112, 112 };
	}

	// override alpha.
	color.a() *= alpha / 255.f;

	// get player info.
	if (!g_csgo.m_engine->GetPlayerInfo(index, &info))
		return;

	// run offscreen ESP.
	OffScreen(player, opacity);

	// attempt to get player box.
	if (!GetPlayerBoxRect(player, box)) {
		// OffScreen( player );
		return;
	}

	AimPlayer* data = &g_aimbot.m_players[index - 1];
	if (!data)
		return;

	bool bone_esp = (enemy && g_menu.main.players.skeleton.get(0)) || (!enemy && g_menu.main.players.skeleton.get(1));
	if (bone_esp)
		DrawSkeleton(player, opacity);

	// is box esp enabled for this player.
	bool box_esp = (enemy && g_menu.main.players.box.get(0)) || (!enemy && g_menu.main.players.box.get(1));

	// render box if specified.
	if (box_esp)
		render::rect_outlined(box.x, box.y, box.w, box.h, color, { 10, 10, 10, low_alpha });

	// is name esp enabled for this player.
	bool name_esp = (enemy && g_menu.main.players.name.get(0)) || (!enemy && g_menu.main.players.name.get(1));

	// draw name.
	if (name_esp) {
		// fix retards with their namechange meme 
		// the point of this is overflowing unicode compares with hardcoded buffers, good hvh strat
		std::string name{ std::string(info.m_name).substr(0, 24) };

		Color clr = g_menu.main.players.name_color.get();
		// override alpha.
		clr.a() *= low_alpha / 255.f;

		render::esp.string(box.x + box.w / 2, box.y - render::esp.m_size.m_height, clr, name, render::ALIGN_CENTER);
	}

	// is health esp enabled for this player.
	bool health_esp = (enemy && g_menu.main.players.health.get(0)) || (!enemy && g_menu.main.players.health.get(1));

	if (health_esp) {
		int y = box.y + 1;
		int h = box.h - 2;

		// retarded servers that go above 100 hp..
		int hp = std::min(100, player->m_iHealth());

		int fill = (int)std::round(hp * h / 100.f);

		Color start_color = { 138, 198, 104 };
		Color end_color = { 255, 0, 0 };

		float hp_fraction = hp / 100.f;

		int r = end_color.r() + (start_color.r() - end_color.r()) * hp_fraction;
		int g = end_color.g() + (start_color.g() - end_color.g()) * hp_fraction;
		int b = end_color.b() + (start_color.b() - end_color.b()) * hp_fraction;

		render::rect_filled(box.x - 6, y - 1, 4, h + 2, { 10, 10, 10, low_alpha });
		render::rect(box.x - 5, y + h - fill, 2, fill, { r, g, b, alpha });

		if (hp < 100)
			render::esp_small.string(box.x - 5, y + (h - fill) - 5, { 255, 255, 255, low_alpha }, std::to_string(hp), render::ALIGN_CENTER);
	}

	// draw flags.
	{
		std::vector< std::pair< std::string, Color > > flags;

		auto items = enemy ? g_menu.main.players.flags_enemy.GetActiveIndices() : g_menu.main.players.flags_friendly.GetActiveIndices();

		for (auto it = items.begin(); it != items.end(); ++it) {

			// money.
			if (*it == 0)
				flags.push_back({ tfm::format(XOR("$%i"), player->m_iAccount()), { 150, 200, 60, low_alpha } });

			// armor.
			if (*it == 1) {
				// helmet and kevlar.
				if (player->m_bHasHelmet() && player->m_ArmorValue() > 0)
					flags.push_back({ XOR("HK"), { 255, 255, 255, low_alpha } });

				// only helmet.
				else if (player->m_bHasHelmet())
					flags.push_back({ XOR("H"), { 255, 255, 255, low_alpha } });

				// only kevlar.
				else if (player->m_ArmorValue() > 0)
					flags.push_back({ XOR("K"), { 255, 255, 255, low_alpha } });
			}

			// scoped.
			if (*it == 2 && player->m_bIsScoped())
				flags.push_back({ XOR("ZOOM"), { 60, 180, 225, low_alpha } });

			// flashed.
			if (*it == 3 && player->m_flFlashBangTime() > 0.f)
				flags.push_back({ XOR("FLASHED"), { 255, 255, 0, low_alpha } });

			// reload.
			if (*it == 4) {
				// get ptr to layer 1.
				C_AnimationLayer* layer1 = &player->m_AnimOverlay()[1];

				// check if reload animation is going on.
				if (layer1->m_weight != 0.f && player->GetSequenceActivity(layer1->m_sequence) == 967 /* ACT_CSGO_RELOAD */)
					flags.push_back({ XOR("RELOAD"), { 60, 180, 225, low_alpha } });
			}

			// bomb.
			if (*it == 5 && player->HasC4())
				flags.push_back({ XOR("BOMB"), { 255, 0, 0, low_alpha } });

			// distortion.
			if (*it == 6 && enemy && !data->m_records.empty() && data->m_records[0].m_resolver_mode == RESOLVE_DISTORTION)
				flags.push_back({ XOR("DISTORTION"), { 255, 255, 255, low_alpha } });

			//resolver confidence
			if (*it == 7 && enemy && !data->m_records.empty()) {
				switch (data->m_records[0].ResolveChance()) {
				case RESOLVE_CONFIDENCE_LOW:
					flags.push_back({ XOR("LOW"), { 255, 0, 0, low_alpha } });
					break;
				case RESOLVE_CONFIDENCE_MEDIUM:
					flags.push_back({ XOR("MEDIUM"), { 243, 156, 18, low_alpha } });
					break;
				case RESOLVE_CONFIDENCE_HIGH:
					flags.push_back({ XOR("HIGH"), { 150, 200, 60, low_alpha } });
					break;
				case RESOLVE_CONFIDENCE_VERY_HIGH:
					flags.push_back({ XOR("VERY HIGH"), { 255, 66, 170, low_alpha } });
					break;
				case RESOLVE_CONFIDENCE_OVERRIDE:
					flags.push_back({ XOR("OVERRIDE"), { 255, 255, 255, low_alpha } });
					break;
				}
			}
		}


		Packet* packet = g_networking.GetUserData(index);
		if (packet && packet->IsRax())
			flags.push_back({
			packet->m_hash == HASH_DEBUG ? XOR("RAXDEV") : XOR("RAX"),
			{ 255, 255, 255, low_alpha }
				});

		// iterate flags.
		for (size_t i{ }; i < flags.size(); ++i) {
			// get flag job (pair).
			const auto& f = flags[i];

			int offset = i * (render::esp_small.m_size.m_height - 1);

			// draw flag.
			render::esp_small.string(box.x + box.w + 2, box.y + offset, f.second, f.first);
		}
	}

	// draw bottom bars.
	{
		int  offset{ 0 };

		// draw weapon.
		if ((enemy && g_menu.main.players.weapon.get(0)) || (!enemy && g_menu.main.players.weapon.get(1))) {
			Weapon* weapon = player->GetActiveWeapon();
			if (weapon) {
				WeaponInfo* data = weapon->GetWpnData();
				if (data) {
					int bar;
					float scale;

					// the maxclip1 in the weaponinfo
					int max = data->m_max_clip1;
					int current = weapon->m_iClip1();

					C_AnimationLayer* layer1 = &player->m_AnimOverlay()[1];

					// set reload state.
					bool reload = (layer1->m_weight != 0.f) && (player->GetSequenceActivity(layer1->m_sequence) == 967);

					// ammo bar.
					if (max != -1 && g_menu.main.players.ammo.get()) {
						// check for reload.
						if (reload)
							scale = layer1->m_cycle;

						// not reloading.
						// make the division of 2 ints produce a float instead of another int.
						else
							scale = (float)current / max;

						// relative to bar.
						bar = (int)std::round((box.w - 2) * scale);

						// draw.
						render::rect_filled(box.x, box.y + box.h + 2 + offset, box.w, 4, { 10, 10, 10, low_alpha });

						Color clr = g_menu.main.players.ammo_color.get();
						clr.a() *= alpha / 255.f;
						render::rect(box.x + 1, box.y + box.h + 3 + offset, bar, 2, clr);

						// less then a 5th of the bullets left.
						if (current <= (int)std::round(max / 5) && !reload)
							render::esp_small.string(box.x + bar, box.y + box.h + offset, { 255, 255, 255, low_alpha }, std::to_string(current), render::ALIGN_CENTER);

						offset += 6;
					}

					// text.
					if (g_menu.main.players.weapon_mode.get() == 0) {
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName() };

						// smallfonts needs upper case.
						std::transform(name.begin(), name.end(), name.begin(), ::toupper);

						render::esp_small.string(box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, name, render::ALIGN_CENTER);
					}

					// icons.
					else if (g_menu.main.players.weapon_mode.get() == 1) {
						// icons are super fat..
						// move them back up.
						offset -= 5;

						std::string icon = tfm::format(XOR("%c"), m_weapon_icons[weapon->m_iItemDefinitionIndex()]);
						render::cs.string(box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, icon, render::ALIGN_CENTER);
					}
					//text + icons
					else if (g_menu.main.players.weapon_mode.get() == 2) {
						// move them back up.
						offset -= 7;

						std::string icon = tfm::format(XOR("%c"), m_weapon_icons[weapon->m_iItemDefinitionIndex()]);
						render::cs.string(box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, icon, render::ALIGN_CENTER);
						// construct std::string instance of localized weapon name.
						std::string name{ weapon->GetLocalizedName() };

						// smallfonts needs upper case.
						std::transform(name.begin(), name.end(), name.begin(), ::toupper);

						render::esp_small.string(box.x + box.w / 2, box.y + box.h + offset, { 255, 255, 255, low_alpha }, name, render::ALIGN_CENTER);
					}
				}
			}
		}
	}
}

void Visuals::DrawPlantedC4() {
	bool        mode_2d, mode_3d, is_visible;
	float       explode_time_diff, dist, range_damage;
	vec3_t      dst, to_target;
	int         final_damage;
	std::string time_str, damage_str;
	Color       damage_color;
	vec2_t      screen_pos;

	static auto scale_damage = [](float damage, int armor_value) {
		float new_damage, armor;

		if (armor_value > 0) {
			new_damage = damage * 0.5f;
			armor = (damage - new_damage) * 0.5f;

			if (armor > (float)armor_value) {
				armor = (float)armor_value * 2.f;
				new_damage = damage - armor;
			}

			damage = new_damage;
		}

		return std::max(0, (int)std::floor(damage));
		};

	// store menu vars for later.
	mode_2d = g_menu.main.visuals.planted_c4.get(0);
	mode_3d = g_menu.main.visuals.planted_c4.get(1);
	if (!mode_2d && !mode_3d)
		return;

	// bomb not currently active, do nothing.
	if (!m_c4_planted)
		return;

	// calculate bomb damage.
	// references:
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L271
	//     https://github.com/VSES/SourceEngine2007/blob/43a5c90a5ada1e69ca044595383be67f40b33c61/se2007/game/shared/cstrike/weapon_c4.cpp#L437
	//     https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/shared/sdk/sdk_gamerules.cpp#L173
	{
		// get our distance to the bomb.
		// todo - dex; is dst right? might need to reverse CBasePlayer::BodyTarget...
		dst = g_cl.m_local->WorldSpaceCenter();
		to_target = m_planted_c4_explosion_origin - dst;
		dist = to_target.length();

		// calculate the bomb damage based on our distance to the C4's explosion.
		range_damage = m_planted_c4_damage * std::exp((dist * dist) / ((m_planted_c4_radius_scaled * -2.f) * m_planted_c4_radius_scaled));

		// now finally, scale the damage based on our armor (if we have any).
		final_damage = scale_damage(range_damage, g_cl.m_local->m_ArmorValue());
	}

	// m_flC4Blow is set to gpGlobals->curtime + m_flTimerLength inside CPlantedC4.
	explode_time_diff = m_planted_c4_explode_time - g_csgo.m_globals->m_curtime;

	// get formatted strings for bomb.
	time_str = tfm::format(XOR("%.2f"), explode_time_diff);
	damage_str = tfm::format(XOR("%i"), final_damage);

	// get damage color.
	damage_color = (final_damage < g_cl.m_local->m_iHealth()) ? colors::white : colors::red;

	// finally do all of our rendering.
	is_visible = render::WorldToScreen(m_planted_c4_explosion_origin, screen_pos);

	// 'on screen (2D)'.
	if (mode_2d) {
		// g_cl.m_height - 80 - ( 30 * i )
		// 80 - ( 30 * 2 ) = 20

		// render::menu_shade.string( 60, g_cl.m_height - 20 - ( render::hud_size.m_height / 2 ), 0xff0000ff, "", render::hud );

		if (explode_time_diff > 0.f) //if bomb no explod
			render::esp.string(2, 65, colors::white, time_str, render::ALIGN_LEFT);

		if (g_cl.m_local->alive()) //if alive and explosion would be fatal make the number red.
			render::esp.string(2, 65 + render::esp.m_size.m_height, damage_color, damage_str, render::ALIGN_LEFT);
	}

	// 'on bomb (3D)'.
	if (mode_3d && is_visible) {
		if (explode_time_diff > 0.f)
			render::esp_small.string(screen_pos.x, screen_pos.y, colors::white, time_str, render::ALIGN_CENTER);

		// only render damage string if we're alive.
		if (g_cl.m_local->alive())
			render::esp_small.string(screen_pos.x, (int)screen_pos.y + render::esp_small.m_size.m_height, damage_color, damage_str, render::ALIGN_CENTER);
	}
}

bool Visuals::GetPlayerBoxRect(Player* player, Rect& box) {
	vec3_t min, max, out_vec;
	float left, bottom, right, top;
	matrix3x4_t& tran_frame = player->m_rgflCoordinateFrame();

	// get hitbox bounds.
	min = player->m_vecMins();
	max = player->m_vecMaxs();

	vec2_t screen_boxes[8];

	// transform mins and maxes to points. 
	vec3_t points[] =
	{
		{ min.x, min.y, min.z },
		{ min.x, max.y, min.z },
		{ max.x, max.y, min.z },
		{ max.x, min.y, min.z },
		{ max.x, max.y, max.z },
		{ min.x, max.y, max.z },
		{ min.x, min.y, max.z },
		{ max.x, min.y, max.z }
	};

	// transform points to 3-dimensional space.
	for (int i = 0; i <= 7; i++)
	{
		math::VectorTransform(points[i], tran_frame, out_vec);
		if (!render::WorldToScreen(out_vec, screen_boxes[i]))
			return false;
	}

	// generate an array to clamp later.
	vec2_t box_array[] = {
		screen_boxes[3],
		screen_boxes[5],
		screen_boxes[0],
		screen_boxes[4],
		screen_boxes[2],
		screen_boxes[1],
		screen_boxes[6],
		screen_boxes[7]
	};

	// state the position and size of the box.
	left = screen_boxes[3].x,
		bottom = screen_boxes[3].y,
		right = screen_boxes[3].x,
		top = screen_boxes[3].y;

	// clamp the box sizes.
	for (int i = 0; i <= 7; i++)
	{
		if (left > box_array[i].x)
			left = box_array[i].x;

		if (bottom > box_array[i].y)
			bottom = box_array[i].y;

		if (right < box_array[i].x)
			right = box_array[i].x;

		if (top < box_array[i].y)
			top = box_array[i].y;
	}

	// state the box bounds.
	box.x = left;
	box.y = bottom;
	box.w = right - left;
	box.h = top - bottom;

	return true;
}

void Visuals::DrawSkeleton(Player* player, float opacity) {
	const model_t* model;
	studiohdr_t* hdr;
	mstudiobone_t* bone;
	int           parent;
	BoneArray     matrix[128];
	vec3_t        bone_pos, parent_pos;
	vec2_t        bone_pos_screen, parent_pos_screen;

	// get player's model.
	model = player->GetModel();
	if (!model)
		return;

	// get studio model.
	hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return;

	// get bone matrix.
	if (!player->SetupBones(matrix, 128, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime))
		return;

	for (int i{ }; i < hdr->m_num_bones; ++i) {
		// get bone.
		bone = hdr->GetBone(i);
		if (!bone || !(bone->m_flags & BONE_USED_BY_HITBOX))
			continue;

		// get parent bone.
		parent = bone->m_parent;
		if (parent == -1)
			continue;

		// resolve main bone and parent bone positions.
		matrix->get_bone(bone_pos, i);
		matrix->get_bone(parent_pos, parent);

		Color clr = player->enemy(g_cl.m_local) ? g_menu.main.players.skeleton_enemy.get() : g_menu.main.players.skeleton_friendly.get();
		clr.a() *= opacity;

		// world to screen both the bone parent bone then draw.
		if (render::WorldToScreen(bone_pos, bone_pos_screen) && render::WorldToScreen(parent_pos, parent_pos_screen))
			render::line(bone_pos_screen.x, bone_pos_screen.y, parent_pos_screen.x, parent_pos_screen.y, clr);
	}
}

void Visuals::RenderGlow() {
	Color   color;
	Player* player;

	if (!g_cl.m_local)
		return;

	if (!g_csgo.m_glow->m_object_definitions.Count())
		return;

	for (int i{ }; i < g_csgo.m_glow->m_object_definitions.Count(); ++i) {
		GlowObjectDefinition_t* obj = &g_csgo.m_glow->m_object_definitions[i];

		// skip non-players.
		if (!obj->m_entity || !obj->m_entity->IsPlayer())
			continue;

		// get player ptr.
		player = obj->m_entity->as< Player* >();

		if (player->m_bIsLocalPlayer())
			continue;

		// get reference to array variable.
		float& opacity = m_opacities[player->index() - 1];

		bool enemy = player->enemy(g_cl.m_local);

		if (enemy && !g_menu.main.players.glow.get(0))
			continue;

		if (!enemy && !g_menu.main.players.glow.get(1))
			continue;

		// enemy color
		if (enemy)
			color = g_menu.main.players.glow_enemy.get();

		// friendly color
		else
			color = g_menu.main.players.glow_friendly.get();

		obj->m_render_occluded = true;
		obj->m_render_unoccluded = false;
		obj->m_render_full_bloom = false;
		obj->m_color = { (float)color.r() / 255.f, (float)color.g() / 255.f, (float)color.b() / 255.f };
		obj->m_alpha = opacity * color.a() / 255.f;
	}
}

void Visuals::DrawHitboxMatrix(Player* player, BoneArray* bones, Color col, float time) {
	const model_t* model = player->GetModel();
	if (!model)
		return;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(player->m_nHitboxSet());
	if (!set)
		return;

	vec3_t             mins, maxs, origin;

	for (int i{ }; i < set->m_hitboxes; ++i)
	{
		mstudiobbox_t* bbox = set->GetHitbox(i);
		if (!bbox)
			continue;

		// bbox.
		if (bbox->m_radius <= 0.f)
		{
			// https://developer.valvesoftware.com/wiki/Rotation_Tutorial

			// convert rotation angle to a matrix.
			matrix3x4_t rot_matrix;
			g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

			// apply the rotation to the entity input space (local).
			matrix3x4_t matrix;
			math::ConcatTransforms(bones[bbox->m_bone], rot_matrix, matrix);

			// extract the compound rotation as an angle.
			ang_t bbox_angle;
			math::MatrixAngles(matrix, bbox_angle);

			// extract hitbox origin.
			vec3_t origin = matrix.GetOrigin();

			// draw box.
			g_csgo.m_debug_overlay->AddBoxOverlay(origin, bbox->m_mins, bbox->m_maxs, bbox_angle, col.r(), col.g(), col.b(), 0, time);
		}

		// capsule.
		else
		{
			// NOTE; the angle for capsules is always 0.f, 0.f, 0.f.

			// create a rotation matrix.
			matrix3x4_t matrix;
			g_csgo.AngleMatrix(bbox->m_angle, matrix);

			// apply the rotation matrix to the entity output space (world).
			math::ConcatTransforms(bones[bbox->m_bone], matrix, matrix);

			// get world positions from new matrix.
			math::VectorTransform(bbox->m_mins, matrix, mins);
			math::VectorTransform(bbox->m_maxs, matrix, maxs);

			g_csgo.m_debug_overlay->AddCapsuleOverlay(mins, maxs, bbox->m_radius, col.r(), col.g(), col.b(), col.a(), time, 0, true);
		}
	}
}

void Visuals::DebugAimbotPoints() {
	if (!g_cl.m_local || !g_cl.m_local->alive()) //if localplayer's variables don't exist or local player is not alive
		return;

	if (!g_menu.main.players.skeleton.get(2)) //only run if skeleton is enabled for enemy
		return;

	std::vector<std::pair<vec2_t, Color>> points;
	for (const TargetData& target : g_aimbot.m_targets) {
		for (const PointData& point : target.m_points) {
			vec2_t screen;
			if (!render::WorldToScreen(point.m_point, screen))
				continue;

			points.emplace_back(screen, g_menu.main.players.point_color.get());
		}
	}

	for (std::pair<vec2_t, Color>& point : points)
		render::rect(point.first.x - 1, point.first.y - 1, 2, 2, point.second); //for every point render rect
}