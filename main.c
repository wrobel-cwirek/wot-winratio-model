#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define GAME_TYPE long
// orig: 30000 test: 300 200000
#define PLAYERS 200000
// orig: 20000000 test: 2000 1000000000
#define GAMES 20000000
// orig: 100000 test: 100
#define DISPLAY_PROGRESS_NONE 0
#define DISPLAY_PROGRESS_TIME 2
#define DISPLAY_PROGRESS_INTERVALS 1
#define DISPLAY_PROGRESS 100000
#define DISPLAY_PROGRESS_EVERY 10
#define TEAM_SIZE 15

// constants for W model
#define TANK_POWER_FACTOR 10
#define SKILL_ADJUSTMENT 20

// constants for cellural W model
#define TANK_BONUS 10
#define RANDOM_BONUS 10
#define PLAYER_RESIGN_CHANCE 1000
#define MAX_GAME_LENGTH 100
#define SAME_SKILL_DOUBLE_KILL 0

#define MODE_WINS 1
#define MODE_SKILL 2

#define LOG 1

typedef struct {
    int id;
    int skill;
    int tank;
    GAME_TYPE wins;
    GAME_TYPE looses;
} Player;

int pTeamSize = TEAM_SIZE;
int pPlayers = PLAYERS;
GAME_TYPE pGames = GAMES;
int pTankBonus = TANK_BONUS;
int pRandomBonus = RANDOM_BONUS;
int pSameSkillDoubleKill = SAME_SKILL_DOUBLE_KILL;
int pProgressMode = DISPLAY_PROGRESS_NONE;
int pProgressValue = 0;
int pDisplayExcel = 0;
GAME_TYPE vGamesTooLong = 0;
GAME_TYPE vGamesDraw = 0;
unsigned long long vGamesIterations = 0;

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
    for (i = 0; i < pTeamSize; ++i) {
        if (what == where[i]) return 1;
        if (where[i] == NULL) return 0;
    }
    return 0;
}

int prepare_team(Player* team[], Player* other_team[], Player* all_players){
	int skill=0, fill = 0;
	Player* p = NULL;

	memset(team, 0, sizeof(team));
	
	while (fill < pTeamSize) {
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
            for (j = 0; j < pTeamSize; ++j) {
                ++(teamA[j]->wins);
                ++(teamB[j]->looses);
            }
        } else if (skillA < skillB) {
            for (j = 0; j < pTeamSize; ++j) {
                ++(teamB[j]->wins);
                ++(teamA[j]->looses);
            }
        } else {
            for (j = 0; j < pTeamSize; ++j) {
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
	for(i = 0; i < pTeamSize; ++i) {
		
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
	for(i = 0; i < pTeamSize; ++i) {
		tanks[i] = rand() % 3;
	}
}

void switchDead(Player* team[], int* tanks, int dead, int alive) {
	Player *p = team[alive];
	team[alive] = team[dead];
	team[dead] = p;
	
	int t = tanks[alive];
	tanks[alive] = tanks[dead];
	tanks[dead] = t;
}

// -1 - teamA won
// 1 - teamB won
// 0 - noone won
int calculate_by_wriothesley_cellural_model(Player* teamA[], Player* teamB[]){
	int out = 0, i, tank;

	int countA = pTeamSize;
	int countB = pTeamSize;

	int* tanksA = NULL; // tanks can be paper = 0, scisors = 1, rock = 2
	int* tanksB = NULL;
	
	tanksA = (int*)malloc(pTeamSize * sizeof(int));
	if(tanksA == NULL) {
		printf("calculate_by_wriothesley_cellural_model - tanksA - memory allocation failed\n");
		exit(1);
	}
	tanksB = (int*)malloc(pTeamSize * sizeof(int));
	if(tanksB == NULL) {
		printf("calculate_by_wriothesley_cellural_model - tanksB - memory allocation failed\n");
		exit(1);
	}

	// random tanks
	init_tanks(tanksA);
	init_tanks(tanksB);

	int count = 0;

	
  while(1 == 1) {
	++count;
	if(countA <= 0 || countB  <= 0) break;
	if(count > MAX_GAME_LENGTH) {
		++vGamesTooLong;
		break;
	}
	for(i = 0; i < pTeamSize; ++i) {

		if(countA <=0 || countB <= 0) break;

		int indexA = find_next_alive_new(countA, i);
		int indexB = find_next_alive_new(countB, i);

		if(indexA < 0 || indexB < 0) break;
		
		++vGamesIterations;

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
			switchDead(teamB, tanksB, indexB, countB);
		} else if(skillB > skillA){
			//statusA[indexA] = 1;
			--countA;
			switchDead(teamA, tanksA, indexA, countA);
		} else {
			if(pSameSkillDoubleKill != 0) {
			--countB;
			switchDead(teamB, tanksB, indexB, countB);	
			--countA;
			switchDead(teamA, tanksA, indexA, countA);			}
		}

	}
  }
	
	if(tanksA != NULL) free(tanksA);
	if(tanksB != NULL) free(tanksB);

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

void display_for_excel(Player players[]){
	GAME_TYPE win_ratio[101] = {0};
	GAME_TYPE i;

	int count = 0;


   	 for (i = 0; i < pPlayers; ++i) {
        	if ((players[i].wins + players[i].looses) > 0) {
            		++(win_ratio[(int) (100 * players[i].wins / (players[i].wins + players[i].looses))]);
			++count;
		}
    	}

	printf("EXCEL;TeamSize;Players;Games;TankBonus;RandomBonus;SameSkillDoubleKill\nEXCEL;%d;%d;%ld;%d;%d;%d\nEXCEL;WIN_RATIO;PLAYERS\n",pTeamSize, pPlayers ,pGames ,pTankBonus, pRandomBonus,pSameSkillDoubleKill);
	for (i = 0; i < 101; ++i) {
        	printf("EXCEL;%d;%d;\n", i, win_ratio[i]);
    	}
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

void display_paramters(char* title){
	if(title != NULL) {
		printf("%s\n",title);
	} 
	printf("TeamSize=%d - Players=%d - Games=%ld - TankBonus=%d - RandomBonus=%d - SameSkillDoubleKill=%d - ProgressMode=%d - ProgressValue=%ld - DisplayExcel=%d\n",pTeamSize, pPlayers ,pGames ,pTankBonus, pRandomBonus,pSameSkillDoubleKill,pProgressMode,pProgressValue,pDisplayExcel);
}

int get_index_of_param(char* p, int argc, const char* argv[], int show_err){
	int i = 0;
	int paramIndex = -10;
	int tmp = 0;
	for(;i < argc; ++i) {
		tmp = strcmp(argv[i], p);
		if(tmp == 0) {
			if(paramIndex != -10){
				if(show_err != 0) printf("Parameter %s provided more then one time. Ignoring!\n", p);
				return -10;
			}
			paramIndex = i;
		}
	}
	return paramIndex;
}

const char* get_value_of_param(char* p, int argc, const char* argv[]){
	const char* out;
	int paramIndex = get_index_of_param(p, argc, argv, 1) + 1;

	if(paramIndex >= 0 && paramIndex < argc){
		out = argv[paramIndex];
		if(out[0] == '-') {
			//printf("Parameter %s provided without value or negative value provided. Ignoring!\n", p);
			return NULL;
		} 
		return out;
	} else {
		//printf("Parameter %s provided without value. Ignoring!\n", p);
		
	}

	return NULL;
}

int get_param_as_int(char* p, int argc, const char* argv[]) {
	int out = -1;
	const char* value = get_value_of_param(p, argc, argv);
	if(value != NULL) {
		out = atoi(value);
	}
	return out;
}

long get_param_as_long(char* p, int argc, const char* argv[]) {
	long out = -1;
	const char* value = get_value_of_param(p, argc, argv);
	if(value != NULL) {
		out = atol(value);
	}
	return out;
}
#define HELP1 "\nWelcome to: \n\n" \
	"     ******************************************\n" \
	"     * \"World of Tanks\" WIN_RATIO simulator *\n" \
	"     ******************************************\n" \
       "\nYou started the simulation without parameters or requested for this help.\n\nParameters options:\n-t  - teamsize (int > 0)\n" \
	"-p  - number of players (int >= team_size * 2)\n-g  - number of games (long > 0)\n-tb - tank bonus (int >= 0)\n-rb - random bonus (int >= 0)\n" \
	"-dk - both tanks dead if same skill (0 for false, 1 for true)\n-pm - mode of displaying progress (0 - none, 1 - intervals, 2 - time)\n-pv - value for progress (int > 0, loops for -pm 1, seconds for -pm 2)\n" \
	"-e  - display data easy to import to Excel (line with prefix EXCEL)\n -h  - display this help and not run simulation\nExample: ./a.out -t 15 -p 300 -g 100 -tb 20 -rb 10 -dk 0 -pm 1 -pv 100 -e\n\n"

void load_parameters_new(int argc, const char* argv[]){

	int show_help = get_index_of_param("-h", argc, argv, 1);
	if(show_help >= 0 || argc <= 1) {
		printf(HELP1);
	}
	int tmp_int;
	long tmp_long;

	display_paramters("Default paramters:");

	tmp_int = get_param_as_int("-t", argc, argv);
	if(tmp_int > 0){
		pTeamSize = tmp_int;
	} else if(get_index_of_param("-t", argc, argv, 0) >= 0){
		printf("Parameter -t incorrect, int > 0 expected. Ignoring!\n");
	}

	tmp_int = get_param_as_int("-p", argc, argv);
	if(tmp_int >= (pTeamSize * 2)){
		pPlayers = tmp_int;
	} else if(get_index_of_param("-p", argc, argv, 0) >= 0){
		printf("Parameter -p incorrect, int >= team_size * 2 expected. Ignoring!\n");
	}
	if(pPlayers < (pTeamSize * 2)){
		printf("Adjusting number of players from %d to be team_size * 2 (team_size provided as %d)\n",pPlayers,pTeamSize);
		pPlayers = pTeamSize * 2;
	}

	tmp_long = get_param_as_long("-g", argc, argv);
	if(tmp_long > 0){
		pGames = tmp_long;
	} else if(get_index_of_param("-g", argc, argv, 0) >= 0){
		printf("Parameter -g incorrect, long > 0 expected. Ignoring!\n");
	}

	tmp_int = get_param_as_int("-tb", argc, argv);
	if(tmp_int >= 0){
		pTankBonus = tmp_int;
	} else if(get_index_of_param("-tb", argc, argv, 0) >= 0){
		printf("Parameter -tb incorrect, int >= 0 expected. Ignoring!\n");
	}

	tmp_int = get_param_as_int("-rb", argc, argv);
	if(tmp_int >= 0){
		pRandomBonus = tmp_int;
	} else if(get_index_of_param("-rb", argc, argv, 0) >= 0){
		printf("Parameter -rb incorrect, int >= 0 expected. Ignoring!\n");
	}

	tmp_int = get_param_as_int("-dk", argc, argv);
	if(tmp_int == 0 || tmp_int == 1){
		pSameSkillDoubleKill = tmp_int;
	} else if(get_index_of_param("-dk", argc, argv, 0) >= 0){
		printf("Parameter -dk incorrect, 0 or 1 expected. Ignoring!\n");
	}

	tmp_int = get_param_as_int("-pm", argc, argv);
	if(tmp_int == 0 || tmp_int == 1 || tmp_int == 2){
		pProgressMode = tmp_int;
	} else if(get_index_of_param("-pm", argc, argv, 0) >= 0){
		printf("Parameter -pm incorrect, 0 or 1 or 2 expected. Ignoring!\n");
	}

	if(pProgressMode == DISPLAY_PROGRESS_INTERVALS) pProgressValue = DISPLAY_PROGRESS;
	else if(pProgressMode == DISPLAY_PROGRESS_TIME) pProgressValue = DISPLAY_PROGRESS_EVERY;
	tmp_int = get_param_as_int("-pv", argc, argv);
	if(tmp_int > 0){
		pProgressValue = tmp_int;
	} else if(get_index_of_param("-pv", argc, argv, 0) >= 0){
		printf("Parameter -pv incorrect, int > 0 expected. Ignoring!\n");
	}

	int show_excel = get_index_of_param("-e", argc, argv, 1);
	if(show_excel >= 0) {
		pDisplayExcel = 1;
	}

	display_paramters("Used paramters:");
	printf("\n");
	if(show_help >= 0 && argc > 1) {
		printf("Option -h used only to display paramters and help. Exiting.\n");
		exit(0);
	}


}

int main(int argc, const char* argv[]) {

	load_parameters_new(argc, argv);

	Player* players = NULL;
	players = (Player*) malloc(pPlayers * sizeof(Player));
	if(players == NULL) {
		printf("main - players - memory allocation failed\n");
		exit(1);
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

    Player** teamA = NULL;
    Player** teamB = NULL;

    teamA = (Player**) malloc(pTeamSize * sizeof(Player*));
    if(teamA == NULL) {
	printf("main - teamA - memory allocation failed\n");
	exit(1);
    }
    teamB = (Player**) malloc(pTeamSize * sizeof(Player*));
    if(teamB == NULL) {
	printf("main - teamB - memory allocation failed\n");
	exit(1);
    }


    int skillA = 0, skillB = 0;

	long last_time = time(NULL);
	long current_time = time(NULL);
	
	printf("SIMULATION START!\n\n");
    for (i = 0; i < pGames; ++i) {
	if(pProgressMode == DISPLAY_PROGRESS_INTERVALS) {
		if((i % pProgressValue) == 0) {
			prepare_time(time_buffer,start,i,pGames);
			printf("Progress... done %d of %d - %lf%% - time of running %s\n",i,pGames,(double)i/(double)pGames*100,time_buffer);
		}
	}
	if(pProgressMode == DISPLAY_PROGRESS_TIME) {
		if((current_time - last_time) >= pProgressValue) {
			prepare_time(time_buffer,start,i,pGames);
			printf("Progress... done %d of %d - %lf%% - time of running %s\n",i,pGames,(double)i/(double)pGames*100,time_buffer);
			last_time = time(NULL);
		}
		current_time = time(NULL);
	}
	skillA = prepare_team(teamA, NULL, players);
	skillB = prepare_team(teamB, teamA, players);

	 //simulate_by_jakub(teamA, skillA, teamB, skillB);
      	//simulate_by_wriothesley(teamA, teamB);
	simulate_by_wriothesley_cellural_model(teamA, teamB);

    }
	printf("\nSIMULATION END!\n\n");
	display(MODE_SKILL, "Skill distribution", players);
	display(MODE_WINS, "Wins distribution", players);
	display_paramters(NULL);
	printf("Games stopped due to being too long: %ld (%.2lf%%), draws: %ld (%.2lf%%), game iterations: %llu (%llu per game)\n", vGamesTooLong,vGamesTooLong/(double)pGames * 100, vGamesDraw, vGamesDraw/(double)pGames * 100, vGamesIterations, vGamesIterations/pGames);
	prepare_time(time_buffer,start,0,0);
	printf("Time of running is %s\n",time_buffer);
	if(pDisplayExcel) {
		display_for_excel(players);
	}
	if(players != NULL) free(players);
	if(teamA != NULL) free(teamA);
	if(teamB != NULL) free(teamB);

    return 0;
}

// some tests

int main2(int argc, const char* argv[]){
	display_paramters("Before");
	load_parameters_new(argc, argv);
	display_paramters("After");
}

int main1() {
	srand(time(NULL));
	long max = 0, min = 2147450833,i = 0;
	for(; i < pPlayers; ++i) {
		long r = lrand();
		if(max < r) max = r;
		if(min > r) min = r;
	}
	printf("Max %ld, min %d\n", max, min);
}


int main0() {
	Player* tmp = NULL;
	tmp = (Player*) malloc(pPlayers * sizeof(Player));
	memset(tmp,0, pPlayers * sizeof(Player));
	Player x = tmp[0];
	free(tmp);
	return 0;
}
