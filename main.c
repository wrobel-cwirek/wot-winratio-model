#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define GAME_TYPE long
// orig: 30000 test: 300 200000
#define PLAYERS 200000
// orig: 20000000 test: 2000 1000000000
#define GAMES 20000000
// orig: 100000 test: 100
#define DISPLAY_PROGRESS 100000
#define TEAM_SIZE 15

// constants for W model
#define TANK_POWER_FACTOR 10
#define SKILL_ADJUSTMENT 20

// constants for cellural W model
#define TANK_BONUS 10
#define RANDOM_BONUS 10
#define PLAYER_RESIGN_CHANCE 1000
#define MAX_GAME_LENGTH 100
#define SAME_SKILL_DOUBLE_KILL 1

#define MODE_WINS 1
#define MODE_SKILL 2

#define LOG 0

typedef struct {
    int id;
    int skill;
    GAME_TYPE wins;
    GAME_TYPE looses;
} Player;

int pPlayers = PLAYERS;
GAME_TYPE pGames = GAMES;
int pTankBonus = TANK_BONUS;
int pRandomBonus = RANDOM_BONUS;
int pDisplayProgress = 1;

GAME_TYPE vGamesTooLong = 0;
GAME_TYPE vGamesDraw = 0;

long lrand()
{
        return rand() << 16 | rand();
}

void logt(char* text) {
	if(LOG != 0) {
		printf("LOG: %s\n", text);
	}
}
void logp(char* text, int arg1) {
	if(LOG != 0) {
		printf("LOG: %s %d\n", text, arg1);
	}
}

int is_in(Player* what, Player** where) {
    int i;
    for (i = 0; i < TEAM_SIZE; ++i) {
        if (what == where[i]) return 1;
        if (where[i] == NULL) return 0;
    }
    return 0;
}

int prepare_team(Player* team[], Player* other_team[], Player* all_players){
	int skill=0, fill = 0;
	Player* p = NULL;

	memset(team, 0, sizeof(team));
	
	while (fill < TEAM_SIZE) {
            p = &(all_players[lrand() % pPlayers]);

            if (!is_in(p, team) && (other_team == NULL || !is_in(p, other_team))) {
                team[fill] = p;
                ++fill;
                skill += p->skill;
            }

       }
	return skill;
}

void update_teams(Player* teamA[], int skillA, Player* teamB[], int skillB) {
	int j;
	 if (skillA > skillB) {
            for (j = 0; j < TEAM_SIZE; ++j) {
                ++(teamA[j]->wins);
                ++(teamB[j]->looses);
            }
        } else if (skillA < skillB) {
            for (j = 0; j < TEAM_SIZE; ++j) {
                ++(teamB[j]->wins);
                ++(teamA[j]->looses);
            }
        } else {
            for (j = 0; j < TEAM_SIZE; ++j) {
                ++(teamB[j]->looses);
                ++(teamA[j]->looses);
            }
        }
}

void simulate_by_jakub(Player* teamA[], int skillA, Player* teamB[], int skillB) {
	update_teams(teamA, skillA, teamB, skillB);
}

int get_tank_for_player(int i) {
// tank selection
	int tank;
		if(i < 2) tank = 5; 			// 2 top tanks
		else if(i < 5) tank  = 4; 		// 3 top tank - 1
		else if(i < 10) tank  = 3;         // 5 top tank - 2
		else if(i < 13) tank  = 2;         // 3 top tank - 3
		else tank = 1;			// 2 top tank - 4
	return tank;
}


int calculate_skill_by_wriothesley(Player* team[]){
	long out = 0, i, tank;
	for(i = 0; i < TEAM_SIZE; ++i) {
		
		tank = get_tank_for_player(i);
		
		// influance of tank is lower for skilled players, but higher for not skilled players
		// weak player gain more power if is in strong tank, while strong player gain less power from being in it
		// however strong player adds to a team more from his skill
		// skill in 5 tier tank for skill 0 player = 5 * 20 + 0= 100
		// skill in 5 tier tank for skill 100 player = 5 * 10 + 100 = 150
		// skill in 1 tier tank for skill 0 player = 1 * 20 + 0 = 20
		// skill in 1 tier tank for skill 100 player = 1 * 10 + 100 = 110
		// still noob in strong tank looses with great player
		// (examples for TANK_POWER_FACTOR = 20 and SKILL_ADJUSTMENT = 10
		
		out += tank * (TANK_POWER_FACTOR - (int)(team[i]->skill/SKILL_ADJUSTMENT))  + team[i]->skill;
	}
	return out;
}

int find_next_alive_new(int alive, int skip){
	if(alive == 0) return -1;
	return skip % alive;
}

void init_tanks(int tanks[]) {
	int i;
	for(i = 0; i < TEAM_SIZE; ++i) {
		tanks[i] = rand() % 3;
	}
}

void switchDead(Player* team[], int dead, int alive) {
	Player *p = team[alive];
	team[alive] = team[dead];
	team[dead] = p;
}

// -1 - teamA won
// 1 - teamB won
// 0 - noone won
int calculate_by_wriothesley_cellural_model(Player* teamA[], Player* teamB[]){
	int out = 0, i, tank;

	int countA = TEAM_SIZE;
	int countB = TEAM_SIZE;

	int tanksA[TEAM_SIZE]; // tanks can be paper = 0, scisors = 1, rock = 2
	int tanksB[TEAM_SIZE];
	// random tanks
	init_tanks(tanksA);
	init_tanks(tanksB);

	int count = 0;

	
  while(1 == 1) {
	++count	;
	if(countA <= 0 || countB  <= 0) break;
	if(count > MAX_GAME_LENGTH) {
		++vGamesTooLong;
		break;
	}
	for(i = 0; i < TEAM_SIZE; ++i) {

		if(countA <=0 || countB <= 0) break;

		int indexA = find_next_alive_new(countA, i);
		int indexB = find_next_alive_new(countB, i);

		if(indexA < 0 || indexB < 0) break;

		// getting tanks
		int tankA = tanksA[indexA];
		int tankB = tanksB[indexB];
		
		// getting skill
		int skillA = teamA[indexA]->skill;
		int skillB = teamB[indexB]->skill;
		
		// counting tank bonus	
		// if same tanks - skill and random only wins - else - better tank gets bonus
		if(tankA != tankB) {
			if(tankA > tankB && ((tankA != 2 && tankB != 0)||(tankA == 0 && tankB == 2))){ // team A has winning tank
				skillA += pTankBonus;
			} else {
				skillB += pTankBonus;  // team B has winning tank
			}
		}
		// adding random bonus
		skillA += (pRandomBonus <= 0) ? 0 : rand()%pRandomBonus;
		skillB += (pRandomBonus <= 0) ? 0 : rand()%pRandomBonus;

		if(skillA > skillB){
 			//statusB[indexB] = 1;
			--countB;
			switchDead(teamB, indexB, countB);
		} else if(skillB > skillA){
			//statusA[indexA] = 1;
			--countA;
			switchDead(teamA, indexA, countA);
		} else {
			if(SAME_SKILL_DOUBLE_KILL != 0) {
			--countB;
			switchDead(teamB, indexB, countB);	
			--countA;
			switchDead(teamA, indexA, countA);			}
		}

	}
  }

	if(countA == countB){
		++vGamesDraw;
		return 0;
	}
	else if(countA > countB) return -1;
	else return 1;
}

void simulate_by_wriothesley_cellural_model(Player* teamA[], Player* teamB[]) {
	int j;
	int skillA = 0;
	int skillB = 0;
	
	int res = calculate_by_wriothesley_cellural_model(teamA, teamB);
	
	if(res < 0){
		skillA = 1;
	} else if (res > 0){
		skillB = 1;
	}
	
	update_teams(teamA, skillA, teamB, skillB);
}


void simulate_by_wriothesley(Player* teamA[], Player* teamB[]) {
	int j;
	
	int skillA = calculate_skill_by_wriothesley(teamA);
	int skillB = calculate_skill_by_wriothesley(teamB);

	update_teams(teamA, skillA, teamB, skillB);
}

int get_random(int min, int max) {
	return rand() % (max - min) + min; 
}

int get_skill_wby_wriothesley(int who) {
	
	// alternative skill distribution (more like normal)
	// we try to divide the population to groups
	// 10 % has skill from 90 to 99
	// 20% has skill from 70 to 80
	// 40% has skill from 30 to 69
	// 20% has skill from 10 to 29
	// 10 % has skill from 0 to 9
	// decide 
	int who_am_I = who * 100 / (double) pPlayers;
	if(who_am_I < 10) return get_random(0, 10);
	if(who_am_I < 30) return get_random(10, 30);
	if(who_am_I < 70) return get_random(30, 70);
	if(who_am_I < 90) return get_random(70, 90);
	return                   get_random(90, 100);
}

int get_skill_by_jakub(int who){
	return rand() % 100;
}

int display(int mode, char* description, Player players[]){
	
	GAME_TYPE win_ratio[101] = {0};
	GAME_TYPE i;

	int count = 0;

    printf("%s\n",description);
    for (i = 0; i < pPlayers; ++i) {
	if(mode == MODE_WINS) {
        	if ((players[i].wins + players[i].looses) > 0) {
            		++(win_ratio[(int) (100 * players[i].wins / (players[i].wins + players[i].looses))]);
			++count;
		}
	} else if(mode == MODE_SKILL) {
		if(players[i].skill >= 0) {
			++(win_ratio[players[i].skill]);
			++count;
		}
	}
    }

    for (i = 0; i < 101; ++i) {
        printf("% 3d%%: % 4ld\t\n", i, win_ratio[i]);
    }
	printf("All players: %d, used for calculation: %d\n", pPlayers, count);
}

char* prepare_time(char* out, long start, GAME_TYPE done, GAME_TYPE total) {
	long current = time(NULL);
	
	long total_time = current - start;
	long hours = total_time / 60 / 60;
	long tmp = total_time - hours * 60 * 60;
	long minutes = tmp / 60;
	long seconds = tmp - minutes * 60;
	
	if(done <= 0 || total <= 0) {
		sprintf(out, "%ld seconds (%02ld:%02ld:%02ld)",total_time,hours,minutes,seconds);
	} else {
		double x = done / (double) total;
		long expected = (long)(total_time / x);
		long ehours = expected / 60 / 60;
		long etmp = expected  - ehours * 60 * 60;
		long eminutes = etmp / 60;
		long eseconds = etmp - eminutes * 60;

		long left = expected - total_time;
		long lhours = left / 60 / 60;
		long ltmp = left - lhours * 60 * 60;
		long lminutes = ltmp / 60;
		long lseconds = ltmp - lminutes * 60;

		sprintf(out, "%ld seconds (%02ld:%02ld:%02ld) - expected %ld seconds (%02ld:%02ld:%02ld) - left %ld seconds (%02ld:%02ld:%02ld)",total_time,hours,minutes,seconds,expected,ehours,eminutes,eseconds,left,lhours,lminutes,lseconds);	
	}
	return out;
}


int main0() {
	srand(time(NULL));
	long max = 0, min = 2147450833,i = 0;
	for(; i < pPlayers; ++i) {
		long r = lrand();
		if(max < r) max = r;
		if(min > r) min = r;
	}
	printf("Max %ld, min %d\n", max, min);
}

int main1() {
	Player* tmp = NULL;
	tmp = (Player*) malloc(pPlayers * sizeof(Player));
	memset(tmp,0, pPlayers * sizeof(Player));
	Player x = tmp[0];
	free(tmp);
	return 0;
}

void display_paramters(){
	printf("Players=%d - Games=%ld - TankBonus=%d - RandomBonus=%d - DisplayProgress=%d\n", pPlayers ,pGames ,pTankBonus, pRandomBonus,pDisplayProgress );
}

#define PARAMS_HELP1 "Expecting 5 parameters: number_of_players (int >= 30), number_of_games (long > 0), tank_bonus (int >= 0), random_bonus (int >= 0), display_progres (0 or 1).\nUsing default values\n"
#define PARAMS_HELP2 "Incorrect one of 5 parameters: number_of_players (int >= 30), number_of_games (long > 0), tank_bonus (int >= 0), random_bonus (int >= 0), display_progres (0 or 1).\nUsing default values\n"

void load_paramters(int argc, const char* argv[]){
	logp("load_paramters ",argc);
	if(argc != 6 ) {
		printf(PARAMS_HELP1);
		return;
	}
	int _pPlayers = pPlayers;
	GAME_TYPE _pGames = GAMES;
	int _pTankBonus = TANK_BONUS;
	int _pRandomBonus = RANDOM_BONUS;
	int _pDisplayProgress = 1;

	_pPlayers = atoi(argv[1]);
	_pGames = atol(argv[2]);
	_pTankBonus = atoi(argv[3]);
	_pRandomBonus = atoi(argv[4]);
	_pDisplayProgress = atoi(argv[5]);
	
	if(
	_pPlayers < 30 ||
	_pGames <=0 ||
	_pTankBonus < 0||
	_pRandomBonus < 0 ||
	(_pDisplayProgress < 0 || _pDisplayProgress > 1)) {
		printf("Provided parameters:\n");
		printf("Players %d\n",_pPlayers);
		printf("Games %ld\n",_pGames);
		printf("Tank bonus %d\n",_pTankBonus);
		printf("Random bonus %d\n",_pRandomBonus);
		printf("Display progress %d\n",_pDisplayProgress);
		printf(PARAMS_HELP2);
		return;
	}
	pPlayers = _pPlayers;
	pGames = _pGames;
	pTankBonus = _pTankBonus;
	pRandomBonus = _pRandomBonus;
	pDisplayProgress = _pDisplayProgress;


}

int main(int argc, const char* argv[]) {

	load_paramters(argc, argv);

	Player* players = NULL;
	players = (Player*) malloc(pPlayers * sizeof(Player));
	if(players == NULL) {
		printf("Error allocating memory\n");
	}

    //Player players[PLAYERS] = {0};
    char time_buffer[400];
	long start = time(NULL);
	
    srand(time(NULL));

    GAME_TYPE i;

    for (i = 0; i < pPlayers; ++i) {
        players[i].id = i;
	 players[i].skill = get_skill_by_jakub(i);
        //players[i]->skill = get_skill_wby_wriothesley(i);
    }

    Player* teamA[TEAM_SIZE];
    Player* teamB[TEAM_SIZE];

    int skillA = 0, skillB = 0;
	display_paramters();
    for (i = 0; i < pGames; ++i) {
	if((pDisplayProgress != 0) && ((i % DISPLAY_PROGRESS) == 0)) {
		prepare_time(time_buffer,start,i,pGames);
		printf("Progress... done %d of %d - %lf%% - time of running %s\n",i,pGames,(double)i/(double)pGames*100,time_buffer);
	}

	skillA = prepare_team(teamA, NULL, players);
	skillB = prepare_team(teamB, teamA, players);

	 //simulate_by_jakub(teamA, skillA, teamB, skillB);
      	//simulate_by_wriothesley(teamA, teamB);
	simulate_by_wriothesley_cellural_model(teamA, teamB);

    }

	display(MODE_SKILL, "Skill distribution", players);
	display(MODE_WINS, "Wins distribution", players);
	display_paramters();
	printf("Games stopped due to being too long: %ld, draws: %ld\n", vGamesTooLong, vGamesDraw);
	prepare_time(time_buffer,start,0,0);
	printf("Time of running is %s\n",time_buffer);
	if(players != NULL) free(players);

    return 0;
}
