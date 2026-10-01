#pragma once

class Bones {
public:
	bool Setup( Player* target, const int& mask, const float& curtime, BoneArray* out );
};

extern Bones g_bones;