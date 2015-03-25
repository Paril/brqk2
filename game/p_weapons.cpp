#include "g_local.h"
#include "m_player.h"

void P_ProjectSourceC(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy(distance, _distance);
	_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}

void generic_fire_lead(edict_t *ent, const int &base_damage, const int &recoil, float spread, const int &pellets)
{
	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_HYPERBLASTER);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		spread /= 2;
	else
		spread += VectorLength(ent->velocity) / 4;

	for (int i = 0; i < 2; i++)
	{
		ent->client->kick_origin[i] = crandom() * (0.35f * (recoil / 20.0f));
		ent->client->kick_angles[i] = crandom() * (0.7f * (recoil / 20.0f));
	}

	vec3_t forward, right, offset, start;

	// get start / end positions
	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -(recoil / 20.0f), ent->client->kick_origin);

	VectorSet(offset, 0, 0, ent->viewheight - 8);
	P_ProjectSourceC(ent->client, ent->s.origin, offset, forward, right, start);

	if (pellets == 1)
		fire_bullet(ent, start, forward, base_damage, 0, spread, spread, MOD_MACHINEGUN);
	else
		fire_shotgun(ent, start, forward, base_damage, 0, spread, spread, pellets, MOD_MACHINEGUN);

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int)(random() + 0.25);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int)(random() + 0.25);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_InitMelee(edict_t *ent)
{
	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/knife_slash.wav"), 1, ATTN_NONE, 0);
	ent->client->ps.gunindex = gi.modelindex("models/weapons/v_melee.md2");
	ent->client->ps.gunframe = 0;
	ent->client->kick_angles[1] = -15;
	ent->client->kick_angles[0] = -10;
}

void do_melee(edict_t *self, vec3_t start, vec3_t aim, int reach, int damage, int kick, int quiet, int mod, int hit, int miss)
{
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	trace_t		tr;

	vectoangles2(aim, v);
	AngleVectors(v, forward, right, up);
	VectorNormalize(forward);
	VectorMA(start, reach, forward, point);

	//see if the hit connects
	tr = gi.trace(start, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction == 1.0)
		return;

	if (tr.ent->takedamage == DAMAGE_YES || tr.ent->takedamage == DAMAGE_AIM)
	{
		T_Damage(tr.ent, self, self, vec3_origin, tr.ent->s.origin, vec3_origin, damage, kick / 2,
			DAMAGE_DESTROY_ARMOR | DAMAGE_NO_KNOCKBACK, mod);

		if (!quiet)
			gi.sound(self, CHAN_WEAPON, hit, 1, ATTN_NORM, 0);
	}
	else
	{
		if (!quiet)
			gi.sound(self, CHAN_WEAPON, miss, 1, ATTN_NORM, 0);

		VectorScale(tr.plane.normal, 256, point);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_GUNSHOT);
		gi.WritePosition(tr.endpos);
		gi.WriteDir(point);
		gi.multicast(tr.endpos, MULTICAST_PVS);
	}
}

bool Weapon_PerformMelee(edict_t *ent)
{
	switch (++ent->client->ps.gunframe)
	{
	case 1:
		ent->client->kick_angles[1] = -15 / 2;
		ent->client->kick_angles[0] = -10 / 2;
		break;
	case 2:
		ent->client->kick_angles[1] = -15 / 3;
		ent->client->kick_angles[0] = -10 / 3;

		{
			vec3_t start, forward, right, up, offset;

			AngleVectors(ent->client->v_angle, forward, right, up);

			// set start point
			VectorSet(offset, 0, 8, ent->viewheight - 4);
			P_ProjectSourceC(ent->client, ent->s.origin, offset, forward, right, start);

			do_melee(ent, start, forward, 52, 52, 0, 0, MOD_CHAINFIST, gi.soundindex("weapons/knife_hit.wav"), gi.soundindex("weapons/knife_bounce.wav"));
		}
		break;
	case 3:
		ent->client->kick_angles[1] = -15 / 4;
		ent->client->kick_angles[0] = -10 / 4;
		break;
	case 4:
		ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		return true;
	}

	return false;
}

void Weapon_Pistol(edict_t *ent)
{
	switch (ent->client->weaponstate)
	{
	case WEAPON_ACTIVATING:
		if (ent->client->ps.gunframe == 0 && !ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
			ent->client->ps.gunframe = 69;

		if (ent->client->ps.gunframe == 3)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 13;
		}
		else if (ent->client->ps.gunframe == 73)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 65;
		}
		else
			ent->client->ps.gunframe++;

		break;
	case WEAPON_READY:
		if (ent->client->newweapon)
		{
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index]) ? 38 : 66;
			break;
		}
		else if (ent->client->latched_buttons & BUTTON_MELEE)
		{
			Weapon_InitMelee(ent);
			ent->client->weaponstate = WEAPON_MELEE;
			break;
		}
		else if (ent->client->latched_buttons & BUTTON_ATTACK)
		{
			if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
			{
				ent->client->weaponstate = WEAPON_FIRING;

				if (--ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
					ent->client->ps.gunframe = 11;
				else
					ent->client->ps.gunframe = 63;

				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/elite_fire.wav"), 1, ATTN_NONE, 0);
				generic_fire_lead(ent, 15, 12, 150, 1);
				break;
			}
			else if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 41;
				break;
			}
		}
		else if (ent->client->latched_buttons & BUTTON_RELOAD)
		{
			if (ent->client->pers.inventory[ent->client->ammo_index] && (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] < clip_ammo_counts[ent->client->pers.weapon->weap_index]))
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 41;
				break;
			}
		}

		++ent->client->ps.gunframe;
			
		if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] && ent->client->ps.gunframe > 36)
			ent->client->ps.gunframe = 13;
		else if (ent->client->ps.gunframe >= 65)
			ent->client->ps.gunframe = 65;
		break;
	case WEAPON_FIRING:
		if (ent->client->ps.gunframe == 11 || ent->client->ps.gunframe == 63)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe++;
		}
		else
			ent->client->ps.gunframe++;
		break;
	case WEAPON_DROPPING:
		if (ent->client->ps.gunframe == 40 || ent->client->ps.gunframe == 68)
			ChangeWeapon(ent);
		else
			++ent->client->ps.gunframe;
		break;
	case WEAPON_RELOADING:
		if (++ent->client->ps.gunframe == 61)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 13;

			int add = clip_ammo_counts[ent->client->pers.weapon->weap_index] - ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index];

			if (ent->client->pers.inventory[ent->client->ammo_index] < add)
				add = ent->client->pers.inventory[ent->client->ammo_index];

			ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] += add;
			ent->client->pers.inventory[ent->client->ammo_index] -= add;
		}
		else if (ent->client->ps.gunframe == 46)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/usp_clipout.wav"), 1, ATTN_NONE, 0);
		else if (ent->client->ps.gunframe == 53)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/usp_clipin.wav"), 1, ATTN_NONE, 0);
		break;
	case WEAPON_MELEE:
		if (Weapon_PerformMelee(ent))
		{
			ent->client->ps.gunframe = 0;
			ent->client->weaponstate = WEAPON_ACTIVATING;
		}
		break;
	}
}

void Weapon_Shotty(edict_t *ent)
{
	switch (ent->client->weaponstate)
	{
	case WEAPON_ACTIVATING:
		if (ent->client->ps.gunframe == 7)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 16;
		}
		else
			ent->client->ps.gunframe++;

		break;
	case WEAPON_READY:
		if (ent->client->newweapon)
		{
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 36;
			break;
		}
		else if (ent->client->latched_buttons & BUTTON_MELEE)
		{
			Weapon_InitMelee(ent);
			ent->client->weaponstate = WEAPON_MELEE;
			break;
		}
		else if (ent->client->buttons & BUTTON_ATTACK)
		{
			if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
			{
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->ps.gunframe = 9;

				--ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index];
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m3_fire.wav"), 1, ATTN_NONE, 0);
				generic_fire_lead(ent, 5, 50, 250, 9);
				break;
			}
			else if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 42;
				break;
			}
		}
		else if (ent->client->latched_buttons & BUTTON_RELOAD)
		{
			if (ent->client->pers.inventory[ent->client->ammo_index] && (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] < clip_ammo_counts[ent->client->pers.weapon->weap_index]))
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 42;
				break;
			}
		}

		++ent->client->ps.gunframe;

		if (ent->client->ps.gunframe > 35)
			ent->client->ps.gunframe = 16;
		break;
	case WEAPON_FIRING:
		if (ent->client->ps.gunframe == 15)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe++;
		}
		else
			ent->client->ps.gunframe++;
		break;
	case WEAPON_DROPPING:
		if (ent->client->ps.gunframe == 41)
			ChangeWeapon(ent);
		else
			++ent->client->ps.gunframe;
		break;
	case WEAPON_RELOADING:
		if (ent->client->ps.gunframe == 47)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m3_shell.wav"), 1, ATTN_NONE, 0);
			ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index]++;
			ent->client->pers.inventory[ent->client->ammo_index]--;
		}
		else if (ent->client->ps.gunframe == 49)
		{
			if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] < clip_ammo_counts[ent->client->pers.weapon->weap_index] &&
				ent->client->pers.inventory[ent->client->ammo_index] && !(ent->client->buttons & BUTTON_ATTACK))
			{
				ent->client->ps.gunframe = 46;
				break;
			}
		}
		else if (ent->client->ps.gunframe == 52)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 16;
			break;
		}

		ent->client->ps.gunframe++;
		break;
	case WEAPON_MELEE:
		if (Weapon_PerformMelee(ent))
		{
			ent->client->ps.gunframe = 0;
			ent->client->weaponstate = WEAPON_ACTIVATING;
		}
		break;
	}
}

void Weapon_Mp5(edict_t *ent)
{
	switch (ent->client->weaponstate)
	{
	case WEAPON_ACTIVATING:
		if (ent->client->ps.gunframe == 3)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 6;
		}
		else
			ent->client->ps.gunframe++;

		break;
	case WEAPON_READY:
		if (ent->client->newweapon)
		{
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 41;
			break;
		}
		else if (ent->client->latched_buttons & BUTTON_MELEE)
		{
			Weapon_InitMelee(ent);
			ent->client->weaponstate = WEAPON_MELEE;
			break;
		}
		else if (ent->client->buttons & BUTTON_ATTACK)
		{
			if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
			{
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->ps.gunframe = 9;

				goto fire_gun;
				break;
			}
			else if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 45;
				break;
			}
		}
		else if (ent->client->latched_buttons & BUTTON_RELOAD)
		{
			if (ent->client->pers.inventory[ent->client->ammo_index] && (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] < clip_ammo_counts[ent->client->pers.weapon->weap_index]))
			{
				ent->client->weaponstate = WEAPON_RELOADING;
				ent->client->ps.gunframe = 45;
				break;
			}
		}

		++ent->client->ps.gunframe;

		if (ent->client->ps.gunframe > 40)
			ent->client->ps.gunframe = 6;
		break;
	case WEAPON_FIRING:
		if (!(ent->client->buttons & BUTTON_ATTACK))
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 6;
		}
		else
		{
			if (ent->client->ps.gunframe == 4)
				ent->client->ps.gunframe = 5;
			else
				ent->client->ps.gunframe = 4;

		fire_gun:
			if (ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index])
			{
				--ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index];
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5_fire.wav"), 1, ATTN_NONE, 0);
				generic_fire_lead(ent, 12, 10, 125, 1);
			}
			else
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = 6;
			}
		}
		break;
	case WEAPON_DROPPING:
		if (ent->client->ps.gunframe == 44)
			ChangeWeapon(ent);
		else
			++ent->client->ps.gunframe;
		break;
	case WEAPON_RELOADING:
		if (++ent->client->ps.gunframe == 62)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = 6;

			int add = clip_ammo_counts[ent->client->pers.weapon->weap_index] - ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index];

			if (ent->client->pers.inventory[ent->client->ammo_index] < add)
				add = ent->client->pers.inventory[ent->client->ammo_index];

			ent->client->pers.loaded_ammo[ent->client->pers.weapon->weap_index] += add;
			ent->client->pers.inventory[ent->client->ammo_index] -= add;
		}
		else if (ent->client->ps.gunframe == 48)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5_out.wav"), 1, ATTN_NONE, 0);
		else if (ent->client->ps.gunframe == 51)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5_in.wav"), 1, ATTN_NONE, 0);
		else if (ent->client->ps.gunframe == 57)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5_slide.wav"), 1, ATTN_NONE, 0);
		break;
	case WEAPON_MELEE:
		if (Weapon_PerformMelee(ent))
		{
			ent->client->ps.gunframe = 0;
			ent->client->weaponstate = WEAPON_ACTIVATING;
		}
		break;
	}
}