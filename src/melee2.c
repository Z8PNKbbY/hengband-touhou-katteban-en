/* File: melee2.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Monster spells and movement */

/*
* This file has several additions to it by Keldon Jones (keldon@umr.edu)
* to improve the general quality of the AI (version 0.1.1).
*/

#include "angband.h"

#define SPEAK_CHANCE 8
#define GRINDNOISE 20
#define CYBERNOISE 20

/*
 * Calculate the direction to the next enemy
 */
/*:::敵を探し、そのターゲットのいる方向をmmに格納する。ペットと友好的モンスターでしか使われていない*/
/*:::敵が見つかった場合TRUEを返す*/
static bool get_enemy_dir(int m_idx, int *mm)
{
	int i;
	int x = 0, y = 0;
	int t_idx;
	int start;
	int plus = 1;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	monster_type *t_ptr;

	//グリーンアイドモンスターはターゲット固定
	if(p_ptr->pclass == CLASS_PARSEE && m_ptr->r_idx == MON_GREEN_EYED)
	{
		int target_idx = p_ptr->magic_num1[0];
		if(!m_list[target_idx].r_idx)
		{
			msg_format(_("ERROR:グリーンアイドモンスターのターゲットIDXが無効",
                        "ERROR: Bad IDX for Green-Eyed Monster target"));
			return FALSE;
		}
		y = m_list[target_idx].fy;
		x = m_list[target_idx].fx;

	}
	//浄頗梨審判もターゲット固定
	else if(p_ptr->pclass == CLASS_EIKI && m_ptr->mflag & MFLAG_EPHEMERA)
	{
		monster_type *target_m_ptr = &m_list[p_ptr->magic_num1[0]];
		if(!target_m_ptr->r_idx)
		{
			msg_format(_("ERROR:浄頗梨審判のターゲットIDXが無効",
                        "ERROR: Bad IDX for Cleansed Crystal Judgement target"));
			return FALSE;
		}
		y = target_m_ptr->fy;
		x = target_m_ptr->fx;
	}
	//八尺様
	else if(m_ptr->r_idx == MON_HASSYAKUSAMA && hassyakusama_target_idx)
	{
		if(m_list[hassyakusama_target_idx].r_idx)
		{
			y = m_list[hassyakusama_target_idx].fy;
			x = m_list[hassyakusama_target_idx].fx;
		}
	}

	/*:::ターゲットが既に設定されている場合*/
	else if (riding_t_m_idx && player_bold(m_ptr->fy, m_ptr->fx))
	{
		y = m_list[riding_t_m_idx].fy;
		x = m_list[riding_t_m_idx].fx;
	}
	else if (is_pet(m_ptr) && pet_t_m_idx)
	{
		y = m_list[pet_t_m_idx].fy;
		x = m_list[pet_t_m_idx].fx;
	}
	else
	{
		if (p_ptr->inside_battle)
		{
			start = randint1(m_max-1)+m_max;
			if(randint0(2)) plus = -1;
		}
		else start = m_max + 1;

		/* Scan thru all monsters */
		/*:::全モンスタースキャン　なぜこんな妙なループにしてるのかよく解らん　闘技場対応らしいが*/
		for (i = start; ((i < start + m_max) && (i > start - m_max)); i+=plus)
		{
			int dummy = (i % m_max);

			if (!dummy) continue;

			t_idx = dummy;
			t_ptr = &m_list[t_idx];

			/* The monster itself isn't a target */
			if (t_ptr == m_ptr) continue;

			/* Paranoia -- Skip dead monsters */
			if (!t_ptr->r_idx) continue;

			if (is_pet(m_ptr))
			{
				/* Hack -- only fight away from player */
				if (p_ptr->pet_follow_distance < 0)
				{
					/* No fighting near player */
					if (t_ptr->cdis <= (0 - p_ptr->pet_follow_distance))
					{
						continue;
					}
				}
				/* Hack -- no fighting away from player */
				else if ((m_ptr->cdis < t_ptr->cdis) &&
							(t_ptr->cdis > p_ptr->pet_follow_distance))
				{
					continue;
				}

				if (r_ptr->aaf < t_ptr->cdis) continue;
			}

			/* Monster must be 'an enemy' */
			if (!are_enemies(m_ptr, t_ptr)) continue;

			/* Monster must be projectable if we can't pass through walls */
			if (((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || p_ptr->pass_wall)) ||
			    ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != p_ptr->riding)))
			{
				if (!in_disintegration_range(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;
			}
			else
			{
				if (!projectable(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;
			}

			/* OK -- we've got a target */
			y = t_ptr->fy;
			x = t_ptr->fx;

			break;
		}
		if (!x && !y) return FALSE;
	}

	/* Extract the direction */
	x -= m_ptr->fx;
	y -= m_ptr->fy;

	/* North */
	if ((y < 0) && (x == 0))
	{
		mm[0] = 8;
		mm[1] = 7;
		mm[2] = 9;
	}
	/* South */
	else if ((y > 0) && (x == 0))
	{
		mm[0] = 2;
		mm[1] = 1;
		mm[2] = 3;
	}
	/* East */
	else if ((x > 0) && (y == 0))
	{
		mm[0] = 6;
		mm[1] = 9;
		mm[2] = 3;
	}
	/* West */
	else if ((x < 0) && (y == 0))
	{
		mm[0] = 4;
		mm[1] = 7;
		mm[2] = 1;
	}
	/* North-West */
	else if ((y < 0) && (x < 0))
	{
		mm[0] = 7;
		mm[1] = 4;
		mm[2] = 8;
	}
	/* North-East */
	else if ((y < 0) && (x > 0))
	{
		mm[0] = 9;
		mm[1] = 6;
		mm[2] = 8;
	}
	/* South-West */
	else if ((y > 0) && (x < 0))
	{
		mm[0] = 1;
		mm[1] = 4;
		mm[2] = 2;
	}
	/* South-East */
	else if ((y > 0) && (x > 0))
	{
		mm[0] = 3;
		mm[1] = 6;
		mm[2] = 2;
	}

	/* Found a monster */
	return TRUE;
}


/*
 * Hack, based on mon_take_hit... perhaps all monster attacks on
 * other monsters should use this?
 */
//通常のモンスター→モンスターへのダメージ処理はproject()経由でproject_m()内で行われる。
//この関数はモンスターが爆発したときと鷲系の掴み投げにしか使われていないらしい。
void mon_take_hit_mon(int m_idx, int dam, bool *fear, cptr note, int who)
{
	monster_type	*m_ptr = &m_list[m_idx];

	monster_race	*r_ptr = &r_info[m_ptr->r_idx];

	char m_name[160];

	bool seen = is_seen(m_ptr);

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT);

	/* Extract monster name */
	monster_desc(m_name, m_ptr, 0);

	/* Redraw (later) if needed */
	if (m_ptr->ml)
	{
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	/* Wake it up */
	(void)set_monster_csleep(m_idx, 0);

	if (p_ptr->riding && (m_idx == p_ptr->riding)) disturb(1, 1);

	if(m_ptr->r_idx == MON_SEIJA && !one_in_(7) )
	{
		if (seen) msg_print(_("正邪は妙な布で攻撃を逸らした。",
                                "Seija uses some weird fabric to evade the attack."));
		return;
	}

	if (MON_INVULNER(m_ptr) && randint0(PENETRATE_INVULNERABILITY))
	{
		if (seen)
		{
#ifdef JP
msg_format("%^sはダメージを受けない。", m_name);
#else
			msg_format("%^s is unharmed.", m_name);
#endif

		}

		return;
	}

	if (r_ptr->flagsr & RFR_RES_ALL)
	{
		if(dam > 0)
		{
			dam /= 100;
			if((dam == 0) && one_in_(3)) dam = 1;
		}
		if (dam==0)
		{
			if (seen)
			{
#ifdef JP
msg_format("%^sはダメージを受けない。", m_name);
#else
				msg_format("%^s is unharmed.", m_name);
#endif

			}
			return;
		}
	}

	/* Hurt it */
	m_ptr->hp -= dam;

	/* It is dead now... or is it? */
	if (m_ptr->hp < 0)
	{
		/*:::EPHEMERAフラグ持ちはユニークでも＠以外の攻撃で倒れる。妖夢の分身用の処理*/
		if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
		    (r_ptr->flags7 & RF7_NAZGUL) ||
		    ((r_ptr->flags7 & RF7_VARIABLE) && (r_ptr->flags7 & RF7_UNIQUE2)) ) &&
			!(m_ptr->mflag & MFLAG_EPHEMERA) &&
		    !p_ptr->inside_battle)
		{
			m_ptr->hp = 1;
		}
		else
		{
			/* Make a sound */
			if (!monster_living(r_ptr))
			{
				sound(SOUND_N_KILL);
			}
			else
			{
				sound(SOUND_KILL);
			}

			if (known)
			{
				monster_desc(m_name, m_ptr, MD_TRUE_NAME);
				/* Unseen death by normal attack */
				if (!seen)
				{
					mon_fight = TRUE;
				}
				//v1.1.24
				else if(m_ptr->r_idx == MON_YOUR_BOAT || m_ptr->r_idx == MON_ANCIENT_SHIP)
				{
					msg_format(_("%^sは沈んだ。", "%^s sinks"), m_name);
				}
				/* Death by special attack */
				else if (note)
				{
#ifdef JP
msg_format("%^s%s", m_name, note);
#else
					msg_format("%^s%s", m_name, note);
#endif

				}
				/* Death by normal attack -- nonliving monster */
				else if (!monster_living(r_ptr))
				{
#ifdef JP
msg_format("%^sは倒れた。", m_name);
#else
					msg_format("%^s is defeated.", m_name);
#endif

				}
				/* Death by normal attack -- living monster */
				else
				{
#ifdef JP
				///msg131221
msg_format("%^sは倒された。", m_name);
#else
					msg_format("%^s is defeated.", m_name);
#endif

				}
			}

			monster_gain_exp(who, m_ptr->r_idx);

			//輝夜が倒されたときの敵を記録
			if(p_ptr->pclass == CLASS_EIRIN && m_ptr->r_idx == MON_MASTER_KAGUYA)
			{
				p_ptr->magic_num1[0] = m_list[m_idx].r_idx;
			}

			/* Generate treasure */
			monster_death(m_idx, FALSE);

			/* Delete the monster */
			delete_monster_idx(m_idx);

			/* Not afraid */
			(*fear) = FALSE;

			/* Monster is dead */
			return;
		}
	}

#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (MON_MONFEAR(m_ptr) && (dam > 0))
	{
		/* Cure fear */
		if (set_monster_monfear(m_idx, MON_MONFEAR(m_ptr) - randint1(dam / 4)))
		{
			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!MON_MONFEAR(m_ptr) && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		/* Percentage of fully healthy */
		int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		* Run (sometimes) if at 10% or less of max hit points,
		* or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (randint0(10) < percentage)) ||
			((dam >= m_ptr->hp) && (randint0(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			(void)set_monster_monfear(m_idx, (randint1(10) +
				(((dam >= m_ptr->hp) && (percentage > 7)) ?
				20 : ((11 - percentage) * 5))));
		}
	}

#endif /* ALLOW_FEAR */

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (who != m_idx))
	{
		if (is_pet(&m_list[who]) && !player_bold(m_ptr->target_y, m_ptr->target_x))
		{
			set_target(m_ptr, m_list[who].fy, m_list[who].fx);
		}
	}

	if (p_ptr->riding && (p_ptr->riding == m_idx) && (dam > 0))
	{
		char m_name[80];

		/* Extract monster name */
		monster_desc(m_name, m_ptr, 0);

		if (m_ptr->hp > m_ptr->maxhp/3) dam = (dam + 1) / 2;
		if (rakuba((dam > 200) ? 200 : dam, FALSE))
		{
#ifdef JP
msg_format("%^sに振り落とされた！", m_name);
#else
				msg_format("You have thrown off from %s!", m_name);
#endif
		}
	}

	/* Not dead yet */
	return;
}


/*
 * Returns whether a given monster will try to run from the player.
 *
 * Monsters will attempt to avoid very powerful players.  See below.
 *
 * Because this function is called so often, little details are important
 * for efficiency.  Like not using "mod" or "div" when possible.  And
 * attempting to check the conditions in an optimal order.  Note that
 * "(x << 2) == (x * 4)" if "x" has enough bits to hold the result.
 *
 * Note that this function is responsible for about one to five percent
 * of the processor use in normal conditions...
 */
/*:::モンスターが逃げ出す判定　詳細未読*/
static int mon_will_run(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

#ifdef ALLOW_TERROR

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u16b p_lev, m_lev;
	u16b p_chp, p_mhp;
	u16b m_chp, m_mhp;
	u32b p_val, m_val;

#endif

	/* Friends can be commanded to avoid the player */
	if (is_pet(m_ptr))
	{
		/* Are we trying to avoid the player? */
		return ((p_ptr->pet_follow_distance < 0) &&
				  (m_ptr->cdis <= (0 - p_ptr->pet_follow_distance)));
	}

	//マミゾウ二ッ岩家の裁き
	if(p_ptr->pclass == CLASS_MAMIZOU && p_ptr->magic_num1[0] == m_idx)
		return (TRUE);

	/* Keep monsters from running too far away */
	if (m_ptr->cdis > MAX_SIGHT + 5) return (FALSE);

	/* All "afraid" monsters will run away */
	if (MON_MONFEAR(m_ptr)) return (TRUE);

#ifdef ALLOW_TERROR

	/* Nearby monsters will not become terrified */
	if (m_ptr->cdis <= 5) return (FALSE);

	/* Examine player power (level) */
	p_lev = p_ptr->lev;

	/* Examine monster power (level plus morale) */
	m_lev = r_ptr->level + (m_idx & 0x08) + 25;

	/* Optimize extreme cases below */
	if (m_lev > p_lev + 4) return (FALSE);
	if (m_lev + 4 <= p_lev) return (TRUE);

	/* Examine player health */
	p_chp = p_ptr->chp;
	p_mhp = p_ptr->mhp;

	/* Examine monster health */
	m_chp = m_ptr->hp;
	m_mhp = m_ptr->maxhp;

	/* Prepare to optimize the calculation */
	p_val = (p_lev * p_mhp) + (p_chp << 2); /* div p_mhp */
	m_val = (m_lev * m_mhp) + (m_chp << 2); /* div m_mhp */

	/* Strong players scare strong monsters */
	if (p_val * m_mhp > m_val * p_mhp) return (TRUE);

#endif

	/* Assume no terror */
	return (FALSE);
}




/*
 * Search spell castable grid
 */
static bool get_moves_aux2(int m_idx, int *yp, int *xp)
{
	int i, y, x, y1, x1, best = 999;

	cave_type *c_ptr;
	bool can_open_door = FALSE;
	int now_cost;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Monster can already cast spell to player */
	if (projectable(y1, x1, py, px)) return (FALSE);

	/* Set current grid cost */
	now_cost = cave[y1][x1].cost;
	if (now_cost == 0) now_cost = 999;

	/* Can monster bash or open doors? */
	if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
	{
		can_open_door = TRUE;
	}

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		int cost;

		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		/* Simply move to player */
		if (player_bold(y, x)) return (FALSE);

		c_ptr = &cave[y][x];

		cost = c_ptr->cost;

		/* Monster cannot kill or pass walls */
		if (!(((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || p_ptr->pass_wall)) || ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != p_ptr->riding))))
		{
			if (cost == 0) continue;
			if (!can_open_door && is_closed_door(c_ptr->feat)) continue;
		}

		/* Hack -- for kill or pass wall monster.. */
		if (cost == 0) cost = 998;

		if (now_cost < cost) continue;

		if (!projectable(y, x, py, px)) continue;

		/* Accept louder sounds */
		if (best < cost) continue;
		best = cost;

		(*yp) = y1 + ddy_ddd[i];
		(*xp) = x1 + ddx_ddd[i];
	}

	/* No legal move (?) */
	if (best == 999) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
 * Choose the "best" direction for "flowing"
 *
 * Note that ghosts and rock-eaters are never allowed to "flow",
 * since they should move directly towards the player.
 *
 * Prefer "non-diagonal" directions, but twiddle them a little
 * to angle slightly towards the player's actual location.
 *
 * Allow very perceptive monsters to track old "spoor" left by
 * previous locations occupied by the player.  This will tend
 * to have monsters end up either near the player or on a grid
 * recently occupied by the player (and left via "teleport").
 *
 * Note that if "smell" is turned on, all monsters get vicious.
 *
 * Also note that teleporting away from a location will cause
 * the monsters who were chasing you to converge on that location
 * as long as you are still near enough to "annoy" them without
 * being close enough to chase directly.  I have no idea what will
 * happen if you combine "smell" with low "aaf" values.
 */
/*:::モンスターが＠の「匂いをたどる」か「音をたどる」処理？*/
static bool get_moves_aux(int m_idx, int *yp, int *xp, bool no_flow)
{
	int i, y, x, y1, x1, best;

	cave_type *c_ptr;
	bool use_scent = FALSE;
	bool can_fly = FALSE;

	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/*:::魔法やブレスを使えるモンスターは、射線が通るマスに向けて優先して動く*/
	/* Can monster cast attack spell? */
	if (r_ptr->flags4 & (RF4_ATTACK_MASK) ||
	    r_ptr->flags5 & (RF5_ATTACK_MASK) ||
	    r_ptr->flags6 & (RF6_ATTACK_MASK))
	{
		/* Can move spell castable grid? */
		if (get_moves_aux2(m_idx, yp, xp)) return (TRUE);
	}

	/* Monster can't flow */
	if (no_flow) return (FALSE);

	/* Monster can go through rocks */
	if ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || p_ptr->pass_wall)) return (FALSE);
	if ((r_ptr->flags2 & RF2_KILL_WALL) && (m_idx != p_ptr->riding)) return (FALSE);
	/* Monster location */
	y1 = m_ptr->fy;
	x1 = m_ptr->fx;

	/* Hack -- Player can see us, run towards him */
	if (player_has_los_bold(y1, x1) && projectable(py, px, y1, x1)) return (FALSE);

	/* Monster grid */
	c_ptr = &cave[y1][x1];

	//v1.1.24 飛行モンスターの暗い穴経由の追跡処理
	if((r_ptr->flags7 & RF7_CAN_FLY) && c_ptr->flying_dist) can_fly = TRUE;

	/* If we can hear noises, advance towards them */
	if (c_ptr->cost || can_fly)
	{
//msg_format("chk:cost:%d",c_ptr->cost);
		best = 999;
	}
	/* Otherwise, try to follow a scent trail */
	else if (c_ptr->when)
	{
		/* Too old smell */
		if (cave[py][px].when - c_ptr->when > 127) return (FALSE);

//msg_format("chk:when:%d",c_ptr->when);
		use_scent = TRUE;
		best = 0;
	}

	/* Otherwise, advance blindly */
	else
	{
		return (FALSE);
	}

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		/* Get the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		c_ptr = &cave[y][x];

		/* We're following a scent trail */
		if (use_scent)
		{
			int when = c_ptr->when;

			/* Accept younger scent */
			if (best > when) continue;
			best = when;
		}

		/* We're using sound */
		else
		{
			int cost;

			//v1.1.24 飛んだときの最短距離が他にある場合そちらを目指す。ドアを開けられないモンスターがドアに突進することもあるがまあ大勢に影響はないだろう
			if(can_fly)
				cost = c_ptr->flying_dist;
			else if (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_OPEN_DOOR))
				cost = c_ptr->dist;
			else
				cost = c_ptr->cost;

			/* Accept louder sounds */
			if ((cost == 0) || (best < cost)) continue;
			best = cost;
		}

		/* Hack -- Save the "twiddled" location */
		(*yp) = py + 16 * ddy_ddd[i];
		(*xp) = px + 16 * ddx_ddd[i];
	}

	/* No legal move (?) */
	if (best == 999 || best == 0) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
* Provide a location to flee to, but give the player a wide berth.
*
* A monster may wish to flee to a location that is behind the player,
* but instead of heading directly for it, the monster should "swerve"
* around the player so that he has a smaller chance of getting hit.
*/
static bool get_fear_moves_aux(int m_idx, int *yp, int *xp)
{
	int y, x, y1, x1, fy, fx, gy = 0, gx = 0;
	int score = -1;
	int i;

	monster_type *m_ptr = &m_list[m_idx];

	/* Monster location */
	fy = m_ptr->fy;
	fx = m_ptr->fx;

	/* Desired destination */
	y1 = fy - (*yp);
	x1 = fx - (*xp);

	/* Check nearby grids, diagonals first */
	for (i = 7; i >= 0; i--)
	{
		int dis, s;

		/* Get the location */
		y = fy + ddy_ddd[i];
		x = fx + ddx_ddd[i];

		/* Ignore locations off of edge */
		if (!in_bounds2(y, x)) continue;

		/* Don't move toward player */
		/* if (cave[y][x].dist < 3) continue; */ /* Hmm.. Need it? */

		/* Calculate distance of this grid from our destination */
		dis = distance(y, x, y1, x1);

		/* Score this grid */
		s = 5000 / (dis + 3) - 500 / (cave[y][x].dist + 1);

		/* No negative scores */
		if (s < 0) s = 0;

		/* Ignore lower scores */
		if (s < score) continue;

		/* Save the score and time */
		score = s;

		/* Save the location */
		gy = y;
		gx = x;
	}

	/* No legal move (?) */
	if (score == -1) return (FALSE);

	/* Find deltas */
	(*yp) = fy - gy;
	(*xp) = fx - gx;

	/* Success */
	return (TRUE);
}

/*
 * Hack -- Precompute a bunch of calls to distance() in find_safety() and
 * find_hiding().
 *
 * The pair of arrays dist_offsets_y[n] and dist_offsets_x[n] contain the
 * offsets of all the locations with a distance of n from a central point,
 * with an offset of (0,0) indicating no more offsets at this distance.
 *
 * This is, of course, fairly unreadable, but it eliminates multiple loops
 * from the previous version.
 *
 * It is probably better to replace these arrays with code to compute
 * the relevant arrays, even if the storage is pre-allocated in hard
 * coded sizes.  At the very least, code should be included which is
 * able to generate and dump these arrays (ala "los()").  XXX XXX XXX
 *
 * Also, the storage needs could be halved by using bytes.  XXX XXX XXX
 *
 * These arrays could be combined into two big arrays, using sub-arrays
 * to hold the offsets and lengths of each portion of the sub-arrays, and
 * this could perhaps also be used somehow in the "look" code.  XXX XXX XXX
 */


static sint d_off_y_0[] =
{ 0 };

static sint d_off_x_0[] =
{ 0 };


static sint d_off_y_1[] =
{ -1, -1, -1, 0, 0, 1, 1, 1, 0 };

static sint d_off_x_1[] =
{ -1, 0, 1, -1, 1, -1, 0, 1, 0 };


static sint d_off_y_2[] =
{ -1, -1, -2, -2, -2, 0, 0, 1, 1, 2, 2, 2, 0 };

static sint d_off_x_2[] =
{ -2, 2, -1, 0, 1, -2, 2, -2, 2, -1, 0, 1, 0 };


static sint d_off_y_3[] =
{ -1, -1, -2, -2, -3, -3, -3, 0, 0, 1, 1, 2, 2,
  3, 3, 3, 0 };

static sint d_off_x_3[] =
{ -3, 3, -2, 2, -1, 0, 1, -3, 3, -3, 3, -2, 2,
  -1, 0, 1, 0 };


static sint d_off_y_4[] =
{ -1, -1, -2, -2, -3, -3, -3, -3, -4, -4, -4, 0,
  0, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 0 };

static sint d_off_x_4[] =
{ -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, -4, 4,
  -4, 4, -3, 3, -2, -3, 2, 3, -1, 0, 1, 0 };


static sint d_off_y_5[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -4, -4, -5, -5,
  -5, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,
  5, 0 };

static sint d_off_x_5[] =
{ -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1, 0, 1,
  -5, 5, -5, 5, -4, 4, -4, 4, -2, -3, 2, 3, -1,
  0, 1, 0 };


static sint d_off_y_6[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5,
  5, 5, 6, 6, 6, 0 };

static sint d_off_x_6[] =
{ -6, 6, -5, 5, -5, 5, -4, 4, -2, -3, 2, 3, -1,
  0, 1, -6, 6, -6, 6, -5, 5, -5, 5, -4, 4, -2,
  -3, 2, 3, -1, 0, 1, 0 };


static sint d_off_y_7[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -5, -5,
  -6, -6, -6, -6, -7, -7, -7, 0, 0, 1, 1, 2, 2, 3,
  3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 0 };

static sint d_off_x_7[] =
{ -7, 7, -6, 6, -6, 6, -5, 5, -4, -5, 4, 5, -2,
  -3, 2, 3, -1, 0, 1, -7, 7, -7, 7, -6, 6, -6,
  6, -5, 5, -4, -5, 4, 5, -2, -3, 2, 3, -1, 0,
  1, 0 };


static sint d_off_y_8[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -6, -6, -7, -7, -7, -7, -8, -8, -8, 0, 0, 1, 1,
  2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
  8, 8, 8, 0 };

static sint d_off_x_8[] =
{ -8, 8, -7, 7, -7, 7, -6, 6, -6, 6, -4, -5, 4,
  5, -2, -3, 2, 3, -1, 0, 1, -8, 8, -8, 8, -7,
  7, -7, 7, -6, 6, -6, 6, -4, -5, 4, 5, -2, -3,
  2, 3, -1, 0, 1, 0 };


static sint d_off_y_9[] =
{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6,
  -7, -7, -7, -7, -8, -8, -8, -8, -9, -9, -9, 0,
  0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7,
  7, 8, 8, 8, 8, 9, 9, 9, 0 };

static sint d_off_x_9[] =
{ -9, 9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4,
  -5, 4, 5, -2, -3, 2, 3, -1, 0, 1, -9, 9, -9,
  9, -8, 8, -8, 8, -7, 7, -7, 7, -6, 6, -4, -5,
  4, 5, -2, -3, 2, 3, -1, 0, 1, 0 };


static sint *dist_offsets_y[10] =
{
	d_off_y_0, d_off_y_1, d_off_y_2, d_off_y_3, d_off_y_4,
	d_off_y_5, d_off_y_6, d_off_y_7, d_off_y_8, d_off_y_9
};

static sint *dist_offsets_x[10] =
{
	d_off_x_0, d_off_x_1, d_off_x_2, d_off_x_3, d_off_x_4,
	d_off_x_5, d_off_x_6, d_off_x_7, d_off_x_8, d_off_x_9
};

/*
* Choose a "safe" location near a monster for it to run toward.
*
* A location is "safe" if it can be reached quickly and the player
* is not able to fire into it (it isn't a "clean shot").  So, this will
* cause monsters to "duck" behind walls.  Hopefully, monsters will also
* try to run towards corridor openings if they are in a room.
*
* This function may take lots of CPU time if lots of monsters are
* fleeing.
*
* Return TRUE if a safe location is available.
*/
static bool find_safety(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int y, x, dy, dx, d, dis, i;
	int gy = 0, gx = 0, gdis = 0;

	sint *y_offsets;
	sint *x_offsets;

	cave_type *c_ptr;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++)
	{
		/* Get the lists of points with a distance d from (fx, fy) */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];

		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i])
		{
			y = fy + dy;
			x = fx + dx;

			/* Skip illegal locations */
			if (!in_bounds(y, x)) continue;

			c_ptr = &cave[y][x];

			/* Skip locations in a wall */
			if (!monster_can_cross_terrain(c_ptr->feat, &r_info[m_ptr->r_idx], (m_idx == p_ptr->riding) ? CEM_RIDING : 0)) continue;

			/* Check for "availability" (if monsters can flow) */
			if (!(m_ptr->mflag2 & MFLAG2_NOFLOW))
			{
				/* Ignore grids very far from the player */
				if (c_ptr->dist == 0) continue;

				/* Ignore too-distant grids */
				if (c_ptr->dist > cave[fy][fx].dist + 2 * d) continue;
			}

			/* Check for absence of shot (more or less) */
			if (!projectable(py, px, y, x))
			{
				/* Calculate distance from player */
				dis = distance(y, x, py, px);

				/* Remember if further than previous */
				if (dis > gdis)
				{
					gy = y;
					gx = x;
					gdis = dis;
				}
			}
		}

		/* Check for success */
		if (gdis > 0)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found safe place */
			return (TRUE);
		}
	}

	/* No safe place */
	return (FALSE);
}


/*
 * Choose a good hiding place near a monster for it to run toward.
 *
 * Pack monsters will use this to "ambush" the player and lure him out
 * of corridors into open space so they can swarm him.
 *
 * Return TRUE if a good location is available.
 */
static bool find_hiding(int m_idx, int *yp, int *xp)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int fy = m_ptr->fy;
	int fx = m_ptr->fx;

	int y, x, dy, dx, d, dis, i;
	int gy = 0, gx = 0, gdis = 999;

	sint *y_offsets, *x_offsets;

	/* Start with adjacent locations, spread further */
	for (d = 1; d < 10; d++)
	{
		/* Get the lists of points with a distance d from (fx, fy) */
		y_offsets = dist_offsets_y[d];
		x_offsets = dist_offsets_x[d];

		/* Check the locations */
		for (i = 0, dx = x_offsets[0], dy = y_offsets[0];
		     dx != 0 || dy != 0;
		     i++, dx = x_offsets[i], dy = y_offsets[i])
		{
			y = fy + dy;
			x = fx + dx;

			/* Skip illegal locations */
			if (!in_bounds(y, x)) continue;

			/* Skip occupied locations */
			if (!monster_can_enter(y, x, r_ptr, 0)) continue;

			/* Check for hidden, available grid */
			if (!projectable(py, px, y, x) && clean_shot(fy, fx, y, x, FALSE))
			{
				/* Calculate distance from player */
				dis = distance(y, x, py, px);

				/* Remember if closer than previous */
				if (dis < gdis && dis >= 2)
				{
					gy = y;
					gx = x;
					gdis = dis;
				}
			}
		}

		/* Check for success */
		if (gdis < 999)
		{
			/* Good location */
			(*yp) = fy - gy;
			(*xp) = fx - gx;

			/* Found good place */
			return (TRUE);
		}
	}

	/* No good place */
	return (FALSE);
}


/*
 * Choose "logical" directions for monster movement
 */
/*:::モンスターがターゲットか＠のほうへ向かう　敵とペットの処理から呼ばれる*/
static bool get_moves(int m_idx, int *mm)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	int          y, ay, x, ax;
	int          move_val = 0;
	int          y2 = py;
	int          x2 = px;
	bool         done = FALSE;
	bool         will_run = mon_will_run(m_idx);
	cave_type    *c_ptr;
	bool         no_flow = ((m_ptr->mflag2 & MFLAG2_NOFLOW) && (cave[m_ptr->fy][m_ptr->fx].cost > 2));
	bool         can_pass_wall = ((r_ptr->flags2 & RF2_PASS_WALL) && ((m_idx != p_ptr->riding) || p_ptr->pass_wall));


	if(m_ptr->r_idx == MON_HASSYAKUSAMA && hassyakusama_target_idx)
	{
		monster_type *tmp_m_ptr = &m_list[hassyakusama_target_idx];
		if( projectable(m_ptr->fy, m_ptr->fx, tmp_m_ptr->fy, tmp_m_ptr->fx))
		{
			y = m_ptr->fy - tmp_m_ptr->fy;
			x = m_ptr->fx - tmp_m_ptr->fx;
			done = TRUE;

		}
	}

	/*:::視界内にターゲットが居ればそちらに向かう*/
	/* Counter attack to an enemy monster */
	if (!will_run && m_ptr->target_y)
	{
		int t_m_idx = cave[m_ptr->target_y][m_ptr->target_x].m_idx;

		/* The monster must be an enemy, and in LOS */
		if (t_m_idx &&
		    are_enemies(m_ptr, &m_list[t_m_idx]) &&
		    los(m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x) &&
		    projectable(m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x))
		{
			/* Extract the "pseudo-direction" */
			y = m_ptr->fy - m_ptr->target_y;
			x = m_ptr->fx - m_ptr->target_x;
			done = TRUE;
		}
	}

	/*:::集団で現れる敵が視界内にいるとき*/
	if (!done && !will_run && is_hostile(m_ptr) &&
	    (r_ptr->flags1 & RF1_FRIENDS) &&
	    ((los(m_ptr->fy, m_ptr->fx, py, px) && projectable(m_ptr->fy, m_ptr->fx, py, px)) ||
	    (cave[m_ptr->fy][m_ptr->fx].dist < MAX_SIGHT / 2)))
	{
	/*
	 * Animal packs try to get the player out of corridors
	 * (...unless they can move through walls -- TY)
	 */
		/*:::動物は通路に逃げようとする*/
		if ((r_ptr->flags3 & RF3_ANIMAL) && !can_pass_wall &&
			 !(r_ptr->flags2 & RF2_KILL_WALL))
		{
			int i, room = 0;

			/* Count room grids next to player */
			for (i = 0; i < 8; i++)
			{
				int xx = px + ddx_ddd[i];
				int yy = py + ddy_ddd[i];

				if (!in_bounds2(yy, xx)) continue;

				c_ptr = &cave[yy][xx];

				/* Check grid */
				if (monster_can_cross_terrain(c_ptr->feat, r_ptr, 0))
				{
					/* One more room grid */
					room++;
				}
			}
			if (cave[py][px].info & CAVE_ROOM) room -= 2;
			if (!r_ptr->flags4 && !r_ptr->flags5 && !r_ptr->flags6) room -= 2;

			/* Not in a room and strong player */
			if (room < (8 * (p_ptr->chp + p_ptr->csp)) /
			    (p_ptr->mhp + p_ptr->msp))
			{
				/* Find hiding place */
				if (find_hiding(m_idx, &y, &x)) done = TRUE;
			}
		}
		/*:::＠を囲もうとする*/
		/* Monster groups try to surround the player */
		if (!done && (cave[m_ptr->fy][m_ptr->fx].dist < 3))
		{
			int i;

			/* Find an empty square near the player to fill */
			for (i = 0; i < 8; i++)
			{
				/* Pick squares near player (semi-randomly) */
				y2 = py + ddy_ddd[(m_idx + i) & 7];
				x2 = px + ddx_ddd[(m_idx + i) & 7];

				/* Already there? */
				if ((m_ptr->fy == y2) && (m_ptr->fx == x2))
				{
					/* Attack the player */
					y2 = py;
					x2 = px;

					break;
				}

				if (!in_bounds2(y2, x2)) continue;

				/* Ignore filled grids */
				if (!monster_can_enter(y2, x2, r_ptr, 0)) continue;

				/* Try to fill this hole */
				break;
			}

			/* Extract the new "pseudo-direction" */
			y = m_ptr->fy - y2;
			x = m_ptr->fx - x2;

			/* Done */
			done = TRUE;
		}
	}
	//パルスィ特殊処理
	if(m_ptr->r_idx == MON_PARSEE && (m_ptr->mflag & MFLAG_EPHEMERA))
	{
		done = FALSE;
		will_run = FALSE;
	}

	/*:::上のような特殊処理以外*/
	if (!done)
	{
		/* Flow towards the player */
		(void)get_moves_aux(m_idx, &y2, &x2, no_flow);

		/* Extract the "pseudo-direction" */
		y = m_ptr->fy - y2;
		x = m_ptr->fx - x2;

		/* Not done */
	}

	/* Apply fear if possible and necessary */
	if (is_pet(m_ptr) && will_run)
	{
		/* XXX XXX Not very "smart" */
		y = (-y), x = (-x);
	}
	else
	{
		if (!done && will_run)
		{
			int tmp_x = (-x);
			int tmp_y = (-y);

			/* Try to find safe place */
			if (find_safety(m_idx, &y, &x))
			{
				/* Attempt to avoid the player */
				if (!no_flow)
				{
					/* Adjust movement */
					if (get_fear_moves_aux(m_idx, &y, &x)) done = TRUE;
				}
			}

			if (!done)
			{
				/* This is not a very "smart" method XXX XXX */
				y = tmp_y;
				x = tmp_x;
			}
		}
	}


	/* Check for no move */
	if (!x && !y) return (FALSE);


	/* Extract the "absolute distances" */
	ax = ABS(x);
	ay = ABS(y);

	/* Do something weird */
	if (y < 0) move_val += 8;
	if (x > 0) move_val += 4;

	/* Prevent the diamond maneuvre */
	if (ay > (ax << 1)) move_val += 2;
	else if (ax > (ay << 1)) move_val++;

	/* Extract some directions */
	/*:::決まった移動先に向けて進む一歩の方向候補をmm[]に格納*/
	switch (move_val)
	{
	case 0:
		mm[0] = 9;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 6;
			mm[3] = 7;
			mm[4] = 3;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 8;
			mm[3] = 3;
			mm[4] = 7;
		}
		break;
	case 1:
	case 9:
		mm[0] = 6;
		if (y < 0)
		{
			mm[1] = 3;
			mm[2] = 9;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 9;
			mm[2] = 3;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 2:
	case 6:
		mm[0] = 8;
		if (x < 0)
		{
			mm[1] = 9;
			mm[2] = 7;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 9;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 4:
		mm[0] = 7;
		if (ay > ax)
		{
			mm[1] = 8;
			mm[2] = 4;
			mm[3] = 9;
			mm[4] = 1;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 8;
			mm[3] = 1;
			mm[4] = 9;
		}
		break;
	case 5:
	case 13:
		mm[0] = 4;
		if (y < 0)
		{
			mm[1] = 1;
			mm[2] = 7;
			mm[3] = 2;
			mm[4] = 8;
		}
		else
		{
			mm[1] = 7;
			mm[2] = 1;
			mm[3] = 8;
			mm[4] = 2;
		}
		break;
	case 8:
		mm[0] = 3;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 6;
			mm[3] = 1;
			mm[4] = 9;
		}
		else
		{
			mm[1] = 6;
			mm[2] = 2;
			mm[3] = 9;
			mm[4] = 1;
		}
		break;
	case 10:
	case 14:
		mm[0] = 2;
		if (x < 0)
		{
			mm[1] = 3;
			mm[2] = 1;
			mm[3] = 6;
			mm[4] = 4;
		}
		else
		{
			mm[1] = 1;
			mm[2] = 3;
			mm[3] = 4;
			mm[4] = 6;
		}
		break;
	case 12:
		mm[0] = 1;
		if (ay > ax)
		{
			mm[1] = 2;
			mm[2] = 4;
			mm[3] = 3;
			mm[4] = 7;
		}
		else
		{
			mm[1] = 4;
			mm[2] = 2;
			mm[3] = 7;
			mm[4] = 3;
		}
		break;
	}

	/* Wants to move... */
	return (TRUE);
}


static int check_hit2(int power, int level, int ac, int stun)
{
	int i, k;

	/* Percentile dice */
	k = randint0(100);

	if (stun && one_in_(2)) return FALSE;

	/* Hack -- Always miss or hit */
	if (k < 10) return (k < 5);

	/* Calculate the "attack quality" */
	i = (power + (level * 3));

	/* Power and Level compete against Armor */
	if ((i > 0) && (randint1(i) > ((ac * 3) / 4))) return (TRUE);

	/* Assume miss */
	return (FALSE);
}


#define BLOW_EFFECT_TYPE_NONE  0
#define BLOW_EFFECT_TYPE_FEAR  1
#define BLOW_EFFECT_TYPE_SLEEP 2
#define BLOW_EFFECT_TYPE_HEAL  3
#define BLOW_EFFECT_TYPE_FOOD	4

//v1.1.94
#define BLOW_EFFECT_TYPE_DEC_ATK	5
#define BLOW_EFFECT_TYPE_DEC_DEF	6
#define BLOW_EFFECT_TYPE_DEC_MAG	7
#define BLOW_EFFECT_TYPE_DEC_ALL	8



/* Monster attacks monster */
/*:::モンスター同士の通常攻撃処理*/
///sys mon モンスター同士の通常攻撃　＠に対するのと同様に修正
static bool monst_attack_monst(int m_idx, int t_idx)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_type    *t_ptr = &m_list[t_idx];

	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	monster_race    *tr_ptr = &r_info[t_ptr->r_idx];

	int             ap_cnt;
	int             ac, rlev, pt;
	char            m_name[80], t_name[80];
	char            temp[MAX_NLEN];
	bool            blinked;
	bool            explode = FALSE, touched = FALSE, fear = FALSE;
	int             y_saver = t_ptr->fy;
	int             x_saver = t_ptr->fx;
	int             effect_type;

	bool flag_paparazzo = FALSE;
	bool see_m = is_seen(m_ptr);
	bool see_t = is_seen(t_ptr);
	bool see_either = see_m || see_t;

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
	bool do_silly_attack = (one_in_(2) && p_ptr->image);

	/* Cannot attack self */
	if (m_idx == t_idx) return FALSE;

	//マミゾウ二ッ岩家の裁きで変身中のモンスターは攻撃不可
	if(p_ptr->pclass == CLASS_MAMIZOU && p_ptr->magic_num1[0] == m_idx)
	{
		return TRUE;
	}
	/* Not allowed to attack */
	if (r_ptr->flags1 & RF1_NEVER_BLOW) return FALSE;

	if (d_info[dungeon_type].flags1 & DF1_NO_MELEE) return (FALSE);

	//v1.1.64 ハニワにはGシンボルの攻撃が効かないことにする
	if (r_ptr->d_char == 'G' && tr_ptr->flags3 & RF3_HANIWA)
	{
		monster_desc(m_name, m_ptr, 0);
		msg_format(_("%^sの攻撃は埴輪には効いていない！",
                    "The attack of %s has no effect on haniwa!"), m_name);
		return TRUE;
	}


	/* Total armor */
	ac = tr_ptr->ac;


	//v1.1.94 モンスター防御力低下中はAC25%カット
	if (MON_DEC_DEF(t_ptr))
	{
		ac = MONSTER_DECREASED_AC(ac);
	}

	//v1.1.41 舞特殊騎乗　AC増加
	if (p_ptr->pclass == CLASS_MAI && p_ptr->riding == t_idx)
	{
		int chr = p_ptr->stat_ind[A_CHR]+3;
		ac += chr * 2;
	}


	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Get the monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Get the monster name (or "it") */
	monster_desc(t_name, t_ptr, 0);

	/* Assume no blink */
	blinked = FALSE;

	if (!see_either && known)
	{
		mon_fight = TRUE;
	}

	if (p_ptr->riding && (m_idx == p_ptr->riding)) disturb(1, 1);

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		int power = 0;
		int damage = 0;

		cptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (!m_ptr->r_idx) break;

		/* Stop attacking if the target dies! */
		if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
			break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (method == RBM_SHOOT) continue;

		//v1.1.41 舞特殊騎乗　隣接攻撃ダメージダイス増加
		if (p_ptr->pclass == CLASS_MAI && p_ptr->riding == m_idx)
		{
			int chr = p_ptr->stat_ind[A_CHR] + 3;
			d_dice += MAX(0,d_dice * (chr-5) / 15);
		}
		//v1.1.95 モンスター狂戦士化中はダメージ2倍 ただし舞のダンスの効果に累積しない
		else if (MON_BERSERK(m_ptr))
		{
			d_side *= 2;
		}

		//喋れないモンスターなど特殊処理
		if((m_ptr->mflag & MFLAG_NO_SPELL) ||
		p_ptr->silent_floor)
		{
			if(method == RBM_MOAN || method == RBM_SHOW || method == RBM_WHISPER ||method == RBM_BEG || method == RBM_INSULT)
			{
			    msg_format(_("%^sは何かを喋ろうとしたようだ。",
                                "%^s tries to say something."), m_name);
				continue;
			}
		}
		if((p_ptr->pclass == CLASS_LUNAR || p_ptr->pclass == CLASS_3_FAIRIES) && (p_ptr->tim_general[0] && m_ptr->cdis <= p_ptr->lev / 5 || p_ptr->tim_general[1]))
		{
			if(method == RBM_MOAN || method == RBM_SHOW || method == RBM_WHISPER ||method == RBM_BEG || method == RBM_INSULT || method == RBM_HOWL)
			{
			    msg_format(_("%^sは声を出せなかった。",
                                "%^s couldn't speak."), m_name);
				continue;
			}
			else if(method == RBM_PLAY)
			{
			    msg_format(_("%^sは演奏に失敗した。",
                                "%^s fails to perform."), m_name);
				continue;
			}
		}

//msg_format("num:%d eff:%d meth:%d",ap_cnt,effect,method);


		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Monster hits */
		if (!effect || check_hit2(power, rlev, ac, MON_STUNNED(m_ptr)))
		{
			/* Wake it up */
			(void)set_monster_csleep(t_idx, 0);

			if (t_ptr->ml)
			{
				/* Redraw the health bar */
				if (p_ptr->health_who == t_idx) p_ptr->redraw |= (PR_HEALTH);
				if (p_ptr->riding == t_idx) p_ptr->redraw |= (PR_UHEALTH);
			}

			/* Describe the attack method */
			switch (method)
			{
			case RBM_HIT:
				{
#ifdef JP
		///mod131231 モンスター攻撃のHITを殴るから「攻撃する」へ表記変更
act = "%sを攻撃した。";
#else
					act = "hits %s.";
#endif

					if(effect == RBE_PHOTO) act = _("%sを撮影した。", "takes a photo of %s.");

					touched = TRUE;
					break;
				}

			case RBM_TOUCH:
				{
#ifdef JP
act = "%sを触った。";
#else
					act = "touches %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_PUNCH:
				{
#ifdef JP
act = "%sを殴った。";
#else
					act = "punches %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_KICK:
				{
#ifdef JP
act = "%sを蹴った。";
#else
					act = "kicks %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CLAW:
				{
#ifdef JP
act = "%sをひっかいた。";
#else
					act = "claws %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_BITE:
				{
#ifdef JP
act = "%sを噛んだ。";
#else
					act = "bites %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_STING:
				{
#ifdef JP
act = "%sを刺した。";
#else
					act = "stabs %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_SLASH:
				{
#ifdef JP
act = "%sを斬った。";
#else
					act = "slashes %s.";
#endif
					touched = TRUE;

					break;
				}

			case RBM_BUTT:
				{
#ifdef JP
act = "%sを角で突いた。";
#else
					act = "butts %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CRUSH:
				{
#ifdef JP
act = "%sに体当りした。";
#else
					act = "crushes %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_ENGULF:
				{
#ifdef JP
act = "%sを飲み込んだ。";
#else
					act = "engulfs %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CHARGE:
				{
#ifdef JP
act = "%sに請求書をよこした。";
#else
					act = "charges %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_CRAWL:
				{
#ifdef JP
act = "%sの体の上を這い回った。";
#else
					act = "crawls on %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_DROOL:
				{
#ifdef JP
act = "%sによだれをたらした。";
#else
					act = "drools on %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SPIT:
				{
#ifdef JP
act = "%sに唾を吐いた。";
#else
					act = "spits on %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_EXPLODE:
				{
					if (see_either) disturb(1, 1);
#ifdef JP
act = "爆発した。";
#else
					act = "explodes.";
#endif

					explode = TRUE;
					touched = FALSE;
					break;
				}

			case RBM_GAZE:
				{
#ifdef JP
act = "%sをにらんだ。";
#else
					act = "gazes at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_WAIL:
				{
#ifdef JP
act = "%sに泣きついた。";
#else
					act = "wails at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SPORE:
				{
#ifdef JP
act = "%sに胞子を飛ばした。";
#else
					act = "releases spores at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_DANCE:
				{
#ifdef JP
act = "%sの回りで踊った。";
#else
					act = "projects XXX4's at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_BEG:
				{
#ifdef JP
act = "%sに金をせがんだ。";
#else
					act = "begs %s for money.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_INSULT:
				{
#ifdef JP
act = "%sを侮辱した。";
#else
					act = "insults %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_MOAN:
				{
#ifdef JP
act = "%sにむかってうめいた。";
#else
					act = "moans at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_SHOW:
				{
#ifdef JP
act = "%sにむかって歌った。";
#else
					act = "sings to %s.";
#endif

					touched = FALSE;
					break;
				}
				///mod131231 新攻撃方法実装
			case RBM_SPEAR:
				{
#ifdef JP
act = "%sを突いた。";
#else
					act = "thrusts at %s.";
#endif

					touched = TRUE;
					break;
				}
			case RBM_BOOK:
				{
#ifdef JP
act = "%sを分厚い本で殴った。";
#else
					act = "bludgeons %s with a thick book.";
#endif

					touched = TRUE;
					break;
				}
			case RBM_MOFUMOFU:
				{
#ifdef JP
act = "%sを尻尾でモフモフした。";
#else
					act = "envelops %s with fluffy tail.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_SQUEEZE:
				{
#ifdef JP
act = "%sを締め付けた。";
#else
					act = "squeezes %s.";
#endif

					touched = TRUE;
					break;
				}
			case RBM_DRAG:
				{
#ifdef JP
act = "%sを引きずり込んだ。";
#else
					act = "drags %s in.";
#endif

					touched = TRUE;
					break;
				}
			case RBM_INJECT:
				{
#ifdef JP
act = "%sに何かを注射した。";
#else
					act = "injects %s with something.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_SMILE:
				{
#ifdef JP
act = "%sに微笑みかけた。";
#else
					act = "smiles at %s.";
#endif

					touched = FALSE;
					break;
				}

			case RBM_HEADBUTT:
				{
#ifdef JP
act = "%sに頭突きした。";
#else
					act = "headbutts %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_FLASH:
				{
#ifdef JP
act = "%sの前で光った。";
#else
					act = "lets out a bright flash in front of %s.";
#endif

					touched = FALSE;

					break;
				}

			case RBM_HOWL:
				{
#ifdef JP
act = "%sに向けて吠えた。";
#else
					act = "howls at %s.";
#endif


					break;
				}

			case RBM_WHISPER:
				{
#ifdef JP
act = "%sに何かを囁いた。";
#else
					act = "whispers something to %s.";
#endif


					break;
				}

			case RBM_PRESS:
				{
#ifdef JP
act = "%sを押し潰した。";
#else
					act = "presses %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_POINT:
				{
#ifdef JP
act = "%sを指差した。";
#else
					act = "points at %s.";
#endif


					break;
				}

			case RBM_STRIKE:
				{
#ifdef JP
act = "%sを打ち据えた。";
#else
					act = "strikes %s.";
#endif

					touched = TRUE;
					break;
				}

			case RBM_PLAY:
				{
#ifdef JP
act = "%sの耳元で轟音を出した。";
#else
					act = "plays loud music at %s.";
#endif

					break;
				}

			case RBM_SMOKE:
			{
				act = _("%sに煙を吹きかけた。",
                        "blows smoke at %s.");
				break;
			}

			}

			/* Message */
			if (act && see_either)
			{

				if (do_silly_attack) act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];
				strfmt(temp, act, t_name);
#ifdef JP
				msg_format("%^sは%s", m_name, temp);
#else
				msg_format("%^s %s", m_name, temp);
#endif
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			//v1.1.94 モンスター攻撃力低下中はダメージ25%カット
			if (MON_DEC_ATK(m_ptr))
			{
				damage = (damage * 3 + 1) / 4;
			}

			/* Assume no effect */
			effect_type = BLOW_EFFECT_TYPE_NONE;

			pt = GF_MISSILE;

			/* Apply appropriate damage */
			switch (effect)
			{
			case 0:
			case RBE_DR_MANA:
				damage = pt = 0;
				break;

			case RBE_SUPERHURT:
				//v1.1.94 弱体化中は痛恨が出ない
				if (((randint1(rlev*2+250) > (ac+200)) || one_in_(13)) && !MON_DEC_ATK(m_ptr))
				{
					int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
					damage = MAX(damage, tmp_damage * 2);
					break;
				}

				/* Fall through */
			case RBE_BLEED:
			case RBE_HURT:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				break;

			case RBE_POISON:
			case RBE_DISEASE:
				pt = GF_POIS;
				break;

			case RBE_UN_BONUS:
			case RBE_UN_POWER:
				pt = GF_DISENCHANT;
				break;

			case RBE_EAT_ITEM:
			case RBE_EAT_GOLD:
				if ((p_ptr->riding != m_idx) && one_in_(2)) blinked = TRUE;
				break;

			case RBE_EAT_FOOD:
			case RBE_EAT_LITE:
			case RBE_BLIND:
				break;
			case RBE_LOSE_STR:
			case RBE_LOSE_DEX:
				effect_type = BLOW_EFFECT_TYPE_DEC_ATK;
				break;

			case RBE_LOSE_INT:
			case RBE_LOSE_WIS:
				effect_type = BLOW_EFFECT_TYPE_DEC_MAG;
				break;
			case RBE_LOSE_CON:
			case RBE_LOSE_CHR:
				effect_type = BLOW_EFFECT_TYPE_DEC_DEF;
				break;

			case RBE_LOSE_ALL:
				effect_type = BLOW_EFFECT_TYPE_DEC_ALL;
				break;

			case RBE_ACID:
				pt = GF_ACID;
				break;

			case RBE_ELEC:
				pt = GF_ELEC;
				break;

			case RBE_FIRE:
				pt = GF_FIRE;
				break;

			case RBE_COLD:
				pt = GF_COLD;
				break;

			case RBE_CONFUSE: case RBE_INSANITY:
				pt = GF_CONFUSION;
				break;

			case RBE_TERRIFY:
				effect_type = BLOW_EFFECT_TYPE_FEAR;
				break;

			case RBE_PARALYZE:
				effect_type = BLOW_EFFECT_TYPE_SLEEP;
				break;

			case RBE_SHATTER:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				if (damage > 23) earthquake_aux(m_ptr->fy, m_ptr->fx, 8, m_idx);
				break;

			case RBE_EXP_10:
			case RBE_EXP_20:
			case RBE_EXP_40:
			case RBE_EXP_80:
				pt = GF_NETHER;
				break;

			case RBE_TIME:
				pt = GF_TIME;
				break;
				///mod131231 吸収打撃は対モンスターでは属性が違うがまあ大勢に影響はなかろう
				//v1.1.32 空腹打撃も追加した
			case RBE_EXP_VAMP: case RBE_MELT:case RBE_HUNGER:
				pt = GF_OLD_DRAIN;
				effect_type = BLOW_EFFECT_TYPE_HEAL;
				break;

			case RBE_INERTIA:
				pt = GF_INACT;
				break;

			case RBE_STUN:
				pt = GF_SOUND;
				break;

			///mod131230 モンスター→モンスターの通常攻撃　新しい効果を追加
			case RBE_CHAOS: case RBE_MUTATE:
				pt = GF_CHAOS;
				break;
			case RBE_ELEMENT:
				if(one_in_(4))pt = GF_FIRE;
				else if(one_in_(3))pt = GF_ACID;
				else if(one_in_(2))pt = GF_ELEC;
				else pt = GF_COLD;
				break;
			case RBE_SMITE:
				pt = GF_HOLY_FIRE;
				break;
			case RBE_DROWN:
				pt = GF_WATER;
				break;
			case RBE_KILL: case RBE_CURSE:
				pt = GF_NETHER;
				break;

				///pend モンスターに対する写真撮影処理要
			case RBE_PHOTO:
				if ((p_ptr->riding != m_idx) && one_in_(2)) blinked = TRUE;
				flag_paparazzo = TRUE;
				break;
			default:
				pt = 0;
				break;
			}

			if (pt)
			{

				int effect_power;

				if (r_ptr->flags2 & RF2_POWERFUL)
					effect_power = r_ptr->level * 3;
				else
					effect_power = r_ptr->level * 2;

				/* Do damage if not exploding */
				if (!explode)
				{
					project(m_idx, 0, t_ptr->fy, t_ptr->fx,	damage, pt, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
				}
				if(!t_ptr->r_idx) break;

				switch (effect_type)
				{
				case BLOW_EFFECT_TYPE_FEAR:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx,
						damage, GF_TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_SLEEP:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx,
						effect_power, GF_OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

					//v1.1.94
				case BLOW_EFFECT_TYPE_DEC_ATK:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_ATK, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_DEF:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_DEF, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_MAG:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_MAG, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_ALL:
					project(m_idx, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;


				case BLOW_EFFECT_TYPE_HEAL:
					if ((monster_living(tr_ptr)) && (damage > 2))
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						///mod150305 レミリアの蝙蝠によるHP吸収
						if(m_ptr->r_idx == MON_V_BAT && m_ptr->mflag & MFLAG_EPHEMERA && p_ptr->pclass == CLASS_REMY)
						{
							hp_player(damroll(4, damage / 6));
						}

						/* Redraw (later) if needed */
						if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
						if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (see_m && did_heal)
						{
#ifdef JP
							msg_format("%sは体力を回復したようだ。", m_name);
#else
							msg_format("%^s appears healthier.", m_name);
#endif
						}
					}
					break;
				}

				if (touched)
				{
					/* Aura fire */
					if ((tr_ptr->flags2 & RF2_AURA_FIRE) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
						{
							if (see_either)
							{
#ifdef JP
								msg_format("%^sは突然熱くなった！", m_name);
#else
								msg_format("%^s is suddenly very hot!", m_name);
#endif
							}
							if (m_ptr->ml && is_original_ap_and_seen(t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_FIRE;
							project(t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll (1 + ((tr_ptr->level) / 26),
								1 + ((tr_ptr->level) / 17)),
								GF_FIRE, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
						}
					}

					/* Aura cold */
///mod131231 モンスターフラグ変更
					//if ((tr_ptr->flags3 & RF3_AURA_COLD) && m_ptr->r_idx)
					if ((tr_ptr->flags2 & RF2_AURA_COLD) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
						{
							if (see_either)
							{
#ifdef JP
								msg_format("%^sは突然寒くなった！", m_name);
#else
								msg_format("%^s is suddenly very cold!", m_name);
#endif
							}
							//if (m_ptr->ml && is_original_ap_and_seen(t_ptr)) tr_ptr->r_flags3 |= RF3_AURA_COLD;
							if (m_ptr->ml && is_original_ap_and_seen(t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_COLD;
							project(t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll (1 + ((tr_ptr->level) / 26),
								1 + ((tr_ptr->level) / 17)),
								GF_COLD, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
						}
					}

					/* Aura elec */
					if ((tr_ptr->flags2 & RF2_AURA_ELEC) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
						{
							if (see_either)
							{
#ifdef JP
								msg_format("%^sは電撃を食らった！", m_name);
#else
								msg_format("%^s gets zapped!", m_name);
#endif
							}
							if (m_ptr->ml && is_original_ap_and_seen(t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_ELEC;
							project(t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll (1 + ((tr_ptr->level) / 26),
								1 + ((tr_ptr->level) / 17)),
								GF_ELEC, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
						}
					}
				}
			}
		}

		/* Monster missed player */
		else
		{
			/* Analyze failed attacks */
			switch (method)
			{
			case RBM_HIT:
			case RBM_TOUCH:
			case RBM_PUNCH:
			case RBM_KICK:
			case RBM_CLAW:
			case RBM_BITE:
			case RBM_STING:
			case RBM_SLASH:
			case RBM_BUTT:
			case RBM_CRUSH:
			case RBM_ENGULF:
			case RBM_CHARGE:
				//v1.1.54 追加し忘れてたので起きそうな攻撃を追加
			case RBM_SPEAR:
			case RBM_BOOK:
			case RBM_SQUEEZE:
			case RBM_DRAG:
			case RBM_INJECT:
			case RBM_HEADBUTT:
			case RBM_PRESS:
			case RBM_STRIKE:
				{
					/* Wake it up */
					(void)set_monster_csleep(t_idx, 0);

					/* Visible monsters */
					if (see_m)
					{
						/* Message */
#ifdef JP
						msg_format("%sは%^sの攻撃をかわした。", t_name,m_name);
#else
						msg_format("%^s misses %s.", m_name, t_name);
#endif
					}

					break;
				}
			}
		}


		/* Analyze "visible" monsters only */
		if (is_original_ap_and_seen(m_ptr) && !do_silly_attack)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
				{
					r_ptr->r_blows[ap_cnt]++;
				}
			}
		}
	}


	if (explode)
	{
		sound(SOUND_EXPLODE);

		/* Cancel Invulnerability */
		(void)set_monster_invulner(m_idx, 0, FALSE);

#ifdef JP
		mon_take_hit_mon(m_idx, m_ptr->hp + 1, &fear, "は爆発して粉々になった。", m_idx);
#else
		mon_take_hit_mon(m_idx, m_ptr->hp + 1, &fear, " explodes into tiny shreds.", m_idx);
#endif

		blinked = FALSE;
	}

	/* Blink away */
	if (blinked && m_ptr->r_idx)
	{
		cptr temp_desc;
		if(flag_paparazzo) temp_desc = _("パパラッチ", "paparazzi");
		else	temp_desc = _("泥棒", "thief");

		if (teleport_barrier(m_idx))
		{
			if (see_m)
			{
				msg_format(_("%sは笑って逃げ...ようとしたが引き戻された。",
                            "The %s flees laughing...? But the magical barrier obstructs it."),temp_desc);
			}
			else if (known)
			{
				mon_fight = TRUE;
			}
		}
		else
		{
			if (see_m)
			{
				msg_format(_("%sは笑って逃げた！", "The %s flees laughing!"),temp_desc);
			}
			else if (known)
			{
				mon_fight = TRUE;
			}

			teleport_away(m_idx, MAX_SIGHT * 2 + 5, 0L);
		}
	}

	return TRUE;
}


/*::: -Mega Hack- ＠が特殊能力でモンスターに変身してモンスターへ通常攻撃したときの処理 monst_attack_monstをベースに色々変更*/
bool monplayer_attack_monst(int t_idx)
{
	monster_type    *t_ptr = &m_list[t_idx];

	monster_race    *r_ptr = &r_info[MON_EXTRA_FIELD];
	monster_race    *tr_ptr = &r_info[t_ptr->r_idx];

	int             ap_cnt;
	int             ac, rlev, pt;
	char            t_name[80];
	char            temp[MAX_NLEN];
	bool            blinked = FALSE;
	bool            explode = FALSE, touched = FALSE, fear = FALSE;
	int             y_saver = t_ptr->fy;
	int             x_saver = t_ptr->fx;
	int             effect_type;

	bool see_t = is_seen(t_ptr);

	/* Can the player be aware of this attack? */
	bool do_silly_attack = (one_in_(2) && p_ptr->image);

	/* Not allowed to attack */
	if (r_ptr->flags1 & RF1_NEVER_BLOW)
	{
		msg_print(_("この身体では攻撃ができない。", "You cannot attack in this form."));
		return FALSE;
	}

	if (d_info[dungeon_type].flags1 & DF1_NO_MELEE) return (FALSE);

	/* Total armor */
	ac = tr_ptr->ac;


	//v1.1.94 モンスター防御力低下中はAC25%カット
	if (MON_DEC_DEF(t_ptr))
	{
		ac = MONSTER_DECREASED_AC(ac);
	}

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Get the monster name (or "it") */
	monster_desc(t_name, t_ptr, 0);

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		int power = 0;
		int damage = 0;

		cptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (p_ptr->is_dead) break;

		/* Stop attacking if the target dies! */
		if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
			break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Monster hits */
		if (!effect || check_hit2(power, rlev, ac, p_ptr->stun))
		{
			/* Wake it up */
			(void)set_monster_csleep(t_idx, 0);

			if (t_ptr->ml)
			{
				/* Redraw the health bar */
				if (p_ptr->health_who == t_idx) p_ptr->redraw |= (PR_HEALTH);
				if (p_ptr->riding == t_idx) p_ptr->redraw |= (PR_UHEALTH);
			}

			/* Describe the attack method */
			switch (method)
			{
			case RBM_HIT:
				{
					if(effect == RBE_PHOTO) act = _("%sを撮影した。", "take a photo of %s.");
					else act = _("%sを攻撃した。", "hit %s.");
					touched = TRUE;
					break;
				}

			case RBM_TOUCH:
				{
					act = _("%sを触った。", "touch %s.");
					touched = TRUE;
					break;
				}

			case RBM_PUNCH:
				{
					act = _("%sを殴った。", "punch %s.");
					touched = TRUE;
					break;
				}

			case RBM_KICK:
				{
					act = _("%sを蹴った。", "kick %s.");
					touched = TRUE;
					break;
				}

			case RBM_CLAW:
				{
					act = _("%sをひっかいた。", "claw %s.");
					touched = TRUE;
					break;
				}

			case RBM_BITE:
				{
					act = _("%sを噛んだ。", "bite %s.");

					touched = TRUE;
					break;
				}

			case RBM_STING:
				{
					act = _("%sを刺した。", "stab %s.");
					touched = TRUE;
					break;
				}

			case RBM_SLASH:
				{
					act = _("%sを斬った。", "slash %s.");
					touched = TRUE;
					break;
				}

			case RBM_BUTT:
				{
					act = _("%sを角で突いた。", "headbutt %s.");
					touched = TRUE;
					break;
				}

			case RBM_CRUSH:
				{
					act = _("%sに体当りした。", "crush %s.");
					touched = TRUE;
					break;
				}

			case RBM_ENGULF:
				{
					act = _("%sを飲み込んだ。", "engulf %s.");
					touched = TRUE;
					break;
				}

			case RBM_CHARGE:
				{
					act = _("%sに請求書を押し付けた。", "give %s an invoice.");
					touched = TRUE;
					break;
				}

			case RBM_CRAWL:
				{
					act = _("%sの体の上を這い回った。", "crawl on %s.");
					touched = TRUE;
					break;
				}

			case RBM_DROOL:
				{
					act = _("%sによだれをたらした。", "drool on %s.");
					touched = FALSE;
					break;
				}

			case RBM_SPIT:
				{
					act = _("%sに唾を吐いた。", "spit on %s.");

					touched = FALSE;
					break;
				}

			case RBM_EXPLODE:
				{
					act = _("爆発した。", "explode.");

					explode = TRUE;
					touched = FALSE;
					break;
				}

			case RBM_GAZE:
				{
					act = _("%sをにらんだ。", "gaze at %s.");

					touched = FALSE;
					break;
				}

			case RBM_WAIL:
				{
					act = _("%sに泣きついた。", "wail at %s.");
					touched = FALSE;
					break;
				}

			case RBM_SPORE:
				{
					act = _("%sに胞子を飛ばした。", "release spores at %s.");
					touched = FALSE;
					break;
				}

			case RBM_DANCE:
				{
					act = _("%sの回りで踊った。", "dance around %s.");

					touched = FALSE;
					break;
				}

			case RBM_BEG:
				{
					act = _("%sに金をせがんだ。", "beg %s for money.");

					touched = FALSE;
					break;
				}

			case RBM_INSULT:
				{
					act = _("%sを侮辱した。", "insult %s.");
					touched = FALSE;
					break;
				}

			case RBM_MOAN:
				{
					act = _("%sにむかってうめいた。", "moan at %s.");

					touched = FALSE;
					break;
				}

			case RBM_SHOW:
				{
					act = _("%sにむかって歌った。", "sing at %s.");

					touched = FALSE;
					break;
				}
			case RBM_SPEAR:
				{
					act = _("%sを突いた。", "thrust at %s.");
					touched = TRUE;
					break;
				}
			case RBM_BOOK:
				{
					act = _("%sを分厚い本で殴った。", "bludgeon %s with a thick book.");
					touched = TRUE;
					break;
				}
			case RBM_MOFUMOFU:
				{
					act = _("%sを尻尾でモフモフした。", "envelop %s with fluffy tail.");
					touched = TRUE;
					break;
				}

			case RBM_SQUEEZE:
				{
					act = _("%sを締め付けた。", "squeeze %s.");
					touched = TRUE;
					break;
				}
			case RBM_DRAG:
				{
					act = _("%sを引きずり込んだ。", "drag %s in.");

					touched = TRUE;
					break;
				}
			case RBM_INJECT:
				{
					act = _("%sに薬液を注射した。", "inject %s with chemicals.");

					touched = TRUE;
					break;
				}

			case RBM_SMILE:
				{
					act = _("%sに微笑みかけた。", "smile at %s.");

					touched = FALSE;
					break;
				}

			case RBM_HEADBUTT:
				{
					act = _("%sに頭突きした。", "headbutt %s.");

					touched = TRUE;
					break;
				}

			case RBM_FLASH:
				{
					act = _("%sの前で光った。", "let out a bright flash in front of %s.");
					touched = FALSE;

					break;
				}

			case RBM_HOWL:
				{
					act = _("%sに向けて吠えた。", "howl at %s.");
					touched = FALSE;

					break;
				}

			case RBM_WHISPER:
				{
					act = _("%sに何かを囁いた。", "whisper something to %s.");
					touched = TRUE;

					break;
				}

			case RBM_PRESS:
				{
					act = _("%sを押し潰した。", "press %s.");

					touched = TRUE;
					break;
				}

			case RBM_POINT:
				{
					act = _("%sを指差した。", "point at %s.");
					touched = FALSE;
					break;
				}

			case RBM_STRIKE:
				{
					act = _("%sを打ち据えた。", "strike %s.");
					touched = TRUE;
					break;
				}

			case RBM_PLAY:
				{
					act = _("%sの耳元で轟音を出した。", "play loud music at %s.");
					touched = FALSE;
					break;
				}
			case RBM_SMOKE:
			{
				act = _("%sへ煙を吹きかけた。", "blow smoke at %s.");
				touched = FALSE;
				break;
			}


			}

			/* Message */
			if (act)
			{
#ifdef JP
				if (do_silly_attack) act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];
#endif
				strfmt(temp, act, t_name);
				msg_format(_("%s", "You %s"),temp);
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/* Assume no effect */
			effect_type = BLOW_EFFECT_TYPE_NONE;

			pt = GF_MISSILE;

			/* Apply appropriate damage */
			switch (effect)
			{
			case 0:
				damage = pt = 0;
				break;

			case RBE_DR_MANA:
				if(player_gain_mana(randint1(damage)))	msg_print(_("魔力を奪った！", "You drain magical power!"));
				damage = pt = 0;
				break;

			case RBE_SUPERHURT:
				if ((randint1(rlev*2+250) > (ac+200)) || one_in_(13))
				{
					int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
					damage = MAX(damage, tmp_damage * 2);
					break;
				}

				/* Fall through */
			case RBE_BLEED:
			case RBE_HURT:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				break;

			case RBE_POISON:
			case RBE_DISEASE:
				pt = GF_POIS;
				break;

			case RBE_UN_BONUS:
			case RBE_UN_POWER:
				pt = GF_DISENCHANT;
				break;

			case RBE_EAT_ITEM:
			case RBE_EAT_GOLD:
				if(one_in_(7))
				{
					int money = randint1(tr_ptr->level * tr_ptr->level);
					p_ptr->au += money;
					msg_print(_("金を盗んだ！", "You steal some money!"));
					p_ptr->redraw |= PR_GOLD;
				}
				if (one_in_(2)) blinked = TRUE;
				break;

			case RBE_EAT_FOOD:
			case RBE_EAT_LITE:
				{
					int new_food = MIN((PY_FOOD_FULL-1),p_ptr->food + 1000);
					if(p_ptr->food < PY_FOOD_FULL)set_food(new_food);
				}
				break;
			case RBE_BLIND:
				break;
			case RBE_LOSE_STR:
			case RBE_LOSE_DEX:
				effect_type = BLOW_EFFECT_TYPE_DEC_ATK;
				break;

			case RBE_LOSE_INT:
			case RBE_LOSE_WIS:
				effect_type = BLOW_EFFECT_TYPE_DEC_MAG;
				break;
			case RBE_LOSE_CON:
			case RBE_LOSE_CHR:
				effect_type = BLOW_EFFECT_TYPE_DEC_DEF;
				break;

			case RBE_LOSE_ALL:
				effect_type = BLOW_EFFECT_TYPE_DEC_ALL;
				break;

			case RBE_ACID:
				pt = GF_ACID;
				break;

			case RBE_ELEC:
				pt = GF_ELEC;
				break;

			case RBE_FIRE:
				pt = GF_FIRE;
				break;

			case RBE_COLD:
				pt = GF_COLD;
				break;

			case RBE_CONFUSE: case RBE_INSANITY:
				pt = GF_CONFUSION;
				break;

			case RBE_TERRIFY:
				effect_type = BLOW_EFFECT_TYPE_FEAR;
				break;

			case RBE_PARALYZE:
				effect_type = BLOW_EFFECT_TYPE_SLEEP;
				break;

			case RBE_SHATTER:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				if (damage > 23) earthquake_aux(py, px, 8, 0);
				break;

			case RBE_EXP_10:
			case RBE_EXP_20:
			case RBE_EXP_40:
			case RBE_EXP_80:
				pt = GF_NETHER;
				break;

			case RBE_TIME:
				pt = GF_TIME;
				break;
				///mod131231 吸収打撃は対モンスターでは属性が違うがまあ大勢に影響はなかろう
			case RBE_EXP_VAMP: case RBE_MELT:
				pt = GF_OLD_DRAIN;
				effect_type = BLOW_EFFECT_TYPE_HEAL;
				break;
				//v1.1.32 空腹打撃追加
			case RBE_HUNGER:
				pt = GF_OLD_DRAIN;
				effect_type = BLOW_EFFECT_TYPE_FOOD;
				break;


			case RBE_INERTIA:
				pt = GF_INACT;
				break;
			case RBE_STUN:
				pt = GF_SOUND;
				break;

			case RBE_CHAOS: case RBE_MUTATE:
				pt = GF_CHAOS;
				break;
			case RBE_ELEMENT:
				if(one_in_(4))pt = GF_FIRE;
				else if(one_in_(3))pt = GF_ACID;
				else if(one_in_(2))pt = GF_ELEC;
				else pt = GF_COLD;
				break;
			case RBE_SMITE:
				pt = GF_HOLY_FIRE;
				break;
			case RBE_DROWN:
				pt = GF_WATER;
				break;
			case RBE_KILL: case RBE_CURSE:
				pt = GF_NETHER;
				break;

			case RBE_PHOTO:
				pt = GF_PHOTO;

				if (one_in_(2)) blinked = TRUE;
				break;
			default:
				pt = 0;
				break;
			}

			if (pt)
			{
				bool flag_living;

				int effect_power;

				if (r_ptr->flags2 & RF2_POWERFUL)
					effect_power = r_ptr->level * 3;
				else
					effect_power = r_ptr->level * 2;

				flag_living = monster_living(tr_ptr);

				/* Do damage if not exploding */
				if (!explode)
				{
					project(0, 0, t_ptr->fy, t_ptr->fx,	damage, pt, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
				}

				switch (effect_type)
				{
				case BLOW_EFFECT_TYPE_FEAR:
					if (!t_ptr->r_idx) break; //v1.1.49 モンスターを倒したら何もしない
					project(0, 0, t_ptr->fy, t_ptr->fx,	damage, GF_TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_SLEEP:
					if (!t_ptr->r_idx) break; //v1.1.49 モンスターを倒したら何もしない
					project(0, 0, t_ptr->fy, t_ptr->fx,	r_ptr->level, GF_OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

					//v1.1.94
				case BLOW_EFFECT_TYPE_DEC_ATK:
					project(0, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_ATK, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_DEF:
					project(0, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_DEF, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_MAG:
					project(0, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_MAG, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;
				case BLOW_EFFECT_TYPE_DEC_ALL:
					project(0, 0, t_ptr->fy, t_ptr->fx, effect_power, GF_DEC_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_HEAL:
					if ((flag_living) && (damage > 2))
					{

						if (p_ptr->chp < p_ptr->mhp) msg_print(_("敵の体力を吸収した！",
                                                                "Life force drains from the enemy's body!"));
						hp_player(damroll(4, damage / 6));

					}
					break;
					//v1.1.32追加　空腹打撃
				case BLOW_EFFECT_TYPE_FOOD:
					if ((flag_living) && p_ptr->food < PY_FOOD_FULL)
					{

						msg_print(_("敵から栄養を吸収した！", "Satiation drains from the enemy's body!"));
						set_food(p_ptr->food + 3000);
					}
					break;

				}

				if (touched)
				{
					touch_zap_player(t_ptr);
				}
			}

			if(explode)
			{
				if(!pt) pt = GF_FIRE;
				project(0, 3, py, px, damage, pt, ( PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
				take_hit(DAMAGE_LOSELIFE,p_ptr->mhp+1,_("自爆", "self-detonation"),-1);
				return TRUE;

			}

		}

		/* Monster missed player */
		//v1.1.54 攻撃をかわされたときは全部起こしてメッセージ出すようにしておく
		else
		{
			/* Wake it up */
			(void)set_monster_csleep(t_idx, 0);
			msg_format(_("ミス！%^sにかわされた。", "You miss %s."), t_name);

		}
	}


	/* Blink away */
	if (blinked && !p_ptr->is_dead)
	{

		if(p_ptr->anti_tele)
		{
			msg_print(_("あなたは笑って逃げ...ようとして失敗した。",
                        "You flee laughing...? But you fail."));
		}
		else
		{
			msg_print(_("あなたは笑って逃げた！", "You flee laughing!"));
			teleport_player(50,0L);
		}
	}

	return TRUE;
}



static bool check_hp_for_feat_destruction(feature_type *f_ptr, monster_type *m_ptr)
{
	return !have_flag(f_ptr->flags, FF_GLASS) ||
	       (r_info[m_ptr->r_idx].flags2 & RF2_STUPID) ||
	       (m_ptr->hp >= MAX(m_ptr->maxhp / 3, 200));
}



/*:::RF1_AISATSUフラグが立っている敵は＠との初回対面時に特別な行動をとる。モンスターが行動完了するときTRUE*/
bool mon_aisatsu(int m_idx)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	bool finish_process = FALSE;

	//フラグのない敵は何もしない
	if(!(r_ptr->flags1 & RF1_AISATSU)) return FALSE;
	//既にアイサツが完了していれば何もしない
	if(m_ptr->mflag & MFLAG_AISATSU_DONE) return FALSE;
	//視界外のときは何もしない
	if(!los(m_ptr->fy, m_ptr->fx, py, px)) return FALSE;

	//ニンジャスレイヤー　アイサツ
	if(m_ptr->r_idx == MON_NINJA_SLAYER)
	{
#ifdef JP
		msg_format("「ドーモ、%s=サン。ニンジャスレイヤーです。」",player_name);
		if(p_ptr->pclass == CLASS_NINJA || p_ptr->prace == RACE_NINJA)
			msg_format("「・・ニンジャ殺すべし。慈悲はない。」");
		finish_process = TRUE;
#endif // JP
	}
	else
	{
		msg_format(_("ERROR:IDX(%d)の敵の初回敵対行動が登録されていない",
                    "ERROR: Greeting action not registered for enemy IDX (%d)"),m_ptr->r_idx);
	}

	//アイサツ完了フラグ
	m_ptr->mflag |= MFLAG_AISATSU_DONE;
	return (finish_process);

}


/*
 * Process a monster
 *
 * The monster is known to be within 100 grids of the player
 *
 * In several cases, we directly update the monster lore
 *
 * Note that a monster is only allowed to "reproduce" if there
 * are a limited number of "reproducing" monsters on the current
 * level.  This should prevent the level from being "swamped" by
 * reproducing monsters.  It also allows a large mass of mice to
 * prevent a louse from multiplying, but this is a small price to
 * pay for a simple multiplication method.
 *
 * XXX Monster fear is slightly odd, in particular, monsters will
 * fixate on opening a door even if they cannot open it.  Actually,
 * the same thing happens to normal monsters when they hit a door
 *
 * XXX XXX XXX In addition, monsters which *cannot* open or bash
 * down a door will still stand there trying to open it...
 *
 * XXX Technically, need to check for monster in the way
 * combined with that monster being in a wall (or door?)
 *
 * A "direction" of "5" means "pick a random direction".
 */
/*:::モンスター一体の行動処理*/
/*:::増殖モンスターの最大数は階ごとに決まっている？*/
/*:::恐怖したモンスターはドアを開けられない種族でもドアを開けるようになる？ドアを打ち破るしかできない種族でもドアを開けるようになる？よくわからん*/

/*:::恐怖したモンスターの挙動を少し変えたい。＠と反対側の適当なグリッドをターゲットに指定したらいいような気がするが*/

static void process_monster(int m_idx)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];
	monster_race    *ap_r_ptr = &r_info[m_ptr->ap_r_idx];

	int             i, d, oy, ox, ny, nx;

	int             mm[8];

	cave_type       *c_ptr;
	feature_type    *f_ptr;

	monster_type    *y_ptr;

	bool            do_turn;
	bool            do_move;
	bool            do_view;
	bool            must_alter_to_move;

	bool            did_open_door;
	bool            did_bash_door;
	bool            did_take_item;
	bool            did_kill_item;
	bool            did_move_body;
	bool            did_pass_wall;
	bool            did_kill_wall;
	bool            gets_angry = FALSE;
	bool            can_cross;
	bool            aware = TRUE;
	bool			kunekune = FALSE;

	bool            fear;

	bool            is_riding_mon = (m_idx == p_ptr->riding);

	bool            see_m = is_seen(m_ptr);
	bool			flag_never_move = FALSE;

	int				card_num_jellyfish = 0;

	int alcohol_rank = 0;

	//v1.1.45 モンスター魔法使用率をこの変数で管理することにした。基本r_ptr->freq_spellと同じだが里乃のバックダンスでボーナスがつく
	int monster_spell_freq;

	monster_spell_freq = r_ptr->freq_spell;

	//泥酔度がMAXHPの25%を超えるごとにランク1加算
	if (MON_DRUNK(m_ptr)) alcohol_rank = MON_DRUNK(m_ptr) * 4 / MAX(m_ptr->maxhp,1);

	//v1.1.94 「すでに行動遅延を受けているフラグ」を削除する
	m_ptr->mflag &= ~(MFLAG_ALREADY_DELAYED);

	//v1.1.48 紫苑がスーパー貧乏神になっているとき、消滅、恐怖、行動妨害判定
	if (SUPER_SHION)
	{
		//v1.1.55 フロアからモンスターが逃亡して敵全滅条件が満たせなくなったのでクエスト内では敵が逃げないようにしとく
		//if (!(r_ptr->flags1 & RF1_QUESTOR) && randint1((r_ptr->level * 5 + m_ptr->cdis) < p_ptr->lev))
		if (!p_ptr->inside_quest && !(r_ptr->flags1 & RF1_QUESTOR) && randint1((r_ptr->level * 5 + m_ptr->cdis) < p_ptr->lev))
			{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sはこのフロアから逃亡した！", "%^s runs away from this floor!"),m_name);
			delete_monster_idx(m_idx);
			return;

		}
		else if (!(r_ptr->flags3 & RF3_NO_FEAR) && player_has_los_bold(m_ptr->fy, m_ptr->fx))
		{
			if (!MON_MONFEAR(m_ptr))set_monster_monfear(m_idx, 5);

			if (randint1(r_ptr->level * 2) < randint1(p_ptr->lev)) return;

		}


	}

	//里乃がバックダンスしているモンスターは魔法使用率にボーナス
	if (p_ptr->pclass == CLASS_SATONO && p_ptr->riding == m_idx && monster_spell_freq)
	{
		monster_spell_freq += p_ptr->stat_ind[A_CHR] + 3;

		if (monster_spell_freq > 100) monster_spell_freq = 100;

	}

	//v1.1.24 ボートなど騎乗専用モンスターは何もしない
	if(r_ptr->flags7 & RF7_ONLY_RIDING)
		return;

	if(r_ptr->flags1 & RF1_NEVER_MOVE) flag_never_move = TRUE;

	//2.0.2 遅効性の管狐弾
	//モンスターの行動ごとに一定のダメージを与える
	if (m_ptr->timed_shard_count)
	{
		int dam = 5 + p_ptr->lev / 2;
		bool dummy;

		m_ptr->timed_shard_count -= 1;

		mon_take_hit(m_idx, dam, &dummy, _("は管狐弾のダメージで倒れた。", "%^s is defeated by kuda-gitsune bullets."));

		if (!m_ptr->r_idx) return;

		if (!m_ptr->timed_shard_count)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sへの管狐弾の効果がなくなったようだ。", "%^s is no longer affected by kuda-gitsune bullets."), m_name);
		}
	}




	/*::: -Hack- 小町などの移動抑止特技の特殊処理 効果持続中は該当モンスターをNEVER_MOVE扱いにするが破られることもある*/
	//v1.1.95 MTIMED2_NO_MOVEの処理に置き換えた。この技をかけた状態でバージョンアップするといきなり効果が切れるが多分そんな人はいないだろう。
	if(MON_NO_MOVE(m_ptr))
	{
		bool smash = FALSE;
		int power = p_ptr->lev + MON_NO_MOVE(m_ptr) * 2;
		char m_name[80];

		monster_desc(m_name, m_ptr, 0);

		flag_never_move = TRUE;

		if(r_ptr->flags2 & RF2_GIGANTIC && power < randint1(r_ptr->level*3/2)) smash = TRUE;
		if(r_ptr->flags2 & RF2_POWERFUL && power < randint1(r_ptr->level)) smash = TRUE;
		if(r_ptr->flags1 & (RF1_UNIQUE) && (power < randint1(r_ptr->level) || one_in_(6))) smash = TRUE;

		if (MON_CSLEEP(m_ptr)) smash = FALSE;

		if(smash)
		{
			msg_format(_("%sは束縛を振り払った！", "%^s breaks free of restraints!"),m_name);
			set_monster_timed_status_add(MTIMED2_NO_MOVE, m_idx, 0);
			if (one_in_(2)) flag_never_move = FALSE;
		}
		else if(one_in_(7))
		{
			msg_format(_("%sは動けない！", "%^s can't move!"), m_name);
			return;

		}
	}

	//v1.1.47 ミスティア特技「ヒューマンケージ」による人間フラグモンスターの移動禁止処理
	if (p_ptr->pclass == CLASS_MYSTIA && music_singing(MUSIC_NEW_MYSTIA_HUMAN_CAGE) && (r_ptr->flags3 & (RF3_HUMAN)))
	{
		flag_never_move = TRUE;
	}

	///mod150412 ヤマメ特殊処理 蜘蛛の巣の上にいる敵は移動に失敗したり巣を壊す
	//v1.1.47 少し判定強化
	if(cave_have_flag_bold(m_ptr->fy,m_ptr->fx,FF_SPIDER_NEST_1) && !MON_CSLEEP(m_ptr))
	{
		int power;
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);

		power = r_ptr->level;
		if(r_ptr->flags2 & RF2_GIGANTIC) power *= 2;
		if(r_ptr->flags2 & RF2_POWERFUL) power *= 2;
		if(r_ptr->flags1 & RF1_UNIQUE || r_ptr->flags7 & RF7_UNIQUE2) power *= 2;

		if (r_ptr->flags2 & (RF2_KILL_WALL)) power *= 3;

		if(power<5) power = 5;


		if(randint1(power) > p_ptr->lev * 2 )
		{
			if(see_m) msg_format(_("%sは巣を引きちぎった！", "%^s tears away the webs!"),m_name);
			cave_set_feat(m_ptr->fy, m_ptr->fx, feat_floor);

		}
		else if(r_ptr->flags2 & (RF2_PASS_WALL))
		{
			;//壁抜けする敵は蜘蛛の巣も抜けてくる
		}
		else if(randint1(power*2) < p_ptr->lev)
		{
			flag_never_move = TRUE;
			if(one_in_(3))
			{
				if(see_m) msg_format(_("%sは巣の中でもがいている。", "%^s is still stuck in the webs."),m_name);
				return;
			}
		}

	}


	//v1.1.35 ネムノ特殊処理　ネムノが縄張りにおらずモンスターが縄張りにいるとき、レベルに応じた判定で縄張りを奪われる
	if(p_ptr->pclass == CLASS_NEMUNO && is_hostile(m_ptr) && !MON_CSLEEP(m_ptr) && !MON_MONFEAR(m_ptr))
	{
		if(!IS_NEMUNO_IN_SANCTUARY && cave[m_ptr->fy][m_ptr->fx].cave_xtra_flag & CAVE_XTRA_FLAG_NEMUNO)
		{
			if(p_ptr->lev < (randint1(r_ptr->level ) + randint1(20)))
			{
				erase_nemuno_sanctuary(TRUE,TRUE);
			}
		}
	}


	//紫「静と動の均衡」
	if(p_ptr->pclass == CLASS_YUKARI && p_ptr->tim_general[0])
	{
		if(m_ptr->cdis <= (2 + p_ptr->lev / 16)) flag_never_move = TRUE;
	}

	//v1.1.12 フェムトファイバー
	if(m_ptr->mflag & MFLAG_FEMTO_FIBER)
	{
		int chance = 20 + r_ptr->level / 3;
		flag_never_move = TRUE;

		if(r_ptr->flags2 & RF2_STUPID) chance /= 8;
		else if(!(r_ptr->flags2 & RF2_SMART)) chance /= 2;

		if (MON_CSLEEP(m_ptr)) chance = 0;

		if(r_ptr->flags2 & RF2_POWERFUL) chance += 20;

		//v1.1.62 判定ミスってたので修正
		if(randint1(100) > chance)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);
			if(see_m) msg_format(_("フェムトファイバーの組紐が%sを締め上げた！",
                                    "%^s is tied up with femtofiber bonds!"),m_name);
			return;
		}
	}


	/*:::このモンスターに乗馬中だが乗馬向きのモンスターでないとき、必ず振り落される*/
	//v1.1.41 ただし舞と里乃は振り落とされないようrakuba()内に記述追加している
	if (is_riding_mon && !(r_ptr->flags7 & RF7_RIDING))
	{
		if (rakuba(0, TRUE))
		{
#ifdef JP
			msg_print("地面に落とされた。");
#else
			char m_name[80];
			monster_desc(m_name, &m_list[p_ptr->riding], 0);
			msg_format("You have fallen from %s.", m_name);
#endif
		}
	}
	/*:::カメレオンフラグを持つモンスターの変身処理*/
	if ((m_ptr->mflag2 & MFLAG2_CHAMELEON) && one_in_(13) && !MON_CSLEEP(m_ptr))
	{
		choose_new_monster(m_idx, FALSE, 0);
		r_ptr = &r_info[m_ptr->r_idx];
	}

	///mod140817 感知範囲外の敵は＠を認識しない。従来は感知範囲外の敵はそもそも行動順が来なかったが変更したのでここで処理。
	if (!is_pet(m_ptr) && m_ptr->cdis > r_ptr->aaf) aware = FALSE;

	/*:::超隠密処理*/
	/* Players hidden in shadow are almost imperceptable. -LM- */
	else if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		int tmp = p_ptr->lev*6+(p_ptr->skill_stl+10)*4;
		if (p_ptr->monlite) tmp /= 3;
		if (p_ptr->cursed & TRC_AGGRAVATE) tmp /= 2;
		if (r_ptr->level > (p_ptr->lev*p_ptr->lev/20+10)) tmp /= 3;
		/* Low-level monsters will find it difficult to locate the player. */
		if (randint0(tmp) > (r_ptr->level+20)) aware = FALSE;
	}
	//ぬえ変身時
	else if ((p_ptr->mimic_form == MIMIC_NUE) && p_ptr->cur_lite <= 0 && !(cave[py][px].info & CAVE_GLOW))
	{
		int tmp = p_ptr->lev * 4 + (p_ptr->skill_stl) * 4;
		if (p_ptr->monlite) tmp /= 3;
		if (r_ptr->flags2 & RF2_SMART) tmp /= 2;
		if (p_ptr->cursed & TRC_AGGRAVATE) tmp /= 2;
		if (r_ptr->level > (p_ptr->lev*p_ptr->lev/20+10)) tmp /= 3;
		/* Low-level monsters will find it difficult to locate the player. */
		if (randint0(tmp) > (r_ptr->level+20)) aware = FALSE;
	}
	//ミスティア夜雀の歌判定
	else if(p_ptr->pclass == CLASS_MYSTIA && music_singing(MUSIC_NEW_MYSTIA_YAZYAKU))
	{
		if(check_mon_blind(m_idx)) aware = FALSE;
	}
	///mod140215 一時超隠密　忍者ステルスと少し違う判定にする
	///mod160312 光学迷彩処理を整理した
	else if (p_ptr->tim_superstealth)
	{
		if (p_ptr->superstealth_type == SUPERSTEALTH_TYPE_OPTICAL)
		{
			if (check_mon_blind(m_idx))
			{
				if (aware && cheat_xtra) msg_print(_("testmsg:光学迷彩成功", "testmsg: Optical camouflage success"));
				aware = FALSE;
			}
		}
		//v1.1.88　真・木隠れの技術 森地形の中のみ有効 少しだけ判定強くしとく
		else if (p_ptr->superstealth_type == SUPERSTEALTH_TYPE_FOREST)
		{
			if (cave_have_flag_bold((py), (px), FF_TREE))
			{
				int tmp = p_ptr->lev * 5 + (p_ptr->skill_stl + 10) * 5;
				if (r_ptr->flags2 & RF2_SMART) tmp /= 2;
				if (r_ptr->flags2 & RF2_EMPTY_MIND) tmp /= 2;
				if (p_ptr->cursed & TRC_AGGRAVATE) tmp /= 3;
				if (r_ptr->level > (p_ptr->lev * p_ptr->lev / 20 + 10)) tmp /= 3;
				/* Low-level monsters will find it difficult to locate the player. */
				if (randint0(tmp) > (r_ptr->level + 20))
				{
					if (cheat_xtra) msg_print(_("testmsg:木隠れ成功", "testmsg: Hiding in trees success"));
					aware = FALSE;
				}
				else
				{
					if (cheat_xtra) msg_print(_("testmsg:木隠れ失敗", "testmsg: Hiding in trees failed"));
				}
			}
		}
		else
		{
			int tmp = p_ptr->lev*5+(p_ptr->skill_stl+10)*4;
	 		if (r_ptr->flags2 & RF2_SMART) tmp /= 2;
	 		if (r_ptr->flags2 & RF2_EMPTY_MIND) tmp /= 2;
			if (p_ptr->cursed & TRC_AGGRAVATE) tmp /= 5;
			if (r_ptr->level > (p_ptr->lev * p_ptr->lev / 20 + 10)) tmp /= 3;
			/* Low-level monsters will find it difficult to locate the player. */
			if (randint0(tmp) > (r_ptr->level+20)) aware = FALSE;
		}


	}
	///mod140511 こいし超隠密判定
	else if(p_ptr->pclass == CLASS_KOISHI)
	{
		int tmp = p_ptr->lev * 6;
		if(r_ptr->flags2 & RF2_EMPTY_MIND) tmp = 0;
		else if(r_ptr->flags2 & RF2_WEIRD_MIND) tmp /= 4;
		else if(r_ptr->flags2 & RF2_ELDRITCH_HORROR) tmp /= 2;
		else if(!monster_living(r_ptr)) tmp = tmp * 2 / 3;
		if(randint0(tmp) > r_ptr->level) aware = FALSE;
	}
	//巨大な敵から見つかりにくい妖精
	else if(CHECK_FAIRY_TYPE == 7)
	{
 		if (r_ptr->flags2 & RF2_GIGANTIC && one_in_(7)) aware = FALSE;

	}
	//針妙丸小型化中
	else if(p_ptr->mimic_form == MIMIC_SHINMYOU)
	{
 		if (r_ptr->flags2 & RF2_GIGANTIC && one_in_(2)) aware = FALSE;
	}
	//v1.1.47 ルーミアが闇生成してるとき近くにいるモンスター
	else if (p_ptr->pclass == CLASS_RUMIA && p_ptr->tim_general[0] && m_ptr->cdis < 2)
	{
		if (check_mon_blind(m_idx)) aware = FALSE;
	}

	//v2.0.9 美宵記憶消去
	if (p_ptr->pclass == CLASS_MIYOI && p_ptr->tim_general[0])
	{
		if (!(r_ptr->flags2 & (RF2_WEIRD_MIND | RF2_EMPTY_MIND)) //無精神には効かない
			&& !(r_ptr->flags3 & RF3_NONLIVING) //無生物には効かない
			&& !player_can_see_bold(m_ptr->fy,m_ptr->fx) //視界内には効かない
			)
		{
			aware = FALSE;
		}
	}


	//v2.0.1 アビリティカードのひらり布
	if (p_ptr->tim_hirari_nuno)
	{
		aware = FALSE;
	}
	//エイボンの霧の車輪
	if(p_ptr->special_defense & (SD_EIBON_WHEEL))
	{
		if(p_ptr->special_defense & (SD_STATUE_FORM))
		{
			aware = FALSE;
		}
		else
		{
			if(!(r_ptr->flags3 & RF3_GEN_MASK)) aware = FALSE;
		}
	}


	///mod150327 隠密が極めて高い時敵が時々＠を見失うようにしてみる 最大値は30
	/* ..と思ったが一時隠密系の特技や魔法が強力になりすぎるのでやっぱやめた
	if(aware && p_ptr->skill_stl > 17)
	{
		if(randint1(100) < (p_ptr->skill_stl)) aware = FALSE;
	}
	*/


	//妖夢の分身との距離が離れすぎると分身が消える
	if(m_ptr->r_idx == MON_YOUMU && (m_ptr->mflag & MFLAG_EPHEMERA) && m_ptr->cdis > MAX_SIGHT)
	{
		msg_print(_("あなたの半霊は霊体に戻って帰ってきた！", "You ghost half returns to its original ghost body!"));
		delete_monster_idx(m_idx);
		return;
	}
	//パルスィ特殊召喚
	else if(p_ptr->pclass == CLASS_PARSEE && m_ptr->r_idx == MON_GREEN_EYED && !m_list[p_ptr->magic_num1[0]].r_idx)
	{
		msg_print(_("緑目の怪物は消えた。", "The green-eyed monster disappear."));
		delete_monster_idx(m_idx);
		return;
	}
	else if(p_ptr->pclass == CLASS_PARSEE && m_ptr->r_idx == MON_PARSEE && (m_ptr->mflag & MFLAG_EPHEMERA) && m_ptr->cdis > 4)
	{
		msg_print(_("あなたの幻影は消えた。", "Your phantom disappears."));
		delete_monster_idx(m_idx);
		return;
	}
	else if(p_ptr->pclass == CLASS_EIKI && (m_ptr->mflag & MFLAG_EPHEMERA)
		&& (!m_list[p_ptr->magic_num1[0]].r_idx || !los(m_ptr->fy,m_ptr->fx,py,px)))
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);
		msg_format(_("%sの幻影は消えた。", "The phantom of %s disappears."),m_name);
		delete_monster_idx(m_idx);
		return;
	}

	//v1.1.51 新アリーナ内では騎乗(ダンス)中でない味方モンスターは即座に消滅することにする
	if (p_ptr->inside_arena && !is_hostile(m_ptr) && p_ptr->riding != m_idx)
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);
		msg_format(_("%sは消えた。", "%^s disappears."), m_name);
		delete_monster_idx(m_idx);
		return;

	}

	/* Are there its parent? */
	/*:::parent_m_idxが設定されていてそのidxのモンスターが居ないとき、このモンスターも消滅*/
	if (m_ptr->parent_m_idx && !m_list[m_ptr->parent_m_idx].r_idx)
	{
		/* Its parent have gone, it also goes away. */

		if (see_m)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

#ifdef JP
			msg_format("%sは消え去った！", m_name);
#else
			msg_format("%^s disappears!", m_name);
#endif
		}

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			char m_name[80];

			monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
			do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_LOSE_PARENT, m_name);
		}

		/* Delete the monster */
		delete_monster_idx(m_idx);

		return;
	}
	/*:::芳香の小神霊吸収*/
	if(p_ptr->pclass == CLASS_YOSHIKA && (m_ptr->r_idx == S_SHINREI_B || m_ptr->r_idx == S_SHINREI_P || m_ptr->r_idx == S_SHINREI_G || m_ptr->r_idx == S_SHINREI_W) && (m_ptr->cdis < p_ptr->lev / 7) && projectable(m_ptr->fy, m_ptr->fx, py, px) && one_in_(4))
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);
		msg_format(_("あなたは%sを吸収した！", "You absorb %s!"),m_name);
		hp_player(p_ptr->mhp / (10+randint0(11)));
		delete_monster_idx(m_idx);
		return;
	}
	/* Quantum monsters are odd */
	/*:::量子フラグのモンスター*/
	if (r_ptr->flags2 & (RF2_QUANTUM))
	{
		/* Sometimes skip move */
		if (!randint0(2)) return;

		/* Sometimes die */
		if (!randint0((m_idx % 100) + 10) && !(r_ptr->flags1 & RF1_QUESTOR))
		{
			bool sad = FALSE;

			if (is_pet(m_ptr) && !(m_ptr->ml))
				sad = TRUE;

			if (see_m)
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Oops */
#ifdef JP
				msg_format("%sは消え去った！", m_name);
#else
				msg_format("%^s disappears!", m_name);
#endif
			}

			/* Generate treasure, etc */
			monster_death(m_idx, FALSE);

			/* Delete the monster */
			delete_monster_idx(m_idx);

			if (sad)
			{
#ifdef JP
				msg_print("少しの間悲しい気分になった。");
#else
				msg_print("You feel sad for a moment.");
#endif
			}

			return;
		}
	}

	//v1.1.78 酔魔の力による幻影　＠の酔いが醒めていると消える
	if (r_ptr->flags2 & (RF2_PHANTOM))
	{
		int j;
		bool flag_exist = TRUE;

		//美宵がいるかチェック..しようかと思ったが別にその場にいなくても効果は出てたはず
		/*
		for (j = 1; j < m_max; j++)
		{
			monster_type *tmp_m_ptr = &m_list[j];
			if (tmp_m_ptr->r_idx == MON_MIYOI)
			{
				flag_exist = TRUE;
				break;
			}
		}
		*/
		//泥酔度チェック
		if (p_ptr->alcohol < DRANK_2)
		{
			flag_exist = FALSE;
		}
		//＠が倒れてると消えることにする
		//if (p_ptr->paralyzed || p_ptr->confused || (p_ptr->stun >= 100) || p_ptr->alcohol >= DRANK_4 && !(p_ptr->muta2 & MUT2_ALCOHOL))
		//{
		//	flag_exist = FALSE;
		//}

		if(!flag_exist)
		{

			char m_name[80];
			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sは消えた。", "%s disappears."), m_name);

			monster_death(m_idx, FALSE);
			delete_monster_idx(m_idx);
			return;
		}



	}


	///mod151118 この処理で爆発してもreturnしないのがバグの元っぽいので処理追加
	if (m_ptr->r_idx == MON_SHURYUUDAN)
	{
#ifdef JP
		mon_take_hit_mon(m_idx, 1, &fear, "は爆発して粉々になった。", m_idx);
#else
		mon_take_hit_mon(m_idx, 1, &fear, " explodes into tiny shreds.", m_idx);
#endif

		if(!m_ptr->r_idx) return;

	}
	/*:::ユニークのペットや友好が逃げ出す処理*/
	if ((is_pet(m_ptr) || is_friendly(m_ptr)) && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) && !p_ptr->inside_battle && !(m_ptr->mflag & MFLAG_EPHEMERA))
	{
		static int riding_pinch = 0;

		int unique_pet_escape_hp = m_ptr->maxhp / 3;

		if (p_ptr->pclass == CLASS_YACHIE) unique_pet_escape_hp = m_ptr->maxhp / 10;

		if (m_ptr->hp < unique_pet_escape_hp)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);

			if (is_riding_mon && riding_pinch < 2)
			{
#ifdef JP
				msg_format("%sはあなたの束縛から逃れようとしている。", m_name);
#else
				msg_format("%^s seems to be trying to escape from your restriction.", m_name);
#endif
				riding_pinch++;
				disturb(1, 1);
			}
			else
			{
				if (is_riding_mon)
				{
#ifdef JP
					if(CLASS_RIDING_BACKDANCE)
						msg_format("%sは背後のあなたを振り払った！", m_name);
					else
						msg_format("%sはあなたの束縛から脱出した。", m_name);

#else
					if(CLASS_RIDING_BACKDANCE)
						msg_format("%s manages to shake you off!", m_name);
					else
						msg_format("%^s succeeded to escape from your restriction!", m_name);
#endif
					if (rakuba(-1, FALSE))
					{
#ifdef JP
						msg_print("地面に落とされた。");
#else
						msg_print("You have fallen from riding pet.");
#endif
					}
				}

				if (see_m)
				{
					if ((r_ptr->flags2 & RF2_CAN_SPEAK) && (m_ptr->r_idx != MON_GRIP) && (m_ptr->r_idx != MON_WOLF) && (m_ptr->r_idx != MON_FANG) &&
					    player_has_los_bold(m_ptr->fy, m_ptr->fx) && projectable(m_ptr->fy, m_ptr->fx, py, px))
					{
#ifdef JP
						///sys msg 友好モンスターが逃げ出す処理　幻想ユニークはセリフ変える？
						//msg_format("%^s「ピンチだ！退却させてもらう！」", m_name);
#else
						//msg_format("%^s says 'It is the pinch! I will retreat'.", m_name);
#endif
					}
#ifdef JP
					msg_format("%^sはこのフロアから離脱した。", m_name);
					///msg131221
					//msg_format("%^sがテレポート・レベルの巻物を読んだ。", m_name);
					//msg_format("%^sが消え去った。", m_name);
#else
                    msg_format("%^s leaves this floor.", m_name);
#endif
				}

// この記述上の方のと被ってるように見えるが
				if (is_riding_mon && rakuba(-1, FALSE))
				{
#ifdef JP
					msg_print("地面に落とされた。");
#else
					msg_print("You have fallen from riding pet.");
#endif
				}
				/* Check for quest completion */
				check_quest_completion(m_ptr);

				delete_monster_idx(m_idx);

				return;
			}
		}
		else
		{
			/* Reset the counter */
			if (is_riding_mon) riding_pinch = 0;
		}
	}

	/* Handle "sleep" */
	/*:::＠の反感で起こす処理*/
	if (MON_CSLEEP(m_ptr))
	{
		///mod140302 仙人と天人が死神を起こす処理
		bool disturb = FALSE;
		/* Handle non-aggravation - Still sleeping */
		//if (!(p_ptr->cursed & TRC_AGGRAVATE)) return;
		if (p_ptr->cursed & TRC_AGGRAVATE) disturb = TRUE;
		if ((p_ptr->pclass == CLASS_NINJA || p_ptr->prace == RACE_NINJA) && (m_ptr->r_idx == MON_NINJA_SLAYER)) disturb = TRUE;
		if ((p_ptr->prace == RACE_SENNIN || p_ptr->prace == RACE_TENNIN)  && (p_ptr->pclass != CLASS_KASEN) && (r_ptr->flags7 & RF7_FROM_HELL)) disturb = TRUE;

		//正邪はお尋ね者
		if(p_ptr->pclass == CLASS_SEIJA && is_gen_unique(m_ptr->r_idx)) disturb = TRUE;

		//v1.1.52 菫子新性格もお尋ね者。ただし「夢は現実化する」クエストを打開したら解除
		if (is_special_seikaku(SEIKAKU_SPECIAL_SUMIREKO) && quest[QUEST_DREAMDWELLER].status != QUEST_STATUS_FINISHED && is_gen_unique(m_ptr->r_idx)) disturb = TRUE;

		if(!disturb) return;

		/* Handle aggravation */

		/* Reset sleep counter */
		(void)set_monster_csleep(m_idx, 0);

		/* Notice the "waking up" */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			/* Dump a message */
#ifdef JP
			msg_format("%^sが目を覚ました。", m_name);
#else
			msg_format("%^s wakes up.", m_name);
#endif
		}

		/* Hack -- Count the wakings */
		if (is_original_ap_and_seen(m_ptr) && (r_ptr->r_wake < MAX_UCHAR))
		{
			r_ptr->r_wake++;
		}
	}

	/* Handle "stun" */
	/*:::朦朧中は1/2で何もしない*/
	if (MON_STUNNED(m_ptr))
	{
		/* Sometimes skip move */
		if (one_in_(2)) return;
	}

	//泥酔度が高いと確率で何もしない
	if (alcohol_rank >= 2)
	{
		if (randint1(5) < alcohol_rank)
		{
			char m_name[80];

			/* Acquire the monster name */
			monster_desc(m_name, m_ptr, 0);

			if(one_in_(3))
				msg_format(_("%^sはぼーっとしている。", "%^s is in a drunken daze."), m_name);
			else if(one_in_(2))
				msg_format(_("%^sは酔拳の構えをとった！", "%^s takes the drunken fist stance!"), m_name);
			else
				msg_format(_("%^sはつんのめって転んだ！", "%^s stumbles and falls down!"), m_name);

		}

	}

	if (is_riding_mon)
	{
		p_ptr->update |= (PU_BONUS);
	}

	/* No one wants to be your friend if you're aggravating */
	if (is_friendly(m_ptr) && (p_ptr->cursed & TRC_AGGRAVATE))
		gets_angry = TRUE;

	///mod140302 仙人と天人が死神を怒らせる処理
	if (is_friendly(m_ptr) && (p_ptr->prace == RACE_SENNIN || p_ptr->prace == RACE_TENNIN) && (p_ptr->pclass != CLASS_KASEN) && (r_ptr->flags7 & RF7_FROM_HELL)) gets_angry = TRUE;

	if (is_friendly(m_ptr) && (p_ptr->pclass == CLASS_NINJA || p_ptr->prace == RACE_NINJA) && (m_ptr->r_idx == MON_NINJA_SLAYER))
		gets_angry = TRUE;
	//正邪はお尋ね者
	if(is_friendly(m_ptr) && p_ptr->pclass == CLASS_SEIJA && is_gen_unique(m_ptr->r_idx))	gets_angry = TRUE;

	//v1.1.52 菫子新性格もお尋ね者。ただし「夢は現実化する」クエストを打開したら解除
	if (is_friendly(m_ptr) && is_special_seikaku(SEIKAKU_SPECIAL_SUMIREKO) && quest[QUEST_DREAMDWELLER].status != QUEST_STATUS_FINISHED && is_gen_unique(m_ptr->r_idx)) gets_angry = TRUE;


	/* Paranoia... no pet uniques outside wizard mode -- TY */
	//v1.1.41 二童子は除く
	if (is_pet(m_ptr) &&
	    ((((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) &&
	      monster_has_hostile_align(NULL, 10, -10, r_ptr) && !CLASS_RIDING_BACKDANCE)
	     || (r_ptr->flagsr & RFR_RES_ALL)))
	{
		gets_angry = TRUE;
	}

	if (p_ptr->inside_battle) gets_angry = FALSE;
	if (m_ptr->r_idx == MON_MASTER_KAGUYA) gets_angry = FALSE;

	if (gets_angry)
	{
		if (is_pet(m_ptr) || see_m)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, is_pet(m_ptr) ? MD_ASSUME_VISIBLE : 0);
#ifdef JP
			msg_format("%^sは突然敵にまわった！", m_name);
#else
			msg_format("%^s suddenly becomes hostile!", m_name);
#endif
		}

		set_hostile(m_ptr);
	}


	//v1.1.58 このモンスターの専用BGMがあるなら鳴らすかどうかチェック
	if(see_m)check_music_mon_r_idx(m_idx);

	/*:::一部特殊モンスターが敵対状態でフロアで最初に＠を見た時の行動*/
	if(is_hostile(m_ptr) && mon_aisatsu(m_idx)) return;

	/* Get the origin */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	/*:::モンスターの分裂処理*/
	/* Attempt to "multiply" if able and allowed */
	///mod140817 ＠を認識していない場合あまり増えないことにした
	///mod150610 闘技場では増えないことにした
	if ((r_ptr->flags2 & RF2_MULTIPLY) && !p_ptr->inside_battle && (num_repro < MAX_REPRO) && (aware || one_in_(13)))
	{
		int k, y, x;

		/* Count the adjacent monsters */
		for (k = 0, y = oy - 1; y <= oy + 1; y++)
		{
			for (x = ox - 1; x <= ox + 1; x++)
			{
				/* Ignore locations off of edge */
				if (!in_bounds2(y, x)) continue;

				if (cave[y][x].m_idx) k++;
			}
		}

		/* Hex */
		/*:::呪術の増殖阻止*/
		if (multiply_barrier(m_idx)) k = 8;

		/* Hack -- multiply slower in crowded areas */
		if ((k < 4) && (!k || !randint0(k * MON_MULT_ADJ)))
		{
			/* Try to multiply */
			if (multiply_monster(m_idx, FALSE, (is_pet(m_ptr) ? PM_FORCE_PET : 0)))
			{
				/* Take note if visible */
				if (m_list[hack_m_idx_ii].ml && is_original_ap_and_seen(m_ptr))
				{
					r_ptr->r_flags2 |= (RF2_MULTIPLY);
				}

				/* Multiplying takes energy */
				return;
			}
		}
	}

	/*:::王蟲がモルドを呼ぶ特別行動*/
	///sys mon RF6_SPECIAL
	if (r_ptr->flags6 & RF6_SPECIAL)
	{
		/* Hack -- Ohmu scatters molds! */
		if (m_ptr->r_idx == MON_OHMU)
		{
			if (!p_ptr->inside_arena && !p_ptr->inside_battle)
			{
				if (r_ptr->freq_spell && (randint1(100) <= monster_spell_freq))
			//	if (r_ptr->freq_spell && (randint1(100) <= r_ptr->freq_spell)) v1.1.45 変更
				{
					int  k, count = 0;
					int  rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
					u32b p_mode = is_pet(m_ptr) ? PM_FORCE_PET : 0L;

					for (k = 0; k < 6; k++)
					{
						if (summon_specific(m_idx, m_ptr->fy, m_ptr->fx, rlev, SUMMON_BIZARRE1, (PM_ALLOW_GROUP | p_mode)))
						{
							if (m_list[hack_m_idx_ii].ml) count++;
						}
					}

					if (count && is_original_ap_and_seen(m_ptr)) r_ptr->r_flags6 |= (RF6_SPECIAL);
				}
			}
		}
	}



	/*:::Mega Hack - 赤蛮奇の頭や妖夢の分身が見たものは＠も認識する。ここに置くと移動時の感知がワンテンポ遅れるので後ろのほうにももう一つ置く*/
	if(p_ptr->pclass == CLASS_BANKI && (m_ptr->r_idx == MON_BANKI_HEAD_1 || m_ptr->r_idx == MON_BANKI_HEAD_2)
		|| p_ptr->pclass == CLASS_YOUMU && m_ptr->r_idx == MON_YOUMU && (m_ptr->mflag & MFLAG_EPHEMERA))
	{
		for (i = m_max - 1; i >= 1; i--)
		{
			monster_type *m_ptr_tmp = &m_list[i];
			if (!m_ptr_tmp->r_idx) continue;
			if(i == m_idx || los(m_ptr->fy,m_ptr->fx,m_ptr_tmp->fy,m_ptr_tmp->fx)) m_ptr_tmp->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
		}
	}

	//八尺様特殊処理
	//v2.0.3 騎乗中(バックダンスや寄生などの特殊騎乗)のときテレポートをしないようにした
	if(m_ptr->r_idx == MON_HASSYAKUSAMA && p_ptr->riding != m_idx)
	{
		if(hassyakusama_target_idx && !m_list[hassyakusama_target_idx].r_idx) hassyakusama_target_idx = 0;
		//ターゲットがいない場合探す
		if(!hassyakusama_target_idx)
		{
			for (i = m_max - 1; i >= 1; i--)
			{
				monster_race *r_ptr_tmp;
				monster_type *m_ptr_tmp = &m_list[i];
				if (!m_ptr_tmp->r_idx) continue;
				r_ptr_tmp = &r_info[m_ptr_tmp->r_idx];

				if(!(r_ptr_tmp->flags1 & RF1_MALE) || !(r_ptr_tmp->flags3 & (RF3_HUMAN | RF3_DEMIHUMAN))) continue;
				if(!are_enemies(m_ptr_tmp,m_ptr)) continue;
				if(distance(m_ptr_tmp->fy,m_ptr_tmp->fx,m_ptr->fy,m_ptr->fx) > r_ptr->aaf) continue;

				hassyakusama_target_idx = i;
//なんか妙なボルト撃ってる。処理が足りてない？
				//ターゲットと離れているとテレポート処理
				if(distance(m_ptr_tmp->fy,m_ptr_tmp->fx,m_ptr->fy,m_ptr->fx) > 2)
				{
					if(teleport_barrier(m_idx)) break;
					teleport_monster_to(m_idx,m_ptr_tmp->fy,m_ptr_tmp->fx, 100, 0L);

					if(is_seen(m_ptr_tmp))
					{
						char m_name[80];
						monster_desc(m_name, m_ptr_tmp, 0);
						msg_format(_("『八尺さま』は%sを追いかけて行った。",
                                    "Hasshaku-sama pursues %s."),m_name);
					}
					else if(is_seen(m_ptr))
						msg_format(_("『八尺さま』は瞬時に消えた。",
                                    "Hasshaku-sama blinks away."));

					return;
				}
				break;
			}
		}
	}


	/*:::サイバーデーモンの足音、モンスターのセリフ*/
	///sys mon モンスターが喋ったり足音を立てる 変更予定
	///mod150610 闘技場でモンスターが喋ることにした
	//if (!p_ptr->inside_battle)
	{
		/* Hack! "Cyber" monster makes noise... */
		if (m_ptr->ap_r_idx == MON_CYBER &&
		    one_in_(CYBERNOISE) &&
		    !m_ptr->ml && (m_ptr->cdis <= MAX_SIGHT))
		{
			if (disturb_minor) disturb(FALSE, FALSE);
#ifdef JP
			msg_print("重厚な足音が聞こえた。");
#else
			msg_print("You hear heavy steps.");
#endif
		}

		/* Some monsters can speak */
		///幻想ユニーク判定追加 RF2_CAN_SPEAKは必要ない
		//タンオブウルフ、静寂、二ッ岩家の裁きなどのときは喋らせない
		if ((ap_r_ptr->flags2 & RF2_CAN_SPEAK || is_gen_unique(m_ptr->ap_r_idx))
			&& aware &&
		    one_in_(SPEAK_CHANCE) &&
			!(m_ptr->mflag & MFLAG_EPHEMERA) &&
		   ( player_has_los_bold(oy, ox) && projectable(oy, ox, py, px) || p_ptr->inside_battle) &&
		    !((p_ptr->pclass == CLASS_LUNAR || p_ptr->pclass == CLASS_3_FAIRIES) && (p_ptr->tim_general[0] && m_ptr->cdis <= p_ptr->lev / 5 || p_ptr->tim_general[1])) &&
			!(m_ptr->mflag & MFLAG_NO_SPELL) &&
			!(p_ptr->pclass == CLASS_MAMIZOU && p_ptr->magic_num1[0] == m_idx) &&
			!p_ptr->silent_floor)
		{
			char m_name[80];
			char monmessage[1024];
			cptr filename;

			/* Acquire the monster name/poss */
			if (m_ptr->ml)
				monster_desc(m_name, m_ptr, 0);
			else
#ifdef JP
				strcpy(m_name, "それ");
#else
				strcpy(m_name, "It");
#endif

			/* Select the file for monster quotes */

			///mod140505 幻想ユニークのセリフは別ファイルにしておく
			//v1.1.84 RF2_TROPHY(反獄王に取り憑かれている)のときは通常セリフを喋る
			if(is_gen_unique(m_ptr->ap_r_idx) && !(r_ptr->flags2 & RF2_TROPHY))
			{
				if (MON_MONFEAR(m_ptr))
					filename = _("monfear_gen.txt", "monfear_gen_e.txt");
				else if((is_pet(m_ptr) || is_friendly(m_ptr)) && !p_ptr->inside_battle)
					filename = _("monfrien_gen.txt", "monfrien_gen_e.txt");
				else
					filename = _("monspeak_gen.txt", "monspeak_gen_e.txt");
			}
			else if (MON_MONFEAR(m_ptr))
#ifdef JP
				filename = "monfear_j.txt";
#else
				filename = "monfear.txt";
#endif
			else if (is_pet(m_ptr))
#ifdef JP
				filename = "monpet_j.txt";
#else
				filename = "monpet.txt";
#endif
			else if (is_friendly(m_ptr))
#ifdef JP
				filename = "monfrien_j.txt";
#else
				filename = "monfrien.txt";
#endif
			else
#ifdef JP
				filename = "monspeak_j.txt";
#else
				filename = "monspeak.txt";
#endif
			/* Get the monster line */
			if (get_rnd_line(filename, m_ptr->ap_r_idx, monmessage) == 0)
			{
				/* Say something */
#ifdef JP
msg_format("%^s%s", m_name, monmessage);
#else
				msg_format("%^s %s", m_name, monmessage);
#endif

			}
		}
	}

	//v1.1.19 くねくねがモンスターの行動を阻害する処理
	for (i = m_max - 1; i >= 1; i--) if(m_list[i].r_idx == MON_KUNEKUNE)
	{
		char m_name[80];

		if(projectable(m_ptr->fy, m_ptr->fx, m_list[i].fy, m_list[i].fx))
		{
			int chance = p_ptr->lev;
			if(r_ptr->flags2 & (RF2_EMPTY_MIND | RF2_WEIRD_MIND | RF2_ELDRITCH_HORROR)) chance = 0;
			if(MON_STUNNED(m_ptr) || MON_CONFUSED(m_ptr)) chance = 0;

			if(r_ptr->flags3 & (RF3_NO_CONF)) chance /= 2;
			if(r_ptr->flags1 & (RF1_UNIQUE) ||r_ptr->flags7 & (RF7_UNIQUE2)) chance /= 2;
			if(r_ptr->flags2 & (RF2_POWERFUL)) chance /= 2;
			if(randint1(r_ptr->level) < chance)
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				if(is_seen(m_ptr)) msg_format(_("%sはくねくねを見てしまった！",
                                                "%s has seen the Kunekune!"),m_name);
				if(!(r_ptr->flags3 & (RF3_NO_CONF)))
				{
					if(is_seen(m_ptr)) msg_format(_("%^sは混乱したようだ。", "%^s is confused."), m_name);
					(void)set_monster_confused(m_idx, MON_CONFUSED(m_ptr) + 10 + randint1(10));
				}
				if(!(r_ptr->flags3 & (RF3_NO_STUN)))
				{
					if(is_seen(m_ptr)) msg_format(_("%^sは朦朧とした。", "%^s is stunned."), m_name);
					(void)set_monster_stunned(m_idx, MON_STUNNED(m_ptr) + 10 + randint1(10));
				}

				return;
			}

		}
		break;
	}

	//v1.1.43 クラウンピースの松明で狂気に冒されたモンスターが同士討ちするための処理。
	//一番近くのモンスターをターゲットにセットする。
	if ((m_ptr->mflag & MFLAG_LUNATIC_TORCH) && !is_pet(m_ptr))
	{
		int new_target_m_idx = 0;
		monster_type *tmp_m_ptr;
		int tmp_dist = 999,tmp_dist_2;

		for (i = m_max - 1; i >= 1; i--)
		{
			if (i == m_idx) continue;
			tmp_m_ptr = &m_list[i];
			if (!tmp_m_ptr->r_idx) continue;
			if (ABS((int)tmp_m_ptr->fx - (int)m_ptr->fx) > MAX_SIGHT || ABS((int)tmp_m_ptr->fy - (int)m_ptr->fy) > MAX_SIGHT) continue;

			tmp_dist_2 = distance(m_ptr->fy, m_ptr->fx, tmp_m_ptr->fy, tmp_m_ptr->fx);

			if ( tmp_dist_2 < tmp_dist)
			{
				tmp_dist = tmp_dist_2;
				new_target_m_idx = i;
			}
		}

		if (new_target_m_idx)
		{
			set_target(m_ptr,m_list[new_target_m_idx].fy, m_list[new_target_m_idx].fx);
		}

	}

	//v1.1.91 抗争系クエストではモンスターは＠より手近な他勢力モンスターを狙うようにしてみる
	//狐狸戦争クエストでもやることにした。そのままでは物凄い勢いで召喚地獄になるので妖怪狐は召喚を持つモンスターに変身しないようにする。
	if (((m_ptr->sub_align & SUB_ALIGN_QUEST_MASK) || p_ptr->inside_quest == QUEST_KORI)
		&& !is_pet(m_ptr) && !(p_ptr->cursed & TRC_AGGRAVATE))
	{
		int new_target_m_idx = 0;
		monster_type *tmp_m_ptr;
		int tmp_dist = 999, tmp_dist_2;

		//一番近くの敵対モンスターをターゲット
		for (i = m_max - 1; i >= 1; i--)
		{
			if (i == m_idx) continue;
			tmp_m_ptr = &m_list[i];
			if (!tmp_m_ptr->r_idx) continue;
			if (!are_enemies(m_ptr, tmp_m_ptr)) continue;

			if (ABS((int)tmp_m_ptr->fx - (int)m_ptr->fx) > MAX_SIGHT || ABS((int)tmp_m_ptr->fy - (int)m_ptr->fy) > MAX_SIGHT) continue;

			tmp_dist_2 = distance(m_ptr->fy, m_ptr->fx, tmp_m_ptr->fy, tmp_m_ptr->fx);

			if (tmp_dist_2 < tmp_dist)
			{
				tmp_dist = tmp_dist_2;
				new_target_m_idx = i;
			}
		}
		//ただし＠のほうが近い場合何もしない
		if (new_target_m_idx && m_ptr->cdis > tmp_dist)
		{
			set_target(m_ptr, m_list[new_target_m_idx].fy, m_list[new_target_m_idx].fx);
		}


	}

	/* Try to cast spell occasionally */
	/*:::呪文使用判定 freq_spellは1/2なら50が入っている*/
//	if (r_ptr->freq_spell && randint1(100) <= r_ptr->freq_spell)
	//v1.1.45　変更
	if (r_ptr->freq_spell && randint1(100) <= monster_spell_freq)
	{
		bool counterattack = FALSE;

		///sys mon 別勢力を互いに喧嘩させるにはこの辺りでもっと明示的にターゲット指定するようにしないといけないのかもしれない

		/* Give priority to counter attack? */
		/*:::ターゲットモンスターが居る場合の処理*/
		if (m_ptr->target_y)
		{
			int t_m_idx = cave[m_ptr->target_y][m_ptr->target_x].m_idx;

			/* The monster must be an enemy, and projectable */
			if (t_m_idx &&
			    are_enemies(m_ptr, &m_list[t_m_idx]) &&
			    projectable(m_ptr->fy, m_ptr->fx, m_ptr->target_y, m_ptr->target_x))
			{
				counterattack = TRUE;
			}
		}

		if (!counterattack)
		{
			/* Attempt to cast a spell */
			if (aware && make_attack_spell(m_idx,0)) return;

			/*
			 * Attempt to cast a spell at an enemy other than the player
			 * (may slow the game a smidgeon, but I haven't noticed.)
			 */
			if (monst_spell_monst(m_idx)) return;
		}
		else
		{
			/* Attempt to do counter attack at first */
			if (monst_spell_monst(m_idx)) return;

			if (aware && make_attack_spell(m_idx,0)) return;
		}
	}

	/* Hack -- Assume no movement */
	/*:::この中に順に移動/攻撃方向候補が格納される。*/
	mm[0] = mm[1] = mm[2] = mm[3] = 0;
	mm[4] = mm[5] = mm[6] = mm[7] = 0;

	//v1.1.86 常に一定の方向へ進む特殊モンスター
	if (r_ptr->flags7 & RF7_FORCE_GO_STRAIGHT)
	{
		if (!m_ptr->xtra_dir_param)
		{
			msg_format(_("ERROR:このモンスターに移動方向が指定されていない",
                        "ERROR: Movement pattern not set for this monster"));
			return;
		}
		else
		{
			mm[0] = mm[1] = mm[2] = mm[3] = m_ptr->xtra_dir_param;
			mm[4] = mm[5] = mm[6] = mm[7] = m_ptr->xtra_dir_param;

		}

	}
	//永琳プレイ時の輝夜の特殊処理
	else if(m_ptr->r_idx == MON_MASTER_KAGUYA && !MON_CONFUSED(m_ptr))
	{
		//あまりプレイヤーから離れているとテレポで寄ってくる
		if(m_ptr->cdis > MAX_SIGHT && r_ptr->level > 34)
		{
			if(m_ptr->cdis >= AAF_LIMIT || randint1(m_ptr->cdis / 10 + 100) > 100)
			{
				if(make_attack_spell(m_idx,1))	return;
			}
		}

		//敵がいない場合プレイヤーについてくる
		if(!get_enemy_dir(m_idx, mm) && !get_moves(m_idx, mm)) return;

	}

	/* Confused -- 100% random */
	/*:::超隠密時や混乱モンスターはランダムに動く*/
	else if (MON_CONFUSED(m_ptr) || !aware)
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 75% random movement */
	else if (((r_ptr->flags1 & (RF1_RAND_50 | RF1_RAND_25)) == (RF1_RAND_50 | RF1_RAND_25)) &&
		 (randint0(100) < 75))
	{
		/* Memorize flags */
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags1 |= (RF1_RAND_50 | RF1_RAND_25);

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 50% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_50) &&
				(randint0(100) < 50))
	{
		/* Memorize flags */
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags1 |= (RF1_RAND_50);

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* 25% random movement */
	else if ((r_ptr->flags1 & RF1_RAND_25) &&
				(randint0(100) < 25))
	{
		/* Memorize flags */
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags1 |= RF1_RAND_25;

		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;
	}

	/* Can't reach player - find something else to hit */
	//else if ((r_ptr->flags1 & RF1_NEVER_MOVE) && (m_ptr->cdis > 1))
	else if (flag_never_move && (m_ptr->cdis > 1))
	{
		/* Try four "random" directions */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		/* Look for an enemy */
#if 0  /* Hack - Too slow.  Mimic pits are horrible with this on. */
		get_enemy_dir(m_idx, mm);
#endif /* 0 */
	}

	/* Pets will follow the player */
	else if (is_pet(m_ptr))
	{
		/* Are we trying to avoid the player? */
		bool avoid = ((p_ptr->pet_follow_distance < 0) &&
						  (m_ptr->cdis <= (0 - p_ptr->pet_follow_distance)));

		/* Do we want to find the player? */
		bool lonely = (!avoid && (m_ptr->cdis > p_ptr->pet_follow_distance));

		/* Should we find the player if we can't find a monster? */
		bool distant = (m_ptr->cdis > PET_SEEK_DIST);

		/* by default, move randomly */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		//パルスィ特殊処理 常にプレイヤーを追いかける
		if(m_ptr->r_idx == MON_PARSEE && (m_ptr->mflag & MFLAG_EPHEMERA))
		{
				(void)get_moves(m_idx, mm);
		}

		/* Look for an enemy */
		else if (!get_enemy_dir(m_idx, mm))
		{
			/* Find the player if necessary */
			if (avoid || lonely || distant)
			{
				/* Remember the leash length */
				int dis = p_ptr->pet_follow_distance;

				/* Hack -- adjust follow distance temporarily */
				if (p_ptr->pet_follow_distance > PET_SEEK_DIST)
				{
					p_ptr->pet_follow_distance = PET_SEEK_DIST;
				}

				/* Find the player */
				(void)get_moves(m_idx, mm);
				/* Restore the leash */
				p_ptr->pet_follow_distance = dis;
			}
		}

	}

	/* Friendly monster movement */
	else if (!is_hostile(m_ptr))
	{
		/* by default, move randomly */
		mm[0] = mm[1] = mm[2] = mm[3] = 5;

		/* Look for an enemy */
		get_enemy_dir(m_idx, mm);
	}
	/* Normal movement */
	else
	{
		/* Logical moves, may do nothing */
		if (!get_moves(m_idx, mm)) return;
	}

	/* Assume nothing */
	do_turn = FALSE;
	do_move = FALSE;
	do_view = FALSE;
	must_alter_to_move = FALSE;

	/* Assume nothing */
	did_open_door = FALSE;
	did_bash_door = FALSE;
	did_take_item = FALSE;
	did_kill_item = FALSE;
	did_move_body = FALSE;
	did_pass_wall = FALSE;
	did_kill_wall = FALSE;

	/*:::mm[]に格納された方角の順に数回移動を試みる*/
	/* Take a zero-terminated array of "directions" */
	for (i = 0; mm[i]; i++)
	{
		/* Get the direction */
		d = mm[i];

		/* Hack -- allow "randomized" motion */
		if (d == 5) d = ddd[randint0(8)];

		/* Get the destination */
		ny = oy + ddy[d];
		nx = ox + ddx[d];

		//v1.1.86 常にまっすぐ動くモンスター　動けなかったら消滅して終了
		if (r_ptr->flags7 & RF7_FORCE_GO_STRAIGHT)
		{
			if (!in_bounds2(ny, nx) || !monster_can_cross_terrain(cave[ny][nx].feat, r_ptr, is_riding_mon ? CEM_RIDING : 0))
			{
				check_quest_completion(m_ptr);
				delete_monster_idx(m_idx);
				return;

			}

		}


		/* Access that cave grid */
		c_ptr = &cave[ny][nx];
		f_ptr = &f_info[c_ptr->feat];
		/* Ignore locations off of edge */
		if (!in_bounds2(ny, nx)) continue;

		can_cross = monster_can_cross_terrain(c_ptr->feat, r_ptr, is_riding_mon ? CEM_RIDING : 0);

		/* Access that cave grid's contents */
		y_ptr = &m_list[c_ptr->m_idx];

		/* Hack -- player 'in' wall */
		if (player_bold(ny, nx))
		{
			do_move = TRUE;
		}

		/* Possibly a monster to attack */
		else if (c_ptr->m_idx)
		{
			do_move = TRUE;
		}
		///mod160627　オカルト魔法「トイレの花子さん」を施された扉
		else if (have_flag(f_ptr->flags, FF_HANAKOSAN))
		{
			int dice = 3 + p_ptr->lev / 12;
			int sides = 77;
			do_move = FALSE;

			if ( !is_pet(m_ptr) && (r_ptr->flags2 & (RF2_BASH_DOOR | RF2_BASH_DOOR))
				|| (p_ptr->pet_extra_flags & PF_OPEN_DOORS))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, is_pet(m_ptr) ? MD_ASSUME_VISIBLE : 0);

				msg_format(_("ドアの中から無数の手が伸び、%sを引きずり込んで閉じた！",
                            "Countless arms burst out of the door and drag %s in!"),m_name);
				project(0, 0, m_ptr->fy, m_ptr->fx, damroll(dice,sides), GF_TELEKINESIS, (PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);

				cave_alter_feat(ny, nx, FF_HURT_ROCK);
				p_ptr->update |= (PU_FLOW);
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
				note_spot(ny, nx);
				lite_spot(ny, nx);
				disturb(1,1);

				if (!m_ptr->r_idx) return;

			}

		}
		/* Monster destroys walls (and doors) */
		else if ((r_ptr->flags2 & RF2_KILL_WALL) &&
		         (can_cross ? !have_flag(f_ptr->flags, FF_LOS) : !is_riding_mon) &&
		         have_flag(f_ptr->flags, FF_HURT_DISI) && !have_flag(f_ptr->flags, FF_PERMANENT) &&
		         check_hp_for_feat_destruction(f_ptr, m_ptr))
		{
			/* Eat through walls/doors/rubble */
			do_move = TRUE;
			if (!can_cross) must_alter_to_move = TRUE;

			/* Monster destroyed a wall (later) */
			did_kill_wall = TRUE;
		}

		/* Floor is open? */
		else if (can_cross)
		{
			/* Go ahead and move */
			do_move = TRUE;

			/* Monster moves through walls (and doors) */
			if ((r_ptr->flags2 & RF2_PASS_WALL) && (!is_riding_mon || p_ptr->pass_wall) &&
			    have_flag(f_ptr->flags, FF_CAN_PASS))
			{
				/* Monster went through a wall */
				did_pass_wall = TRUE;
			}
		}

		/* Handle doors and secret doors */
		else if (is_closed_door(c_ptr->feat))
		{
			bool may_bash = TRUE;

			/* Assume no move allowed */
			do_move = FALSE;

			/* Creature can open doors. */
			if ((r_ptr->flags2 & RF2_OPEN_DOOR) && have_flag(f_ptr->flags, FF_OPEN) &&
				 (!is_pet(m_ptr) || (p_ptr->pet_extra_flags & PF_OPEN_DOORS)))
			{
				/* Closed doors */
				if (!f_ptr->power)
				{
					/* The door is open */
					did_open_door = TRUE;

					/* Do not bash the door */
					may_bash = FALSE;

					/* Take a turn */
					do_turn = TRUE;
				}

				/* Locked doors (not jammed) */
				else
				{
					/* Try to unlock it XXX XXX XXX */
					if (randint0(m_ptr->hp / 10) > f_ptr->power)
					{
						/* Unlock the door */
						cave_alter_feat(ny, nx, FF_DISARM);

						/* Do not bash the door */
						may_bash = FALSE;

						/* Take a turn */
						do_turn = TRUE;
					}
				}
			}

			/* Stuck doors -- attempt to bash them down if allowed */
			if (may_bash && (r_ptr->flags2 & RF2_BASH_DOOR) && have_flag(f_ptr->flags, FF_BASH) &&
				(!is_pet(m_ptr) || (p_ptr->pet_extra_flags & PF_OPEN_DOORS)))
			{
				/* Attempt to Bash XXX XXX XXX */
				if (check_hp_for_feat_destruction(f_ptr, m_ptr) && (randint0(m_ptr->hp / 10) > f_ptr->power))
				{

					/* Message */
					if (have_flag(f_ptr->flags, FF_GLASS))
#ifdef JP
						msg_print("ガラスが砕ける音がした！");
#else
						msg_print("You hear glass breaking!");
#endif


					else if (have_flag(f_ptr->flags, FF_ICE_WALL))
						msg_print(_("氷が砕ける音がした！", "Your hear ice breaking!"));


					else
#ifdef JP
						msg_print("ドアを叩き開ける音がした！");
#else
						msg_print("You hear a door burst open!");
#endif

					/* Disturb (sometimes) */
					if (disturb_minor) disturb(0, 0);

					/* The door was bashed open */
					did_bash_door = TRUE;

					/* Hack -- fall into doorway */
					do_move = TRUE;
					must_alter_to_move = TRUE;
				}
			}


			/* Deal with doors in the way */
			if (did_open_door || did_bash_door)
			{
				/* Break down the door */
				if (did_bash_door && ((randint0(100) < 50) || (feat_state(c_ptr->feat, FF_OPEN) == c_ptr->feat) || have_flag(f_ptr->flags, FF_GLASS)))
				{
					cave_alter_feat(ny, nx, FF_BASH);

					if (!m_ptr->r_idx) /* Killed by shards of glass, etc. */
					{
						/* Update some things */
						p_ptr->update |= (PU_FLOW);
						p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_BASH_DOOR);

						return;
					}
				}

				/* Open the door */
				else
				{
					cave_alter_feat(ny, nx, FF_OPEN);
				}

				f_ptr = &f_info[c_ptr->feat];

				/* Handle viewable doors */
				do_view = TRUE;
			}
		}

		if(p_ptr->pclass == CLASS_MERLIN  &&  music_singing(MUSIC_NEW_MERLIN_SOLO) && player_bold(ny,nx))
		{

			int tmp = r_ptr->level * (r_ptr->flags2 & RF2_POWERFUL)?3:2;
			if(randint1(tmp) < p_ptr->lev + adj_general[p_ptr->stat_ind[A_CHR]])
			{
				do_move = FALSE;
			}
		}


		/*:::「旧神の印」の処理 混沌勢力モンスターの移動を高確率で阻害し、破壊されたら爆発する*/
		if (do_move && is_elder_sign(c_ptr) && (r_ptr->flags3 & RF3_ANG_CHAOS) )
		{

			do_move = FALSE;

			//v2.0.15 友好モンスターも＠の足元の爆発ルーンを踏みに来ないことにする
			if (!is_pet(m_ptr) && !is_friendly(m_ptr) && randint1(r_ptr->level) > 80 + randint1(p_ptr->lev))
			{
				/* Describe observable breakage */
				if (c_ptr->info & CAVE_MARK)
				{
					msg_print(_("旧神の印が破壊された！", "The Sign of Elder Gods is destroyed!"));
				}
				c_ptr->info &= ~(CAVE_MARK);
				c_ptr->info &= ~(CAVE_OBJECT);
				c_ptr->mimic = 0;
				do_move = TRUE;
				note_spot(ny, nx);

				project(0, 4, ny, nx, 2 * (400), GF_HOLY_FIRE, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
				if (!m_ptr->r_idx) return;
			}
		}

		/* Hack -- check for Glyph of Warding */
		else if (do_move && is_glyph_grid(c_ptr) &&
		    !((r_ptr->flags1 & RF1_NEVER_BLOW) && player_bold(ny, nx)))
		{
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward */
			//v2.0.15 友好モンスターも＠の足元のルーンを踏みに来ないことにする
			if (!is_pet(m_ptr) && !is_friendly(m_ptr) && (randint1(BREAK_GLYPH) < r_ptr->level))
			{
				/* Describe observable breakage */
				if (c_ptr->info & CAVE_MARK)
				{
#ifdef JP
					msg_print("守りのルーンが壊れた！");
#else
					msg_print("The rune of protection is broken!");
#endif
				}

				/* Forget the rune */
				c_ptr->info &= ~(CAVE_MARK);

				/* Break the rune */
				c_ptr->info &= ~(CAVE_OBJECT);
				c_ptr->mimic = 0;

				/* Allow movement */
				do_move = TRUE;

				/* Notice */
				note_spot(ny, nx);
			}
		}
		else if (do_move && is_explosive_rune_grid(c_ptr) &&
			 !((r_ptr->flags1 & RF1_NEVER_BLOW) && player_bold(ny, nx)))
		{
			/* Assume no move allowed */
			do_move = FALSE;

			/* Break the ward */
			//v2.0.15 友好モンスターも＠の足元の爆発ルーンを踏みに来ないことにする
			if (!is_pet(m_ptr) && !is_friendly(m_ptr))
			{
				/* Break the ward */
				if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level)
				{
					/* Describe observable breakage */
					if (c_ptr->info & CAVE_MARK)
					{
						/*:::特殊処理　チルノのみルーンの属性が違う*/
						int typ;
						if(p_ptr->pclass == CLASS_CIRNO)
						{
							typ = GF_ICE;
							msg_print(_("地面から氷柱が飛び出した！", "Ice pillars rise from the ground!"));
						}
						else
						{
							typ = GF_MANA;
							msg_print(_("ルーンが爆発した！", "The rune explodes!"));
						}

						project(0, 2, ny, nx, 2 * (p_ptr->lev + damroll(7, 7)), typ, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
					}
				}
				else
				{
#ifdef JP
					msg_print("爆発のルーンは解除された。");
#else
					msg_print("An explosive rune was disarmed.");
#endif
				}

				/* Forget the rune */
				c_ptr->info &= ~(CAVE_MARK);

				/* Break the rune */
				c_ptr->info &= ~(CAVE_OBJECT);
				c_ptr->mimic = 0;

				note_spot(ny, nx);
				lite_spot(ny, nx);

				if (!m_ptr->r_idx) return;
				/* Allow movement */
				do_move = TRUE;
			}
		}

		//v1.1.14 依姫の「祇園様の剣」処理
		if(do_move && (p_ptr->kamioroshi & KAMIOROSHI_GION) && is_hostile(m_ptr))
		{
			char m_name[80];
			int range;

			range = p_ptr->lev / 12;
			if(inventory[INVEN_RARM].name1 == ART_AMENOHABAKIRI ||inventory[INVEN_LARM].name1 == ART_AMENOHABAKIRI)
				range += 4;

			//v1.1.93 <=が<になってて効果範囲が1グリッド少なくlev12〜23まで無意味な技になっていた。修正
			if(m_ptr->cdis <= range && projectable(m_ptr->fy,m_ptr->fx,py,px))
			{
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%sは祇園様の怒りに触れた！",
                            "%s is hit by the wrath of Sword of Gion!"),m_name);
				py_attack(m_ptr->fy,m_ptr->fx,HISSATSU_GION);
				if (m_ptr->r_idx && randint1(r_ptr->level) < randint1(p_ptr->lev)) return;
			}

			if (!m_ptr->r_idx) return;
		}


		/* The player is in the way */
		if (do_move && player_bold(ny, nx))
		{
			/* Some monsters never attack */
			if (r_ptr->flags1 & RF1_NEVER_BLOW)
			{
				/* Hack -- memorize lack of attacks */
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags1 |= (RF1_NEVER_BLOW);

				/* Do not move */
				do_move = FALSE;
			}

			//エイボンの霧の車輪実行中は該当モンスターが攻撃してくることはない
			//v1.1.33 あうん石像化に関して追記
			if(p_ptr->special_defense & (SD_EIBON_WHEEL))
			{
				if(p_ptr->special_defense & (SD_STATUE_FORM))
				{
					do_move = FALSE;
				}
				else
				{
					if(	!(r_ptr->flags3 & RF3_GEN_MASK))
						do_move = FALSE;
				}
			}
			//v2.0.1 ひらり布
			if (p_ptr->tim_hirari_nuno)
			{
				do_move = FALSE;
			}

			/* In anti-melee dungeon, stupid or confused monster takes useless turn */
			if (do_move && (d_info[dungeon_type].flags1 & DF1_NO_MELEE))
			{
				if (!MON_CONFUSED(m_ptr))
				{
					if (!(r_ptr->flags2 & RF2_STUPID)) do_move = FALSE;
					else
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_STUPID);
					}
				}
			}

			/* The player is in the way.  Attack him. */
			//配下や友好モンスターのときもここに入ってmake_attack_normal()に行き、そこで何もせず返ってくる
			if (do_move)
			{
//				if (!p_ptr->riding || one_in_(2))
//				↑元の式。この何気ない条件式は「モンスターが＠の位置に来ない」ための重要な判定だった。
//				if (!p_ptr->riding || CLASS_RIDING_BACKDANCE ? one_in_(4) : one_in_(2))
//				↑こう書いたらペットが＠の位置に動いてくるようになった。三項演算子の優先順位を間違えたせい。
				if (!p_ptr->riding || (CLASS_RIDING_BACKDANCE ? one_in_(4) : one_in_(2)))
				{
					/* Do the attack */
					(void)make_attack_normal(m_idx);

					/* Do not move */
					do_move = FALSE;

					/* Took a turn */
					do_turn = TRUE;

					if(p_ptr->pclass == CLASS_NUE && aware)
					{
						check_nue_revealed(m_idx,200);

					}

				}
			}


		}

		/* A monster is in the way */
		if (do_move && c_ptr->m_idx)
		{
			monster_race *z_ptr = &r_info[y_ptr->r_idx];

			/* Assume no movement */
			do_move = FALSE;

			//v1.1.86 常にまっすぐ動くモンスター　何かが立ちふさがってたら必ず攻撃しそれで終了
			if (r_ptr->flags7 & RF7_FORCE_GO_STRAIGHT)
			{
				if (!y_ptr->r_idx) return;
				monst_attack_monst(m_idx, c_ptr->m_idx);
				return;
			}

			/* Attack 'enemies' */
			else if (((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW) &&
				//v1.1.15フラグ追加
				!(cp_ptr->magicmaster && !are_enemies(m_ptr, y_ptr)) &&
				 (r_ptr->mexp * r_ptr->level > z_ptr->mexp * z_ptr->level) &&
				 can_cross && (c_ptr->m_idx != p_ptr->riding)) ||
				are_enemies(m_ptr, y_ptr) || MON_CONFUSED(m_ptr))
			{
				if (!(r_ptr->flags1 & RF1_NEVER_BLOW))
				{
					if (r_ptr->flags2 & RF2_KILL_BODY)
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_KILL_BODY);
					}

					/* attack */
					if (y_ptr->r_idx && (y_ptr->hp >= 0))
					{
						if (monst_attack_monst(m_idx, c_ptr->m_idx)) return;

						/* In anti-melee dungeon, stupid or confused monster takes useless turn */
						else if (d_info[dungeon_type].flags1 & DF1_NO_MELEE)
						{
							if (MON_CONFUSED(m_ptr)) return;
							else if (r_ptr->flags2 & RF2_STUPID)
							{
								if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_STUPID);
								return;
							}
						}
					}
				}
			}

			/* Push past weaker monsters (unless leaving a wall) */
			else if ((r_ptr->flags2 & RF2_MOVE_BODY) && !flag_never_move &&
				(r_ptr->mexp > z_ptr->mexp) &&
				can_cross && (c_ptr->m_idx != p_ptr->riding) &&
				monster_can_cross_terrain(cave[m_ptr->fy][m_ptr->fx].feat, z_ptr, 0))
			{
				/* Allow movement */
				do_move = TRUE;

				/* Monster pushed past another monster */
				did_move_body = TRUE;

				/* Wake up the moved monster */
				(void)set_monster_csleep(c_ptr->m_idx, 0);

				/* XXX XXX XXX Message */
			}
		}

		if (is_riding_mon)
		{
			if (!p_ptr->riding_ryoute && !MON_MONFEAR(&m_list[p_ptr->riding])) do_move = FALSE;
			///mod140309 乗馬技能がそこそこあると馬が勝手に動かなくなるようにした
			if(ref_skill_exp(SKILL_RIDING) >= RIDING_STOP) do_move = FALSE;

			if (CLASS_RIDING_BACKDANCE) do_move = FALSE;

		}

		if (did_kill_wall && do_move)
		{
			if (one_in_(GRINDNOISE))
			{
				if (have_flag(f_ptr->flags, FF_GLASS))
#ifdef JP
					msg_print("何かの砕ける音が聞こえる。");
#else
					msg_print("There is a crashing sound.");
#endif
				else
#ifdef JP
					msg_print("ギシギシいう音が聞こえる。");
#else
					msg_print("There is a grinding sound.");
#endif
			}

			cave_alter_feat(ny, nx, FF_HURT_DISI);

			if (!m_ptr->r_idx) /* Killed by shards of glass, etc. */
			{
				/* Update some things */
				p_ptr->update |= (PU_FLOW);
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_KILL_WALL);

				return;
			}

			f_ptr = &f_info[c_ptr->feat];

			/* Note changes to viewable region */
			do_view = TRUE;

			/* Take a turn */
			do_turn = TRUE;
		}

		if (must_alter_to_move && (r_ptr->flags7 & RF7_AQUATIC))
		{
			if (!monster_can_cross_terrain(c_ptr->feat, r_ptr, is_riding_mon ? CEM_RIDING : 0))
			{
				/* Assume no move allowed */
				do_move = FALSE;
			}
		}

		/*
		 * Check if monster can cross terrain
		 * This is checked after the normal attacks
		 * to allow monsters to attack an enemy,
		 * even if it can't enter the terrain.
		 */
		if (do_move && !can_cross && !did_kill_wall && !did_bash_door)
		{
			/* Assume no move allowed */
			do_move = FALSE;
		}

		/* Some monsters never move */
		if (do_move && flag_never_move)
		{
			/* Hack -- memorize lack of moves */
			if (is_original_ap_and_seen(m_ptr) && (r_ptr->flags1 & RF1_NEVER_MOVE)) r_ptr->r_flags1 |= (RF1_NEVER_MOVE);

			/* Do not move */
			do_move = FALSE;
		}

		/* Creature has been allowed move */
		if (do_move)
		{
			/* Take a turn */
			do_turn = TRUE;

			if(p_ptr->pclass == CLASS_NUE)
			{
				check_nue_revealed(m_idx,50);
			}

			if (have_flag(f_ptr->flags, FF_TREE))
			{
				if (!(r_ptr->flags7 & RF7_CAN_FLY) && !(r_ptr->flags8 & RF8_WILD_WOOD))
				{
					m_ptr->energy_need += ENERGY_NEED();
				}
			}
			//v1.1.85 深い雷雲と吹雪に耐性のないモンスターが入ろうとすると一瞬減速する(森と同じ)
			if (have_flag(f_ptr->flags, FF_COLD_PUDDLE) && have_flag(f_ptr->flags, FF_DEEP) && !(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
			{
					m_ptr->energy_need += ENERGY_NEED();
			}
			if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE) && have_flag(f_ptr->flags, FF_DEEP) && !(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
			{
					m_ptr->energy_need += ENERGY_NEED();

			}


			if (!is_riding_mon)
			{
				/* Hack -- Update the old location */
				cave[oy][ox].m_idx = c_ptr->m_idx;

				/* Mega-Hack -- move the old monster, if any */
				if (c_ptr->m_idx)
				{
					/* Move the old monster */
					y_ptr->fy = oy;
					y_ptr->fx = ox;

					/* Update the old monster */
					update_mon(c_ptr->m_idx, TRUE);
				}

				/* Hack -- Update the new location */
				c_ptr->m_idx = m_idx;

				/* Move the monster */
				m_ptr->fy = ny;
				m_ptr->fx = nx;

				/* Update the monster */
				update_mon(m_idx, TRUE);

				/* Redraw the old grid */
				lite_spot(oy, ox);

				/* Redraw the new grid */
				lite_spot(ny, nx);

			}
			else
			{
				/* Sound */
				/* sound(SOUND_WALK); */

				/* Move the player */
				if (!move_player_effect(ny, nx, MPE_DONT_PICKUP)) break;
			}

			//v2.0.12 敵対的なモンスターが＠の作ったトラップに踏み入ったときの解除判定とトラップ発動処理
			if (have_flag(f_ptr->flags, FF_TRAP) && is_hostile(m_ptr) && (c_ptr->cave_xtra_flag & CAVE_XTRA_FLAG_YOUR_TRAP))
			{

				int rlev = r_ptr->level;
				if (rlev < 5) rlev = 5;
				if (r_ptr->flags2 & (RF2_POWERFUL)) rlev = rlev * 3 / 2;
				if (r_ptr->flags2 & (RF2_GIGANTIC)) rlev /= 2;

				if (r_ptr->flags2 & (RF2_SMART)) rlev *= 2;
				else if (r_ptr->flags2 & (RF2_EMPTY_MIND)) rlev /= 2;
				else if (r_ptr->flags2 & RF2_STUPID) rlev = 0;

				if (randint1(rlev) > p_ptr->lev)
				{
					msg_print(_("トラップが破壊された！", "The trap gets destroyed!"));
					cave_alter_feat(ny, nx, FF_DISARM);
					note_spot(ny, nx);
					lite_spot(ny, nx);
				}
				else
				{
					//床トラップの発動
					activate_floor_trap(ny, nx, 0);
					note_spot(ny, nx);
					lite_spot(ny, nx);
					//トラップで倒したとき
					if (!m_ptr->r_idx)
					{
						p_ptr->update |= (PU_FLOW);
						p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
						return;
					}
				}
				note_spot(ny, nx);
				lite_spot(ny, nx);

			}


			/* Possible disturb */
			if (m_ptr->ml &&
			    (disturb_move ||
			     (disturb_near && (m_ptr->mflag & MFLAG_VIEW) && projectable(py, px, m_ptr->fy, m_ptr->fx)) ||
			     (disturb_high && ( ap_r_ptr->r_tkills == 0 || ap_r_ptr->r_tkills && ap_r_ptr->level >= p_ptr->lev))))
			{
				/* Disturb */
				if (is_hostile(m_ptr))
					disturb(0, 1);
			}

			/* Take or Kill objects on the floor */
			/*:::アイテム拾い、破壊　武器が弱点属性エゴの場合拾えないらしい*/
			if (c_ptr->o_idx && (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_KILL_ITEM)) &&
			    (!is_pet(m_ptr) || ((p_ptr->pet_extra_flags & PF_PICKUP_ITEMS) && (r_ptr->flags2 & RF2_TAKE_ITEM))))
			{
				s16b this_o_idx, next_o_idx;
				bool do_take = (r_ptr->flags2 & RF2_TAKE_ITEM) ? TRUE : FALSE;

				/* Scan all objects in the grid */
				for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
				{
					u32b flgs[TR_FLAG_SIZE], flg2 = 0L, flg3 = 0L, flgr = 0L;
					char m_name[80], o_name[MAX_NLEN];

					/* Acquire object */
					object_type *o_ptr = &o_list[this_o_idx];

					/* Acquire next object */
					next_o_idx = o_ptr->next_o_idx;

					if (do_take)
					{
						/* Skip gold */
						if (o_ptr->tval == TV_GOLD) continue;

						/*
						 * Skip "real" corpses and statues, to avoid extreme
						 * silliness like a novice rogue pockets full of statues
						 * and corpses.
						 */
						///del131221 死体と像
						//if ((o_ptr->tval == TV_CORPSE) ||
						//    (o_ptr->tval == TV_STATUE)) continue;
					}

					/* Extract some flags */
					object_flags(o_ptr, flgs);

					/* Acquire the object name */
					object_desc(o_name, o_ptr, 0);

					/* Acquire the monster name */
					monster_desc(m_name, m_ptr, MD_INDEF_HIDDEN);
					///mon res
					/* React to objects that hurt the monster */
					///res item ego　アイテム拾い判定　面倒だからスレイとか気にせず拾うことにしようか？
					///mod131231 モンスターフラグ変更
					if (have_flag(flgs, TR_SLAY_DRAGON)) flg3 |= (RF3_DRAGON);
					if (have_flag(flgs, TR_KILL_DRAGON)) flg3 |= (RF3_DRAGON);
					if (have_flag(flgs, TR_SLAY_DEITY))  flg3 |= (RF3_DEITY);
					if (have_flag(flgs, TR_KILL_DEITY))  flg3 |= (RF3_DEITY);
					if (have_flag(flgs, TR_SLAY_HUMAN))  flg3 |= (RF3_DEMIHUMAN);
					if (have_flag(flgs, TR_KILL_HUMAN))  flg3 |= (RF3_DEMIHUMAN);
					if (have_flag(flgs, TR_SLAY_KWAI))    flg3 |= (RF3_KWAI);
					if (have_flag(flgs, TR_KILL_KWAI))    flg3 |= (RF3_KWAI);
					if (have_flag(flgs, TR_SLAY_DEMON))  flg3 |= (RF3_DEMON);
					if (have_flag(flgs, TR_KILL_DEMON))  flg3 |= (RF3_DEMON);
					if (have_flag(flgs, TR_SLAY_UNDEAD)) flg3 |= (RF3_UNDEAD);
					if (have_flag(flgs, TR_KILL_UNDEAD)) flg3 |= (RF3_UNDEAD);
					if (have_flag(flgs, TR_SLAY_ANIMAL)) flg3 |= (RF3_ANIMAL);
					if (have_flag(flgs, TR_KILL_ANIMAL)) flg3 |= (RF3_ANIMAL);
					if (have_flag(flgs, TR_SLAY_EVIL))   flg3 |= (RF3_ANG_CHAOS);
					if (have_flag(flgs, TR_KILL_EVIL))   flg3 |= (RF3_ANG_CHAOS);
					if (have_flag(flgs, TR_SLAY_GOOD))   flg3 |= (RF3_ANG_COSMOS);
					if (have_flag(flgs, TR_KILL_GOOD))   flg3 |= (RF3_ANG_COSMOS);
					//if (have_flag(flgs, TR_SLAY_HUMAN))  flg2 |= (RF2_HUMAN);
					//if (have_flag(flgs, TR_KILL_HUMAN))  flg2 |= (RF2_HUMAN);
					if (have_flag(flgs, TR_SLAY_HUMAN))  flg3 |= (RF3_HUMAN);
					if (have_flag(flgs, TR_KILL_HUMAN))  flg3 |= (RF3_HUMAN);
					if (have_flag(flgs, TR_BRAND_ACID))  flgr |= (RFR_IM_ACID);
					if (have_flag(flgs, TR_BRAND_ELEC))  flgr |= (RFR_IM_ELEC);
					if (have_flag(flgs, TR_BRAND_FIRE))  flgr |= (RFR_IM_FIRE);
					if (have_flag(flgs, TR_BRAND_COLD))  flgr |= (RFR_IM_COLD);
					if (have_flag(flgs, TR_BRAND_POIS))  flgr |= (RFR_IM_POIS);

					/* The object cannot be picked up by the monster */
					if (object_is_artifact(o_ptr) || (r_ptr->flags3 & flg3) || (r_ptr->flags2 & flg2) ||
					    ((~(r_ptr->flagsr) & flgr) && !(r_ptr->flagsr & RFR_RES_ALL)))
					{
						/* Only give a message for "take_item" */
						if (do_take && (r_ptr->flags2 & RF2_STUPID))
						{
							/* Take note */
							did_take_item = TRUE;

							/* Describe observable situations */
							if (m_ptr->ml && player_can_see_bold(ny, nx))
							{
								/* Dump a message */
#ifdef JP
								msg_format("%^sは%sを拾おうとしたが、だめだった。", m_name, o_name);
#else
								msg_format("%^s tries to pick up %s, but fails.", m_name, o_name);
#endif
							}
						}
					}

					/* Pick up the item */
					else if (do_take)
					{
						/* Take note */
						did_take_item = TRUE;

						/* Describe observable situations */
						if (player_can_see_bold(ny, nx))
						{
							/* Dump a message */
#ifdef JP
							msg_format("%^sが%sを拾った。", m_name, o_name);
#else
							msg_format("%^s picks up %s.", m_name, o_name);
#endif
						}

						/* Excise the object */
						excise_object_idx(this_o_idx);

						/* Forget mark */
						o_ptr->marked &= OM_TOUCHED;

						/* Forget location */
						o_ptr->iy = o_ptr->ix = 0;

						/* Memorize monster */
						o_ptr->held_m_idx = m_idx;

						/* Build a stack */
						o_ptr->next_o_idx = m_ptr->hold_o_idx;

						/* Carry object */
						m_ptr->hold_o_idx = this_o_idx;
					}

					/* Destroy the item if not a pet */
					else if (!is_pet(m_ptr))
					{
						/* Take note */
						did_kill_item = TRUE;

						/* Describe observable situations */
						if (player_has_los_bold(ny, nx))
						{
							/* Dump a message */
#ifdef JP
							msg_format("%^sが%sを破壊した。", m_name, o_name);
#else
							msg_format("%^s destroys %s.", m_name, o_name);
#endif
						}

						/* Delete the object */
						delete_object_idx(this_o_idx);
					}
				}
			}
		}

		/* Stop when done */
		if (do_turn) break;
	}

	/*
	 *  Forward movements failed, but now received LOS attack!
	 *  Try to flow by smell.
	 */
	if (p_ptr->no_flowed && i > 2 &&  m_ptr->target_y)
		m_ptr->mflag2 &= ~MFLAG2_NOFLOW;

	/* If we haven't done anything, try casting a spell again */
	if (!do_turn && !do_move && !MON_MONFEAR(m_ptr) && !is_riding_mon && aware)
	{
		/* Try to cast spell again */
//		if (r_ptr->freq_spell && randint1(100) <= r_ptr->freq_spell)
		//v1.1.45 変更
		if (r_ptr->freq_spell && randint1(100) <= monster_spell_freq)
		{
			if (make_attack_spell(m_idx,0)) return;
		}
	}

	///mod141005 幽々子の「死出の誘蛾灯」敵の行動終了時に解除
	if (p_ptr->pclass == CLASS_YUYUKO && m_ptr->mflag & MFLAG_SPECIAL) m_ptr->mflag &= ~(MFLAG_SPECIAL);


	/* Notice changes in view */
	if (do_view)
	{
		/* Update some things */
		p_ptr->update |= (PU_FLOW);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}

	/* Notice changes in view */
	if (do_move && ((r_ptr->flags7 & (RF7_SELF_LD_MASK | RF7_HAS_DARK_1 | RF7_HAS_DARK_2))
		|| ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !p_ptr->inside_battle)))
	{
		/* Update some things */
		p_ptr->update |= (PU_MON_LITE);
	}


	/*:::Mega Hack - 赤蛮奇の頭が見たものは＠も認識する。前の方にも似たルーチンがある*/
	if(do_move &&
		(p_ptr->pclass == CLASS_BANKI && (m_ptr->r_idx == MON_BANKI_HEAD_1 || m_ptr->r_idx == MON_BANKI_HEAD_2))
			|| (p_ptr->pclass == CLASS_YOUMU && m_ptr->r_idx == MON_YOUMU && (m_ptr->mflag & MFLAG_EPHEMERA)))

	{
		for (i = m_max - 1; i >= 1; i--)
		{
			monster_type *m_ptr_tmp = &m_list[i];
			if (!m_ptr_tmp->r_idx) continue;
			if(i == m_idx || los(m_ptr->fy,m_ptr->fx,m_ptr_tmp->fy,m_ptr_tmp->fx)) m_ptr_tmp->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
		}
	}


	/* Learn things from observable monster */
	if (is_original_ap_and_seen(m_ptr))
	{
		/* Monster opened a door */
		if (did_open_door) r_ptr->r_flags2 |= (RF2_OPEN_DOOR);

		/* Monster bashed a door */
		if (did_bash_door) r_ptr->r_flags2 |= (RF2_BASH_DOOR);

		/* Monster tried to pick something up */
		if (did_take_item) r_ptr->r_flags2 |= (RF2_TAKE_ITEM);

		/* Monster tried to crush something */
		if (did_kill_item) r_ptr->r_flags2 |= (RF2_KILL_ITEM);

		/* Monster pushed past another monster */
		if (did_move_body) r_ptr->r_flags2 |= (RF2_MOVE_BODY);

		/* Monster passed through a wall */
		if (did_pass_wall) r_ptr->r_flags2 |= (RF2_PASS_WALL);

		/* Monster destroyed a wall */
		if (did_kill_wall) r_ptr->r_flags2 |= (RF2_KILL_WALL);
	}


	/* Hack -- get "bold" if out of options */
	/*:::袋小路に恐怖した敵を追い詰めたら恐怖解除?*/
	//v1.1.96 低レベルのモンスターは恐怖解除されにくいようにする
	if (!do_turn && !do_move && MON_MONFEAR(m_ptr) && aware )
	{
		char m_name[80];

		/* Acquire the monster name */
		monster_desc(m_name, m_ptr, 0);

		if (randint1(100) < r_ptr->level)
		{
			/* No longer afraid */
			(void)set_monster_monfear(m_idx, 0);

			/* Message if seen */
			if (see_m)
			{

				/* Dump a message */
#ifdef JP
				msg_format("%^sは戦いを決意した！", m_name);
#else
				msg_format("%^s turns to fight!", m_name);
#endif
			}

		}
		else
		{
			/* Message if seen */
			if (see_m)
			{
				/* Dump a message */
#ifdef JP
				msg_format("%^sは恐怖におののいている...", m_name);
#else
				msg_format("%^s is overwhelmed with fear...", m_name);
#endif
			}


		}
		//if (m_ptr->ml) chg_virtue(V_COMPASSION, -1);

	}
}

/*
 * Process all the "live" monsters, once per game turn.
 *:::生きているモンスターの処理
 *
 * During each game turn, we scan through the list of all the "live" monsters,
 * (backwards, so we can excise any "freshly dead" monsters), energizing each
 * monster, and allowing fully energized monsters to move, attack, pass, etc.
 *:::turnごとに一度全モンスターをスキャンし、
 *:::モンスターに行動エネルギーを与え、エネルギーが足りたモンスターは行動させる。
 *
 * Note that monsters can never move in the monster array (except when the
 * "compact_monsters()" function is called by "dungeon()" or "save_player()").
 *:::ここよく解らん
 *
 * This function is responsible for at least half of the processor time
 * on a normal system with a "normal" amount of monsters and a player doing
 * normal things.
 *:::この関数でこのゲームの処理の半分くらい喰う？
 *
 * When the player is resting, virtually 90% of the processor time is spent
 * in this function, and its children, "process_monster()" and "make_move()".
 *:::＠の休憩中は処理の90%以上使う。
 *
 * Most of the rest of the time is spent in "update_view()" and "lite_spot()",
 * especially when the player is running.
 *:::他には主にアップデートや光源関係処理に使う
 *
 * Note the special "MFLAG_BORN" flag, which allows us to ignore "fresh"
 * monsters while they are still being "born".  A monster is "fresh" only
 * during the turn in which it is created, and we use the "hack_m_idx" to
 * determine if the monster is yet to be processed during the current turn.
 *:::よく解らん。このターンで生成されたばかりのモンスターはこの関数の処理対象にならない？
 *
 * Note the special "MFLAG_NICE" flag, which allows the player to get one
 * move before any "nasty" monsters get to use their spell attacks.
 *:::NICEフラグのついたモンスターは＠に魔法攻撃をしない？
 *
 * Note that when the "knowledge" about the currently tracked monster
 * changes (flags, attacks, spells), we induce a redraw of the monster
 * recall window.
 *:::モンスターの行動によりそのモンスターの思い出が更新された場合、思い出のサブウィンドウを再描画する
 */
/*:::モンスターすべての行動をする*/
void process_monsters(void)
{
	int             i;
	int             fx, fy;

	/*:::モンスターが＠の位置を感知できているかどうかのフラグ？*/
	bool            test;

	monster_type    *m_ptr;
	monster_race    *r_ptr;

	int             old_monster_race_idx;

	u32b    old_r_flags1 = 0L;
	u32b    old_r_flags2 = 0L;
	u32b    old_r_flags3 = 0L;
	u32b    old_r_flags4 = 0L;
	u32b    old_r_flags5 = 0L;
	u32b    old_r_flags6 = 0L;
	///mod140102 モンスター新魔法フラグ追加に伴いflags9追加
	u32b    old_r_flags9 = 0L;
	u32b    old_r_flagsr = 0L;

	byte    old_r_blows0 = 0;
	byte    old_r_blows1 = 0;
	byte    old_r_blows2 = 0;
	byte    old_r_blows3 = 0;

	byte    old_r_cast_spell = 0;

	int speed;

	//v2.0.1 現実変容の発動カウントが1のときモンスターは動かないことにする。
	//テレポレベルは安全に離脱できるが即時現実変容は安全でないというのは分かりづらい事故要因になりそうなので
	if (p_ptr->alter_reality == 1) return;

	/* Clear monster fighting indicator */
	mon_fight = FALSE;

	/* Memorize old race */
	old_monster_race_idx = p_ptr->monster_race_idx;

	/* Acquire knowledge */
	/*:::＠がターゲットにしているモンスターがいればフラグを保持しておく*/
	if (p_ptr->monster_race_idx)
	{
		/* Acquire current monster */
		r_ptr = &r_info[p_ptr->monster_race_idx];

		/* Memorize flags */
		old_r_flags1 = r_ptr->r_flags1;
		old_r_flags2 = r_ptr->r_flags2;
		old_r_flags3 = r_ptr->r_flags3;
		old_r_flags4 = r_ptr->r_flags4;
		old_r_flags5 = r_ptr->r_flags5;
		old_r_flags6 = r_ptr->r_flags6;

	///mod140102 モンスター新魔法フラグ追加に伴いflags9追加
		old_r_flags9 = r_ptr->r_flags9;

		old_r_flagsr = r_ptr->r_flagsr;

		/* Memorize blows */
		old_r_blows0 = r_ptr->r_blows[0];
		old_r_blows1 = r_ptr->r_blows[1];
		old_r_blows2 = r_ptr->r_blows[2];
		old_r_blows3 = r_ptr->r_blows[3];

		/* Memorize castings */
		old_r_cast_spell = r_ptr->r_cast_spell;
	}


	/* Process the monsters (backwards) */
	/*:::全モンスターループ*/
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];
		r_ptr = &r_info[m_ptr->r_idx];

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		/*:::なぜbreakでなくcontinue？*/
		if (p_ptr->wild_mode) continue;


		/* Handle "fresh" monsters */
		/*:::MFLAG_BORNの処理　フラグを消して処理を飛ばす*/
		if (m_ptr->mflag & MFLAG_BORN)
		{
			/* No longer "fresh" */
			m_ptr->mflag &= ~(MFLAG_BORN);

			/* Skip */
			continue;
		}

		/* Hack -- Require proximity */
		/*:::＠から100グリッド以上離れているモンスターは処理しない*/
		///system 一定以上離れたモンスターは行動しない。ここ消したらどうなるだろう？
		//輝夜だけは行動するようにした。必ずテレポートしてくる。
		if (m_ptr->cdis >= AAF_LIMIT &&
			!(m_ptr->r_idx == MON_MASTER_KAGUYA && r_ptr->level > 34 && !MON_CONFUSED(m_ptr))) continue;


		/* Access the location */
		fx = m_ptr->fx;
		fy = m_ptr->fy;

		/* Flow by smell is allowed */
		if (!p_ptr->no_flowed)
		{
			m_ptr->mflag2 &= ~MFLAG2_NOFLOW;
		}

		/* Assume no move */
		test = FALSE;

		/* Handle "sensing radius" */
		if (m_ptr->cdis <= (is_pet(m_ptr) ? (r_ptr->aaf > MAX_SIGHT ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf))
		{
			/* We can "sense" the player */
			test = TRUE;
		}

		/* Handle "sight" and "aggravation" */
		else if ((m_ptr->cdis <= MAX_SIGHT) &&
			(player_has_los_bold(fy, fx) || (p_ptr->cursed & TRC_AGGRAVATE)))
		{
			/* We can "see" or "feel" the player */
			test = TRUE;
		}

		/*:::匂い追跡処理　今は使われていないようだ*/
#if 0 /* (cave[py][px].when == cave[fy][fx].when) is always FALSE... */
		/* Hack -- Monsters can "smell" the player from far away */
		/* Note that most monsters have "aaf" of "20" or so */
		else if (!(m_ptr->mflag2 & MFLAG2_NOFLOW) &&
			cave_have_flag_bold(py, px, FF_MOVE) &&
			(cave[py][px].when == cave[fy][fx].when) &&
			(cave[fy][fx].dist < MONSTER_FLOW_DEPTH) &&
			(cave[fy][fx].dist < r_ptr->aaf))
		{
			/* We can "smell" the player */
			test = TRUE;
		}
#endif
		else if (m_ptr->target_y) test = TRUE;

		/*:::感知範囲内に＠がいないモンスターは動かない・・？*/
		///mod140816 感知範囲内に＠がいないモンスターは動かないらしい。消してみた。
		/* Do nothing */
		//if (!test) continue;

		//v1.1.41 舞と里乃の特殊騎乗　モンスターの通常の速度になる
		if (CLASS_RIDING_BACKDANCE && p_ptr->riding == i)
		{
			speed = m_ptr->mspeed;
			if (ironman_nightmare) speed += 5;
			if (MON_FAST(m_ptr)) speed += 10;
			if (MON_SLOW(m_ptr)) speed -= 10;

			//プレイヤーよりモンスターのほうが遅い場合+10を上限に速度加算
			//v2.0.2 この処理は典には適用されない
			if ((p_ptr->pclass == CLASS_MAI || p_ptr->pclass == CLASS_SATONO) && speed < p_ptr->pspeed )
				speed += MIN(10,(p_ptr->pspeed - speed)/2);
		}
		else if (p_ptr->riding == i)
		{
			speed = p_ptr->pspeed;
		}
		else
		{
			speed = m_ptr->mspeed;

			/* Monsters move quickly in Nightmare mode */
			if (ironman_nightmare) speed += 5;

			if (MON_FAST(m_ptr)) speed += 10;
			if (MON_SLOW(m_ptr)) speed -= 10;

		}

		/* Give this monster some energy */
		m_ptr->energy_need -= SPEED_TO_ENERGY(speed);

		/* Not enough energy to move */
		if (m_ptr->energy_need > 0) continue;

		/* Use up "some" energy */
		///system ここを定数にして加速値のばらつきをなくせばモンスターの行動順が一定になるか
		m_ptr->energy_need += ENERGY_NEED();


		/* Save global index */
		/*:::今行動しているモンスターのidxを保存しておく*/
		hack_m_idx = i;

		/* Process the monster */
		process_monster(i);

		//v1.1.73 八千慧のモンスター買収フラグ解除 make_attack_spell()の終了時にもFALSEにしているが念のためここでもFALSEにしとく
		flag_bribe_summon_monsters = FALSE;


		///v1.1.13 豊姫の「公転周期の罠」でprocess_monster()中にそのモンスターがdeleteされるようになったので念のため処理終了しとく
		if(!m_list[i].r_idx) continue;


		///mod160319 てゐの掘った落とし穴に飛んでいないモンスターが乗った時
		if(m_ptr->r_idx && cave_have_flag_bold(m_ptr->fy,m_ptr->fx,FF_TEWI_PIT) && !(r_ptr->flags7 & RF7_CAN_FLY))
		{

			int check = r_ptr->level;
			char m_name[160];

			monster_desc(m_name, m_ptr, 0);
			if(r_ptr->flags1 & RF1_UNIQUE || r_ptr->flags7 & RF7_UNIQUE2) check *= 2;
			if(r_ptr->flags2 & RF2_SMART) check *= 2;
			if(r_ptr->flags2 & RF2_GIGANTIC && p_ptr->lev < 40) check *= 2;

			if(randint1(check) < p_ptr->lev)
			{
				msg_format(_("%sは落とし穴に落ちた！", "%s fell into a pit!"), m_name);
				//耐性無視スタン
				set_monster_stunned(i, 5+randint1(5) + MON_STUNNED(m_ptr));
				//さらに判定通ったら気絶
				if(randint1(check*2) < p_ptr->lev) set_monster_csleep(i, 100+randint1(100));
				//次回行動遅延
				m_ptr->energy_need += ENERGY_NEED();
			}
			else
			{
				msg_format(_("%sは落とし穴に落ちなかった。", "%s avoided a pit."), m_name);
			}
			cave_set_feat(m_ptr->fy,m_ptr->fx,feat_floor);
		}

		reset_target(m_ptr);

		/*:::匂いを忘れる判定　今は使われているのか？*/
		/* Give up flow_by_smell when it might useless */
		if (p_ptr->no_flowed && one_in_(3))
			m_ptr->mflag2 |= MFLAG2_NOFLOW;

		/* Hack -- notice death or departure */
		if (!p_ptr->playing || p_ptr->is_dead) break;

		/* Notice leaving */
		if (p_ptr->leaving) break;
	}

	/* Reset global index */
	hack_m_idx = 0;


	/* Tracking a monster race (the same one we were before) */
	if (p_ptr->monster_race_idx && (p_ptr->monster_race_idx == old_monster_race_idx))
	{
		/* Acquire monster race */
		r_ptr = &r_info[p_ptr->monster_race_idx];

		/* Check for knowledge change */
		if ((old_r_flags1 != r_ptr->r_flags1) ||
			(old_r_flags2 != r_ptr->r_flags2) ||
			(old_r_flags3 != r_ptr->r_flags3) ||
			(old_r_flags4 != r_ptr->r_flags4) ||
			(old_r_flags5 != r_ptr->r_flags5) ||
			(old_r_flags6 != r_ptr->r_flags6) ||
	///mod140102 モンスター新魔法フラグ追加に伴いflags9追加
			(old_r_flags9 != r_ptr->r_flags9) ||
			(old_r_flagsr != r_ptr->r_flagsr) ||
			(old_r_blows0 != r_ptr->r_blows[0]) ||
			(old_r_blows1 != r_ptr->r_blows[1]) ||
			(old_r_blows2 != r_ptr->r_blows[2]) ||
			(old_r_blows3 != r_ptr->r_blows[3]) ||
			(old_r_cast_spell != r_ptr->r_cast_spell))
		{
			/* Window stuff */
			p_ptr->window |= (PW_MONSTER);
		}
	}
}


int get_mproc_idx(int m_idx, int mproc_type)
{
	s16b *cur_mproc_list = mproc_list[mproc_type];
	int i;

	for (i = mproc_max[mproc_type] - 1; i >= 0; i--)
	{
		if (cur_mproc_list[i] == m_idx) return i;
	}

	return -1;
}


static void mproc_add(int m_idx, int mproc_type)
{
	if (mproc_max[mproc_type] < max_m_idx) mproc_list[mproc_type][mproc_max[mproc_type]++] = m_idx;
}


static void mproc_remove(int m_idx, int mproc_type)
{
	int mproc_idx = get_mproc_idx(m_idx, mproc_type);
	if (mproc_idx >= 0) mproc_list[mproc_type][mproc_idx] = mproc_list[mproc_type][--mproc_max[mproc_type]];
}


/*
 * Initialize monster process
 */
//フロアのモンスター全てが持っている一時ステータス情報を初期化　
//mproc_list[一時ステータス番号][連番]にm_listのインデックスを記録する
//process_monster_mtimed()でモンスターの一時ステータスカウントを一括して減らすためにmproc_list[]が使われているが、
//m_list[]を普通にループするのに比べて何のメリットがあるのかよくわからない。
//なぜこんなややこしい仕組みを実装したんだろうか？
void mproc_init(void)
{
	monster_type *m_ptr;
	int          i, cmi;

	/* Reset "mproc_max[]" */
	for (cmi = 0; cmi < MAX_MTIMED; cmi++) mproc_max[cmi] = 0;

	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		for (cmi = 0; cmi < MAX_MTIMED; cmi++)
		{
			if (m_ptr->mtimed[cmi]) mproc_add(i, cmi);
		}
	}
}

//v1.1.94 モンスター用の一時効果発生/終了の追加分
//m_ptr->mtimed[7]以降が対象
//一時効果が発生/終了したときTRUEを返す。数値の増減だけならFALSE。
//「攻撃力が下がった！」などのメッセージはこの中では出さない。
//アルコールのみ出すことにする
bool set_monster_timed_status_add(int mtimed_type, int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;
	int old_val,new_val;

	/* Hack -- Force good values */
	v = (v > 30000) ? 30000 : (v < 0) ? 0 : v;

	if (mtimed_type <= MTIMED_INVULNER || mtimed_type >= MAX_MTIMED)
	{
		msg_format(_("ERROR:set_monster_timed_status_add()に不正なtype値(%d)が渡された",
                    "ERROR: Incorrect type value (%d) passed to set_monster_timed_status_add()"), mtimed_type);
		return FALSE;
	}

	if (cheat_xtra)
		msg_format("m_idx%d timed2: type(%d) val(%d)",m_idx, mtimed_type, v);

	old_val = m_ptr->mtimed[mtimed_type];

	/* Open */
	if (v)
	{
		if (!m_ptr->mtimed[mtimed_type])
		{
			mproc_add(m_idx, mtimed_type);
			notice = TRUE;
		}
	}
	/* Shut */
	else
	{
		if (m_ptr->mtimed[mtimed_type])
		{
			mproc_remove(m_idx, mtimed_type);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[mtimed_type] = v;

	new_val = m_ptr->mtimed[mtimed_type];

	//メッセージ
	if (mtimed_type == MTIMED2_DRUNK)
	{
		monster_type *m_ptr = &m_list[m_idx];
		int old_rank, new_rank;

		if (m_ptr->r_idx && m_ptr->maxhp && m_ptr->hp >= 0)
		{

			//モンスター泥酔度がMAXHPの25%を超えるごとに泥酔度ランク1加算
			old_rank = old_val * 4 / m_ptr->maxhp;
			new_rank = new_val * 4 / m_ptr->maxhp;

			if (old_rank != new_rank)
			{
				char m_name[120];
				monster_desc(m_name, m_ptr, 0L);

				if (!new_rank)
					msg_format(_("%^sの酔いが覚めた。", "%^s is no longer drunk."),m_name);
				else if (new_rank == 1)
				{
					if(new_rank > old_rank)
						msg_format(_("%^sは楽しそうにしている。", "%^s looks enjoyed."), m_name);
					else
						msg_format(_("%^sは酔いが覚めてきたようだ。", "%^s is sobering up."), m_name);
				}
				else if (new_rank == 2)
				{
					if (new_rank > old_rank)
						msg_format(_("%^sは酔ってフラフラだ。", "%^s is swaying drunkenly."), m_name);
					else
						msg_format(_("%^sは少し自分を取り戻した。", "%^s comes back to senses somewhat."), m_name);
				}
				else if (new_rank == 3)
				{
					if (new_rank > old_rank)
						msg_format(_("%^sはもうすぐ酔い潰れそうだ。", "%^s is about to pass out."), m_name);
					else
						msg_format(_("%^sは倒れそうになっていたが踏みとどまった。", "%^s was about to collapse, but manages to recover."), m_name);//←このメッセージはほぼ出ない
				}
				//4以上になると良い潰して倒す扱いになる
			}
		}

	}

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	return TRUE;

}

/*
 * Set "m_ptr->mtimed[MTIMED_CSLEEP]", notice observable changes
 */
/*:::睡眠判定？*/
bool set_monster_csleep(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_CSLEEP(m_ptr))
		{
			mproc_add(m_idx, MTIMED_CSLEEP);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_CSLEEP(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_CSLEEP);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_CSLEEP] = v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	if (r_info[m_ptr->r_idx].flags7 & RF7_HAS_LD_MASK) p_ptr->update |= (PU_MON_LITE);

	return TRUE;
}


/*
 * Set "m_ptr->mtimed[MTIMED_FAST]", notice observable changes
 */
bool set_monster_fast(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_FAST(m_ptr))
		{
			mproc_add(m_idx, MTIMED_FAST);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_FAST(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_FAST);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_FAST] = v;

	if (!notice) return FALSE;

	if ((p_ptr->riding == m_idx) && !p_ptr->leaving) p_ptr->update |= (PU_BONUS);

	return TRUE;
}


/*
 * Set "m_ptr->mtimed[MTIMED_SLOW]", notice observable changes
 */
bool set_monster_slow(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_SLOW(m_ptr))
		{
			mproc_add(m_idx, MTIMED_SLOW);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_SLOW(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_SLOW);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_SLOW] = v;

	if (!notice) return FALSE;

	if ((p_ptr->riding == m_idx) && !p_ptr->leaving) p_ptr->update |= (PU_BONUS);

	return TRUE;
}


/*
 * Set "m_ptr->mtimed[MTIMED_STUNNED]", notice observable changes
 */
bool set_monster_stunned(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_STUNNED(m_ptr))
		{
			mproc_add(m_idx, MTIMED_STUNNED);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_STUNNED(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_STUNNED);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_STUNNED] = v;

	return notice;
}


/*
 * Set "m_ptr->mtimed[MTIMED_CONFUSED]", notice observable changes
 */
bool set_monster_confused(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_CONFUSED(m_ptr))
		{
			mproc_add(m_idx, MTIMED_CONFUSED);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_CONFUSED(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_CONFUSED);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_CONFUSED] = v;

	return notice;
}


/*
 * Set "m_ptr->mtimed[MTIMED_MONFEAR]", notice observable changes
 */
bool set_monster_monfear(int m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_MONFEAR(m_ptr))
		{
			mproc_add(m_idx, MTIMED_MONFEAR);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_MONFEAR(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_MONFEAR);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_MONFEAR] = v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	return TRUE;
}


/*
 * Set "m_ptr->mtimed[MTIMED_INVULNER]", notice observable changes
 */
bool set_monster_invulner(int m_idx, int v, bool energy_need)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_INVULNER(m_ptr))
		{
			mproc_add(m_idx, MTIMED_INVULNER);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_INVULNER(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_INVULNER);
			if (energy_need && !p_ptr->wild_mode) m_ptr->energy_need += ENERGY_NEED();
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_INVULNER] = v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	return TRUE;
}





static u32b csleep_noise;

//モンスター一時ステータスのタイムアウト処理
//内部的10turn(ゲーム上1ターン)に一度呼ばれる
static void process_monsters_mtimed_aux(int m_idx, int mtimed_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int rlev = r_ptr->level;
	int recover_val = randint1(rlev / 20 + 1);//一部のバステの回復判定値　急回復があると倍

	if (r_info[m_ptr->r_idx].flags2 & RF2_REGENERATE) recover_val *= 2;

	switch (mtimed_idx)
	{
	case MTIMED_CSLEEP:
	{
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Assume does not wake up */
		bool test = FALSE;

		/* Hack -- Require proximity */
		if (m_ptr->cdis < AAF_LIMIT)
		{
			/* Handle "sensing radius" */
			if (m_ptr->cdis <= (is_pet(m_ptr) ? ((r_ptr->aaf > MAX_SIGHT) ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf))
			{
				/* We may wake up */
				test = TRUE;
			}

			/* Handle "sight" and "aggravation" */
			else if ((m_ptr->cdis <= MAX_SIGHT) && (player_has_los_bold(m_ptr->fy, m_ptr->fx)))
			{
				/* We may wake up */
				test = TRUE;
			}
		}

		if (test)
		{
			u32b notice = randint0(1024);

			/* Nightmare monsters are more alert */
			if (ironman_nightmare) notice /= 2;

			/* Hack -- See if monster "notices" player */
			if ((notice * notice * notice) <= csleep_noise)
			{
				/* Hack -- amount of "waking" */
				/* Wake up faster near the player */
				int d = (m_ptr->cdis < AAF_LIMIT / 2) ? (AAF_LIMIT / m_ptr->cdis) : 1;

				/* Hack -- amount of "waking" is affected by speed of player */
				d = (d * SPEED_TO_ENERGY(p_ptr->pspeed)) / 10;
				if (d < 0) d = 1;

				/* Monster wakes up "a little bit" */

				/* Still asleep */
				if (!set_monster_csleep(m_idx, MON_CSLEEP(m_ptr) - d))
				{
					/* Notice the "not waking up" */
					if (is_original_ap_and_seen(m_ptr))
					{
						/* Hack -- Count the ignores */
						if (r_ptr->r_ignore < MAX_UCHAR) r_ptr->r_ignore++;
					}
				}

				/* Just woke up */
				else
				{
					/* Notice the "waking up" */
					if (m_ptr->ml)
					{
						char m_name[80];

						/* Acquire the monster name */
						monster_desc(m_name, m_ptr, 0);

						/* Dump a message */
#ifdef JP
						msg_format("%^sが目を覚ました。", m_name);
#else
						msg_format("%^s wakes up.", m_name);
#endif
					}

					if (is_original_ap_and_seen(m_ptr))
					{
						/* Hack -- Count the wakings */
						if (r_ptr->r_wake < MAX_UCHAR) r_ptr->r_wake++;
					}
				}
			}
		}
		break;
	}

	case MTIMED_FAST:
		/* Reduce by one, note if expires */
		if (set_monster_fast(m_idx, MON_FAST(m_ptr) - 1))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sはもう加速されていない。", m_name);
#else
				msg_format("%^s is no longer fast.", m_name);
#endif
			}
		}
		break;

	case MTIMED_SLOW:
		/* Reduce by one, note if expires */
		//v1.1.90 朦朧と似た判定で回復する
		//if (set_monster_slow(m_idx, MON_SLOW(m_ptr) - 1))
		if (set_monster_slow(m_idx, (randint0(10000) <= rlev * rlev) ? 0 : MON_SLOW(m_ptr) - 1))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sはもう減速されていない。", m_name);
#else
				msg_format("%^s is no longer slow.", m_name);
#endif
			}
		}
		break;

	case MTIMED_STUNNED:
	{

		/* Recover from stun */
		if (set_monster_stunned(m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (MON_STUNNED(m_ptr) - 1)))
		{
			/* Message if visible */
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sは朦朧状態から立ち直った。", m_name);
#else
				msg_format("%^s is no longer stunned.", m_name);
#endif
			}
		}
		break;
	}

	case MTIMED_CONFUSED:
		/* Reduce the confusion */
		//v1.1.95 朦朧と同じ回復判定にした。こころの特技で混乱耐性を貫通できるようになったので。
		if (set_monster_confused(m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (MON_CONFUSED(m_ptr) - 1)))
	//	if (set_monster_confused(m_idx, MON_CONFUSED(m_ptr) - recover_val))
		{
			/* Message if visible */
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sは混乱から立ち直った。", m_name);
#else
				msg_format("%^s is no longer confused.", m_name);
#endif
			}
		}
		break;

	case MTIMED_MONFEAR:
		/* Reduce the fear */
		if (set_monster_monfear(m_idx, MON_MONFEAR(m_ptr) - recover_val))
		{
			/* Visual note */
			if (is_seen(m_ptr))
			{
				char m_name[80];
#ifndef JP
				char m_poss[80];

				/* Acquire the monster possessive */
				monster_desc(m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sは勇気を取り戻した。", m_name);
#else
				msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif
			}
		}
		break;

	case MTIMED_INVULNER:
		/* Reduce by one, note if expires */
		if (set_monster_invulner(m_idx, MON_INVULNER(m_ptr) - 1, TRUE))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sはもう無敵でない。", m_name);
#else
				msg_format("%^s is no longer invulnerable.", m_name);
#endif
			}
		}
		break;

	case MTIMED2_DEC_ATK:
		if (set_monster_timed_status_add(MTIMED2_DEC_ATK, m_idx, MON_DEC_ATK(m_ptr) - recover_val))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sの攻撃力が元に戻ったようだ。",
                            "The attack power of %s returns to normal."), m_name);
			}
		}
		break;
	case MTIMED2_DEC_DEF:
		if (set_monster_timed_status_add(MTIMED2_DEC_DEF, m_idx, MON_DEC_DEF(m_ptr) - recover_val))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sの防御力が元に戻ったようだ。",
                            "The defense power of %s returns to normal."), m_name);
			}
		}
		break;
	case MTIMED2_DEC_MAG:
		if (set_monster_timed_status_add(MTIMED2_DEC_MAG, m_idx, MON_DEC_MAG(m_ptr) - recover_val))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sの魔法力が元に戻ったようだ。",
                            "The magic power of %s returns to normal."), m_name);
			}
		}
		break;

	case MTIMED2_DRUNK:
		//アルコールの回復はHPの自然回復と同じにする
		recover_val = m_ptr->maxhp / 100;
		if (r_info[m_ptr->r_idx].flags2 & RF2_REGENERATE) recover_val *= 2;

		if (set_monster_timed_status_add(MTIMED2_DRUNK, m_idx, MON_DRUNK(m_ptr) - recover_val))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sは酔いを覚ましたようだ。",
                            "%^s sobers up."), m_name);
			}
		}
		break;

	//移動禁止　ここの他にもモンスターの行動のたびに解除判定が発生する
	case MTIMED2_NO_MOVE:
	if (set_monster_timed_status_add(MTIMED2_NO_MOVE, m_idx, MON_NO_MOVE(m_ptr) - 1))
	{
		if (is_seen(m_ptr))
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%^sは束縛を振り払った！", "%^s breaks free of restraints!"), m_name);
		}
	}
	break;

	case MTIMED2_BERSERK:
		if (set_monster_timed_status_add(MTIMED2_BERSERK, m_idx, MON_BERSERK(m_ptr) - recover_val))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%^sは激情が収まったようだ。", "%^s is no longer furious."), m_name);
			}
		}
		break;




	}
}


/*
 * Process the counters of monsters (once per 10 game turns)
 *
 * These functions are to process monsters' counters same as player's.
 */
/*:::モンスターの一時ステータスの解除カウント処理　寝てるモンスターが＠の隠密により起きるのもここ*/
//内部的10turn(ゲーム上1ターン)に一度呼ばれる
void process_monsters_mtimed(int mtimed_idx)
{
	int  i;
	s16b *cur_mproc_list = mproc_list[mtimed_idx];

	/* Hack -- calculate the "player noise" */
	if (mtimed_idx == MTIMED_CSLEEP) csleep_noise = (1L << (30 - p_ptr->skill_stl));

	/* Process the monsters (backwards) */
	for (i = mproc_max[mtimed_idx] - 1; i >= 0; i--)
	{
		/* Access the monster */
		process_monsters_mtimed_aux(cur_mproc_list[i], mtimed_idx);
	}
}

/*:::モンスターの加速、減速、無敵化を解除する*/
void dispel_monster_status(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	char         m_name[80];

	monster_desc(m_name, m_ptr, 0);
	if (set_monster_invulner(m_idx, 0, TRUE))
	{
#ifdef JP
		if (m_ptr->ml) msg_format("%sはもう無敵ではない。", m_name);
#else
		if (m_ptr->ml) msg_format("%^s is no longer invulnerable.", m_name);
#endif
	}
	if (set_monster_fast(m_idx, 0))
	{
#ifdef JP
		if (m_ptr->ml) msg_format("%sはもう加速されていない。", m_name);
#else
		if (m_ptr->ml) msg_format("%^s is no longer fast.", m_name);
#endif
	}
	if (set_monster_slow(m_idx, 0))
	{
#ifdef JP
		if (m_ptr->ml) msg_format("%sはもう減速されていない。", m_name);
#else
		if (m_ptr->ml) msg_format("%^s is no longer slow.", m_name);
#endif
	}

	//

}

/*:::敵による時間停止処理*/
bool process_the_world(int num, int who, bool vs_player)
{
	monster_type *m_ptr = &m_list[hack_m_idx];  /* the world monster */

	if(world_monster) return (FALSE);

	if(vs_player)
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);

		if (who == 1)
#ifdef JP
			msg_print("「『ザ・ワールド』！時は止まった！」");
#else
			msg_format("%s yells 'The World! Time has stopped!'", m_name);
#endif
		else if (who == 2)
#ifdef JP
			msg_print("「あなたの時間も私のもの・・・」");
#else
			msg_print("%s says 'Your time is mine...'");
#endif

		else if (who == 3)
#ifdef JP
			msg_print("「時よ！」");
#else
			msg_format("%s yells 'Time!'", m_name);
#endif
		else msg_print("hek!");

		msg_print(NULL);

		if(p_ptr->pclass == CLASS_SAKUYA || p_ptr->pclass == CLASS_KAGUYA && p_ptr->lev > 29)
		{
			msg_format(_("しかしあなたは%sの時間停止を妨害した！",
                        "You prevent %s from stopping time!"),m_name);
			return FALSE;
		}
	}

	/* This monster cast spells */
	world_monster = hack_m_idx;

	if (vs_player) do_cmd_redraw();

	while(num--)
	{
		if(!m_ptr->r_idx) break;
		process_monster(world_monster);

		reset_target(m_ptr);

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff */
		if (p_ptr->window) window_stuff();

		/* Delay */
		if (vs_player) Term_xtra(TERM_XTRA_DELAY, 500);
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	world_monster = 0;
	if (vs_player || (player_has_los_bold(m_ptr->fy, m_ptr->fx) && projectable(py, px, m_ptr->fy, m_ptr->fx)))
	{
#ifdef JP
		msg_print("「時は動きだす…」");
#else
		msg_print("You feel time flowing around you once more.");
#endif
		msg_print(NULL);
	}

	handle_stuff();

	return (TRUE);
}


void monster_gain_exp(int m_idx, int s_idx)
{
	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_race *s_ptr;
	int new_exp;

	/* Paranoia */
	if (m_idx <= 0 || s_idx <= 0) return;

	m_ptr = &m_list[m_idx];

	/* Paranoia -- Skip dead monsters */
	if (!m_ptr->r_idx) return;

	r_ptr = &r_info[m_ptr->r_idx];
	s_ptr = &r_info[s_idx];

	if (p_ptr->inside_battle) return;

	if (!r_ptr->next_exp) return;

	new_exp = s_ptr->mexp * s_ptr->level / (r_ptr->level + 2);
	if (m_idx == p_ptr->riding) new_exp = (new_exp + 1) / 2;
	if (!dun_level) new_exp /= 5;
	m_ptr->exp += new_exp;
	if (m_ptr->mflag2 & MFLAG2_CHAMELEON) return;

	if (m_ptr->exp >= r_ptr->next_exp)
	{
		char m_name[80];
		int old_hp = m_ptr->hp;
		int old_maxhp = m_ptr->max_maxhp;
		int old_r_idx = m_ptr->r_idx;
		byte old_sub_align = m_ptr->sub_align;

		/* Hack -- Reduce the racial counter of previous monster */
		real_r_ptr(m_ptr)->cur_num--;

		monster_desc(m_name, m_ptr, 0);
		m_ptr->r_idx = r_ptr->next_r_idx;

		/* Count the monsters on the level */
		real_r_ptr(m_ptr)->cur_num++;

		m_ptr->ap_r_idx = m_ptr->r_idx;
		r_ptr = &r_info[m_ptr->r_idx];

		if (r_ptr->flags1 & RF1_FORCE_MAXHP)
		{
			m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
		}
		else
		{
			m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
		}
		if (ironman_nightmare)
		{
			u32b hp = m_ptr->max_maxhp * 2L;

			m_ptr->max_maxhp = (s16b)MIN(30000, hp);
		}
		else if(difficulty == DIFFICULTY_EASY) m_ptr->max_maxhp = MAX(1,m_ptr->max_maxhp/2);
		else if(difficulty == DIFFICULTY_NORMAL) m_ptr->max_maxhp = MAX(1,m_ptr->max_maxhp/2 + m_ptr->max_maxhp/4);

		m_ptr->maxhp = m_ptr->max_maxhp;
		m_ptr->hp = old_hp * m_ptr->maxhp / old_maxhp;

		/* Extract the monster base speed */
		m_ptr->mspeed = get_mspeed(r_ptr);

		/* Sub-alignment of a monster */
		if (!is_pet(m_ptr) && !(r_ptr->flags3 & (RF3_ANG_CHAOS | RF3_ANG_COSMOS)))
			m_ptr->sub_align = old_sub_align;
		else
		{
			m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
			if (r_ptr->flags3 & RF3_ANG_CHAOS) m_ptr->sub_align |= SUB_ALIGN_EVIL;
			if (r_ptr->flags3 & RF3_ANG_COSMOS) m_ptr->sub_align |= SUB_ALIGN_GOOD;
		}

		m_ptr->exp = 0;

		if (is_pet(m_ptr) || m_ptr->ml)
		{
			if (!ignore_unview || player_can_see_bold(m_ptr->fy, m_ptr->fx))
			{
				if (p_ptr->image)
				{
					monster_race *hallu_race;

					do
					{
						hallu_race = &r_info[randint1(max_r_idx - 1)];
					}
					while (!hallu_race->name || (hallu_race->flags1 & RF1_UNIQUE));

#ifdef JP
					msg_format("%sは%sに進化した。", m_name, r_name + hallu_race->name);
#else
					msg_format("%^s evolved into %s.", m_name, r_name + hallu_race->name);
#endif
				}
				else
				{
#ifdef JP
					msg_format("%sは%sに進化した。", m_name, r_name + r_ptr->name);
#else
					msg_format("%^s evolved into %s.", m_name, r_name + r_ptr->name);
#endif
				}
			}

			if (!p_ptr->image) r_info[old_r_idx].r_xtra1 |= MR1_SINKA;

			/* Now you feel very close to this pet. */
			m_ptr->parent_m_idx = 0;
		}
		update_mon(m_idx, FALSE);
		lite_spot(m_ptr->fy, m_ptr->fx);
	}
	if (m_idx == p_ptr->riding) p_ptr->update |= PU_BONUS;
}
