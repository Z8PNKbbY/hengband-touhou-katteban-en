#include "angband.h"

#define MAX_SNIPE_POWERS 16

typedef struct snipe_power snipe_power;
struct snipe_power
{
	int     min_lev;
	int     mana_cost;
	const char *name;
};

static const char *snipe_tips[MAX_SNIPE_POWERS] =
{
#ifdef JP
	"精神を集中する。射撃の威力、精度が上がり、高度な射撃術が使用できるようになる。",
	"光る矢を放つ。光に弱いモンスターに威力を発揮する。",
	"射撃を行った後、短距離の瞬間移動を行う。",
	"軌道上の罠をすべて無効にする低空飛行の矢を放つ。",
	"火炎属性の矢を放つ。",
	"壁を粉砕する矢を放つ。岩でできたモンスターと無生物のモンスターに威力を発揮する。",
	"冷気属性の矢を放つ。",
	"敵を突き飛ばす矢を放つ。",
	"複数の敵を貫通する矢を放つ。",
	"善良なモンスターに威力を発揮する矢を放つ。",
	"邪悪なモンスターに威力を発揮する矢を放つ。",
	"当たると爆発する矢を放つ。",
	"2回射撃を行う。",
	"電撃属性の矢を放つ。",
	"敵の急所にめがけて矢を放つ。成功すると敵を一撃死させる。失敗すると1ダメージ。",
	"全てのモンスターに高威力を発揮する矢を放つ。反動による副次効果を受ける。",
#else
	"Concentrate your mind fot shooting.",
	"Shot an allow with brightness.",
	"Blink after shooting.",
	"Shot an allow able to shatter traps.",
	"Deals extra damege of fire.",
	"Shot an allow able to shatter rocks.",
	"Deals extra damege of ice.",
	"An allow rushes away a target.",
	"An allow pierces some monsters.",
	"Deals more damage to good monsters.",
	"Deals more damage to evil monsters.",
	"An allow explodes when it hits a monster.",
	"Shot allows twice.",
	"Deals extra damege of lightning.",
	"Deals quick death or 1 damage.",
	"Deals great damage to all monsters, and some side effects to you.",
#endif
};

snipe_power snipe_powers[MAX_SNIPE_POWERS] =
{
	/* Level gained,  cost,  name */
#ifdef JP
	{  1,  0,  "精神集中" },
	{  2,  1,  "フラッシュアロー" },
	{  3,  1,  "シュート＆アウェイ" },
	{  5,  1,  "解除の矢" },
	{  8,  2,  "火炎の矢" },
	{ 10,  2,  "岩砕き" },
	{ 13,  2,  "冷気の矢" },
	{ 18,  2,  "烈風弾"},
	{ 22,  3,  "貫通弾" },
	{ 25,  4,  "邪念弾"},
	{ 26,  4,  "破魔矢" },
	{ 30,  3,  "爆発の矢"},
	{ 32,  4,  "ダブルショット" },
	{ 36,  3,  "プラズマボルト" },
	{ 40,  3,  "ニードルショット" },
	{ 48,  7,  "セイントスターアロー" },
#else
	{  1,  0,  "Concentration" },
	{  2,  1,  "Flush Arrow" },
	{  3,  1,  "Shoot & Away" },
	{  5,  1,  "Disarm Shot" },
	{  8,  2,  "Fire Shot" },
	{ 10,  2,  "Shatter Arrow" },
	{ 13,  2,  "Ice Shot" },
	{ 18,  2,  "Rushing Arrow"},
	{ 22,  3,  "Piercing Shot" },
	{ 25,  4,  "Evil Shot"},
	{ 26,  4,  "Holy Shot" },
	{ 30,  3,  "Missile"},
	{ 32,  4,  "Double Shot" },
	{ 36,  3,  "Plasma Bolt" },
	{ 40,  3,  "Needle Shot" },
	{ 48,  7,  "Saint Stars Arrow" },
#endif
};


static bool snipe_concentrate(void)
{
	if ((int)p_ptr->concent < (2 + (p_ptr->lev + 5) / 10)) p_ptr->concent++;

#ifdef JP
	msg_format("集中した。(集中度 %d)", p_ptr->concent);
#else
	msg_format("You concentrate deeply. (lvl %d)", p_ptr->concent);
#endif

	reset_concent = FALSE;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	p_ptr->redraw |= (PR_STATUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	return (TRUE);
}

///mod141116 少しルーチン変更
void reset_concentration(bool msg)
{
	if(!(CLASS_USE_CONCENT)){ msg_print(_("ERROR:登録していない職業でreset_concentration()が呼ばれた",
                                        "ERROR: reset_concentration() called for an unregistered class"));return;}
	if(!p_ptr->concent) return;

	if (msg)
	{
		if(p_ptr->pclass == CLASS_KISUME)
			msg_print(_("あなたは地面まで降下した。", "You lower to the ground."));
		else if(p_ptr->pclass == CLASS_YUGI)
			msg_print(_("必殺の構えを解いた。","You resume your normal stance."));
		else if(p_ptr->pclass == CLASS_SANAE)
			msg_print(_("詠唱が途切れた。", "You stop chanting."));
		else if(p_ptr->pclass == CLASS_MEIRIN)
			msg_print(_("連撃が途切れた。", "You stop your barrage."));
		else if(p_ptr->pclass == CLASS_BENBEN || p_ptr->pclass == CLASS_YATSUHASHI)
			msg_print(_("演奏が途切れてしまった。", "You stop your performance."));
		else if(p_ptr->pclass == CLASS_RAIKO)
			msg_print(_("ビートが止んだ。", "The beat has stopped."));
		else if(p_ptr->pclass == CLASS_SOLDIER)
			msg_print(_("集中が途切れた。", "You lose your concentration."));
		else if (p_ptr->pclass == CLASS_ENOKO)
			msg_print(_("集中が途切れた。", "You lose your concentration."));
		//v1.1.14 依姫祇園様の剣リセット処理
		else if(p_ptr->pclass == CLASS_YORIHIME)
		{
			//他の神が降りていたときそちらがリセットされないよう例外処理する必要がある。
			if(p_ptr->kamioroshi & KAMIOROSHI_GION)
			{
				//祇園様だけのとき普通に神降ろしリセット
				if(p_ptr->kamioroshi == KAMIOROSHI_GION)
				{
					p_ptr->kamioroshi_count = 0;
					set_kamioroshi(0,0);
				}
				//他の神も降りているときメッセージ出して祇園様のフラグだけ消去
				else
				{
					p_ptr->kamioroshi &= ~(KAMIOROSHI_GION);
					if(p_ptr->paralyzed || (p_ptr->stun >= 100) || p_ptr->alcohol >= DRANK_4 && !(p_ptr->muta2 & MUT2_ALCOHOL))
						msg_print(_("祇園様の剣が地面から抜けた。", "The Sword of Gion comes out of the ground."));
					else
						msg_print(_("祇園様の剣を地面から引き抜いた。", "You pull the Sword of Gion out of the ground."));
				}
			}
		}
		else if (p_ptr->pclass == CLASS_RESEARCHER)
		{
			if(p_ptr->magic_num1[0] == CONCENT_KANA)
				msg_print(_("気合いが散った。", "You stop focusing."));
			else
				msg_print(_("魔力の集中が元に戻った。", "You are no longer concentrating mana."));

		}
		else
			msg_print(_("ERROR:この職業の集中妨害メッセージが設定されていない",
                        "ERROR: No message set for concentration interrupt message for this class"));
	}

	//詠唱モードをリセット
	if(p_ptr->pclass == CLASS_SANAE)
		p_ptr->magic_num1[0] = 0;

	p_ptr->concent = 0;
	reset_concent = FALSE;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_STATUS);
	///mod141116 キスメ用に追記
	p_ptr->window |= (PW_EQUIP);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);
}

/*:::スナイパーの集中によるダメージ加算*/
///class スナイパー
int boost_concentration_damage(int tdam)
{
	tdam *= (10 + p_ptr->concent);
	tdam /= 10;

	return (tdam);
}

void display_snipe_list(void)
{
	int             i;
	int             y = 1;
	int             x = 1;
	int             plev = p_ptr->lev;
	snipe_power     spell;
	char            psi_desc[80];

	/* Display a list of spells */
	prt("", y, x);
#ifdef JP
	put_str("名前", y, x + 5);
	put_str("Lv   MP", y, x + 35);
#else
	put_str("Name", y, x + 5);
	put_str("Lv Mana", y, x + 35);
#endif

	/* Dump the spells */
	for (i = 0; i < MAX_SNIPE_POWERS; i++)
	{
		/* Access the available spell */
		spell = snipe_powers[i];
		if (spell.min_lev > plev) continue;
		if (spell.mana_cost > (int)p_ptr->concent) continue;

		/* Dump the spell */
		sprintf(psi_desc, "  %c) %-30s%2d %4d",
			I2A(i), spell.name, spell.min_lev, spell.mana_cost);

		Term_putstr(x, y + i + 1, -1, TERM_WHITE, psi_desc);
	}
	return;
}


/*
 * Allow user to choose a mindcrafter power.
 *
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
/*:::スナイパー専用　技を選択する*/
static int get_snipe_power(int *sn, bool only_browse)
{
	int             i;
	int             num = 0;
	int             y = 1;
	int             x = 20;
	int             plev = p_ptr->lev;
	int             ask;
	char            choice;
	char            out_val[160];
#ifdef JP
	cptr            p = "射撃術";
#else
	cptr            p = "power";
#endif
	snipe_power     spell;
	bool            flag, redraw;

#ifdef ALLOW_REPEAT /* TNB */

	repeat_push(*sn);

	/* Assume cancelled */
	*sn = (-1);

	/* Repeat previous command */
	/* Get the spell, if available */
	if (repeat_pull(sn))
	{
		/* Verify the spell */
		if ((snipe_powers[*sn].min_lev <= plev) && (snipe_powers[*sn].mana_cost <= (int)p_ptr->concent))
		{
			/* Success */
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT -- TNB */

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	for (i = 0; i < MAX_SNIPE_POWERS; i++)
	{
		if ((snipe_powers[i].min_lev <= plev) &&
			((only_browse) || (snipe_powers[i].mana_cost <= (int)p_ptr->concent)))
		{
			num = i;
		}
	}

	/* Build a prompt (accept all spells) */
	if (only_browse)
	{
#ifdef JP
		(void)strnfmt(out_val, 78, "(%^s %c-%c, '*'で一覧, ESC) どの%sについて知りますか？",
#else
		(void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) Use which %s? ",
#endif
			      p, I2A(0), I2A(num), p);
	}
	else
	{
#ifdef JP
		(void)strnfmt(out_val, 78, "(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？",
#else
		(void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) Use which %s? ",
#endif
			  p, I2A(0), I2A(num), p);
	}

	/* Get a spell from the user */
	choice = always_show_list ? ESCAPE : 1;
	while (!flag)
	{
		if(choice == ESCAPE) choice = ' ';
		else if( !get_com(out_val, &choice, FALSE) )break;

		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				char psi_desc[80];

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				if (!only_browse) screen_save();

				/* Display a list of spells */
				prt("", y, x);
#ifdef JP
				put_str("名前", y, x + 5);
				if (only_browse) put_str("Lv   集中度", y, x + 35);
#else
				put_str("Name", y, x + 5);
				if (only_browse) put_str("Lv Pow", y, x + 35);
#endif

				/* Dump the spells */
				for (i = 0; i < MAX_SNIPE_POWERS; i++)
				{
					Term_erase(x, y + i + 1, 255);

					/* Access the spell */
					spell = snipe_powers[i];
					if (spell.min_lev > plev) continue;
					if (!only_browse && (spell.mana_cost > (int)p_ptr->concent)) continue;

					/* Dump the spell --(-- */
					if (only_browse)
						sprintf(psi_desc, "  %c) %-30s%2d %4d",
							I2A(i), spell.name,	spell.min_lev, spell.mana_cost);
					else
						sprintf(psi_desc, "  %c) %-30s", I2A(i), spell.name);
					prt(psi_desc, y + i + 1, x);
				}

				/* Clear the bottom line */
				prt("", y + i + 1, x);
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				if (!only_browse) screen_load();
			}

			/* Redo asking */
			continue;
		}

		/* Note verify */
		ask = isupper(choice);

		/* Lowercase */
		if (ask) choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		/* Totally Illegal */
		if ((i < 0) || (i > num) ||
			(!only_browse &&(snipe_powers[i].mana_cost > (int)p_ptr->concent)))
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = snipe_powers[i];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
#ifdef JP
			(void) strnfmt(tmp_val, 78, "%sを使いますか？", snipe_powers[i].name);
#else
			(void)strnfmt(tmp_val, 78, "Use %s? ", snipe_powers[i].name);
#endif

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw && !only_browse) screen_load();

	/* Show choices */
	p_ptr->window |= (PW_SPELL);

	/* Window stuff */
	window_stuff();

	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = i;

#ifdef ALLOW_REPEAT /* TNB */

	repeat_push(*sn);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}


int tot_dam_aux_snipe (int mult, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool seen = is_seen(m_ptr);

	switch (snipe_type)
	{
	case SP_LITE:
///mod131231 モンスターフラグ変更
		if (r_ptr->flagsr & (RFR_HURT_LITE))
		{
			int n = 20 + p_ptr->concent;
			if (seen) r_ptr->r_flagsr |= (RFR_HURT_LITE);
			if (mult < n) mult = n;
		}
		break;
	case SP_FIRE:
		if (r_ptr->flagsr & RFR_IM_FIRE)
		{
			if (seen) r_ptr->r_flagsr |= RFR_IM_FIRE;
		}
		else
		{
			int n = 15 + (p_ptr->concent * 3);
			if (mult < n) mult = n;
		}
		break;
	case SP_COLD:
		if (r_ptr->flagsr & RFR_IM_COLD)
		{
			if (seen) r_ptr->r_flagsr |= RFR_IM_COLD;
		}
		else
		{
			int n = 15 + (p_ptr->concent * 3);
			if (mult < n) mult = n;
		}
		break;
	case SP_ELEC:
		if (r_ptr->flagsr & RFR_IM_ELEC)
		{
			if (seen) r_ptr->r_flagsr |= RFR_IM_ELEC;
		}
		else
		{
			int n = 18 + (p_ptr->concent * 4);
			if (mult < n) mult = n;
		}
		break;
	case SP_KILL_WALL:
		///mod131231 モンスターフラグ変更 岩石妖怪弱点RF3からRFRへ
		if (r_ptr->flagsr & RFR_HURT_ROCK)
		{
			int n = 15 + (p_ptr->concent * 2);
			if (seen) r_ptr->r_flagsr |= RFR_HURT_ROCK;
			if (mult < n) mult = n;
		}
		else if (r_ptr->flags3 & RF3_NONLIVING)
		{
			int n = 15 + (p_ptr->concent * 2);
			if (seen) r_ptr->r_flags3 |= RF3_NONLIVING;
			if (mult < n) mult = n;
		}
		break;
	case SP_EVILNESS:
		///mod131231 モンスターフラグ変更 ヘルファイア弱点RF3からRFRへ
		if (r_ptr->flagsr & RFR_HURT_HELL)
		{
			int n = 15 + (p_ptr->concent * 4);
			if (seen) r_ptr->r_flagsr |= RFR_HURT_HELL;
			if (mult < n) mult = n;
		}
		break;
	case SP_HOLYNESS:
		///mod131231 モンスターフラグ変更 閃光弱点RF3からRFRへ
		if (r_ptr->flagsr & RFR_HURT_HOLY)
		{
			int n = 12 + (p_ptr->concent * 3);
			if (seen) r_ptr->r_flagsr |= RFR_HURT_HOLY;
			if (r_ptr->flagsr & (RFR_HURT_LITE))
			{
				n += (p_ptr->concent * 3);
				if (seen) r_ptr->r_flagsr |= (RFR_HURT_LITE);
			}
			if (mult < n) mult = n;
		}
		break;
	case SP_FINAL:
		if (mult < 50) mult = 50;
		break;
	}

	return (mult);
}

/*
 * do_cmd_cast calls this function if the player's class
 * is 'mindcrafter'.
 */
///mod140308 弓枠廃止処理関連
static bool cast_sniper_spell(int spell)
{
	//object_type *o_ptr = &inventory[INVEN_BOW];
	object_type *o_ptr;
	int dummy;

	o_ptr = &inventory[INVEN_PACK + shootable(&dummy)];

	if (o_ptr->tval != TV_CROSSBOW && o_ptr->tval != TV_BOW )
	{
#ifdef JP
		msg_print("弓を装備していない！");
#else
		msg_print("You wield no bow!");
#endif
		return (FALSE);
	}

	/* spell code */
	switch (spell)
	{
	case 0: /* Concentration */
		if (!snipe_concentrate()) return (FALSE);
		energy_use = 100;
		return (TRUE);
	case 1: snipe_type = SP_LITE; break;
	case 2: snipe_type = SP_AWAY; break;
	case 3: snipe_type = SP_KILL_TRAP; break;
	case 4: snipe_type = SP_FIRE; break;
	case 5: snipe_type = SP_KILL_WALL; break;
	case 6: snipe_type = SP_COLD; break;
	case 7: snipe_type = SP_RUSH; break;
	case 8: snipe_type = SP_PIERCE; break;
	case 9: snipe_type = SP_EVILNESS; break;
	case 10: snipe_type = SP_HOLYNESS; break;
	case 11: snipe_type = SP_EXPLODE; break;
	case 12: snipe_type = SP_DOUBLE; break;
	case 13: snipe_type = SP_ELEC; break;
	case 14: snipe_type = SP_NEEDLE; break;
	case 15: snipe_type = SP_FINAL; break;
	default:
#ifdef JP
		msg_print("なに？");
#else
		msg_print("Zap?");
#endif
	}

	command_cmd = 'f';
	do_cmd_fire();
	snipe_type = 0;

	return (is_fired);
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'mindcrafter'.
 */
void do_cmd_snipe(void)
{
	int             n = 0;
	bool            cast;


	/* not if confused */
	if (p_ptr->confused)
	{
#ifdef JP
		msg_print("混乱していて集中できない！");
#else
		msg_print("You are too confused!");
#endif
		return;
	}

	/* not if hullucinated */
	if (p_ptr->image)
	{
#ifdef JP
		msg_print("幻覚が見えて集中できない！");
#else
		msg_print("You are too hallucinated!");
#endif
		return;
	}

	/* not if stuned */
	if (p_ptr->stun)
	{
#ifdef JP
		msg_print("頭が朦朧としていて集中できない！");
#else
		msg_print("You are too stunned!");
#endif
		return;
	}

	/* get power */
	if (!get_snipe_power(&n, FALSE)) return;

	sound(SOUND_SHOOT);

	/* Cast the spell */
	cast = cast_sniper_spell(n);

	if (!cast) return;
#if 0
	/* Take a turn */
	energy_use = 100;
#endif
	/* Redraw mana */
	p_ptr->redraw |= (PR_HP | PR_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);
	p_ptr->window |= (PW_SPELL);
}

/*
 * do_cmd_cast calls this function if the player's class
 * is 'mindcrafter'.
 */
void do_cmd_snipe_browse(void)
{
	int n = 0;
	int j, line;
	char temp[62 * 4];

	screen_save();

	while(1)
	{
		/* get power */
		if (!get_snipe_power(&n, TRUE))
		{
			screen_load();
			return;
		}

		/* Clear lines, position cursor  (really should use strlen here) */
		Term_erase(12, 22, 255);
		Term_erase(12, 21, 255);
		Term_erase(12, 20, 255);
		Term_erase(12, 19, 255);
		Term_erase(12, 18, 255);

		roff_to_buf(snipe_tips[n], 62, temp, sizeof(temp));
		for(j = 0, line = 19; temp[j]; j += (1 + strlen(&temp[j])))
		{
			prt(&temp[j], line, 15);
			line++;
		}
	}
}
