#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <termios.h>
#include <unistd.h>
#include <random>
#include <csignal>

using namespace std;

/*
green colors from darkest to brightest:
R	G	B
13	2	8
0	59	0
0	143	17
0	255	65
*/

int nbCol(0), nbLig(0);

int const ALPHABET_SIZE = 55;
const char *alphabet[ALPHABET_SIZE];
bool keepRunning = true;

void signalHandler(int dummy){
	keepRunning = false;
}

int color_tab[12] = { 13,	2,		8,
					  0,	59,		0,
					  0,	143,	17,
					  0,	255,	65};

int evaluate(int char_index){
	if(char_index >= 0) return char_index;
	return rand()%ALPHABET_SIZE;
}

class Word{
	public:
		Word(){
			m_size = rand()%31;
			m_position = 0;
			m_char_index = new int[m_size];
			m_lv = new int[m_size];

			for(int i(0); i<m_size; i++){
				if(rand()%7==0)
					m_char_index[i] = -1;
				else
					m_char_index[i] = rand()%ALPHABET_SIZE;

				if(i<=m_size/4)
					m_lv[i] = 2+rand()%2;
				else if(i<=3*m_size/4)
					m_lv[i] = 1+rand()%2;
				else
					m_lv[i] = rand()%2;
			}
		}

		bool advance(int column){
			//yields true if needs to be suppressed, and false otherwise
			m_position++;
			for(int i(0); i<m_size; i++){
				if(0 <= m_position - i && nbLig > m_position - i){
					int const char_value = evaluate(m_char_index[i]);
					 
					 printf("\033[%d;%dH\033[48;2;0;0;0m\033[38;2;%d;%d;%dm%s", m_position - i, column, 
															color_tab[3*m_lv[i]], color_tab[3*m_lv[i]+1], color_tab[3*m_lv[i]+2], 
																alphabet[char_value]);
					fflush(stdout);
				}
				if(0 <= m_position - m_size){
					printf("\033[%d;%dH ", m_position - m_size, column);
					fflush(stdout);
				}
			}
			if(m_position - m_size == nbLig) return true;
			return false;
		}

	private:
		int m_size, m_position;
		int *m_char_index, *m_lv; //lv stands for light level
};

int main(){
	
	srand(time(NULL));

	//initialize the alphabet which are unicode characters

	alphabet[0] = "\u65e5";
	alphabet[1] = "\uff8a";
	alphabet[2] = "\uff90";
	alphabet[3] = "\uff8b";
	alphabet[4] = "\uff70";
	alphabet[5] = "\uff73";
	alphabet[6] = "\uff7c";
	alphabet[7] = "\uff85";
	alphabet[8] = "\uff93";
	alphabet[9] = "\uff86";
	alphabet[10] = "\uff7b";
	alphabet[11] = "\uff9c";
	alphabet[12] = "\uff82";
	alphabet[13] = "\uff75";
	alphabet[14] = "\uff98";
	alphabet[15] = "\uff71";
	alphabet[16] = "\uff8e";
	alphabet[17] = "\uff83";
	alphabet[18] = "\uff8f";
	alphabet[19] = "\uff79";
	alphabet[20] = "\uff92";
	alphabet[21] = "\uff74";
	alphabet[22] = "\uff76";
	alphabet[23] = "\uff77";
	alphabet[24] = "\uff91";
	alphabet[25] = "\uff95";
	alphabet[26] = "\uff97";
	alphabet[27] = "\uff7e";
	alphabet[28] = "\uff88";
	alphabet[29] = "\uff7d";
	alphabet[30] = "\uff80";
	alphabet[31] = "\uff87";
	alphabet[32] = "\uff8d";
	alphabet[33] = "\u0054";
	alphabet[34] = "\u0048";
	alphabet[35] = "\u0045";
	alphabet[36] = "\u004d";
	alphabet[37] = "\u0041";
	alphabet[38] = "\u0054";
	alphabet[39] = "\u0052";
	alphabet[40] = "\u0049";
	alphabet[41] = "\u0058";
	alphabet[42] = "\u005a";
	alphabet[43] = "\u003a";
	alphabet[44] = "\u30fb";
	alphabet[45] = "\u002e";
	alphabet[46] = "\u0022";
	alphabet[47] = "\u003d";
	alphabet[48] = "\u002a";
	alphabet[49] = "\u002b";
	alphabet[50] = "\u002d";
	alphabet[51] = "\u003c";
	alphabet[52] = "\u003e";
	alphabet[53] = "\u00a6";
	alphabet[54] = "\uff5c";

	/*
	for(int i(0); i<ALPHABET_SIZE; i++){
		printf("%s\n", alphabet[i]);
	}
	*/


	struct termios old_term, new_term;
	if(tcgetattr(STDIN_FILENO, &old_term) != 0){
		printf("Couldn't get old terminal attributes\n");
		return 1;
	}
	
	new_term = old_term;
	new_term.c_lflag &=~ICANON;
	new_term.c_lflag &=~ECHO;

	if(tcsetattr(STDIN_FILENO, TCSANOW, &new_term) != 0){
		printf("Couldn't modify terminal attributes\n");
		return 1;
	}
	
	printf("\033[?25l"); //invisible cursor
	printf("\033[2J\n"); //clear screen

	printf("\033[s\033[999;999H\033[6n\033[u\n"); 
	//save cursor position, set to down right corner, get pos, reset to saved position

	
	
	scanf("\033[%d;%d R", &nbLig, &nbCol);  

	printf("\033[m");

	for(int i(0); i<nbLig; i++){
		for(int _(0); _<nbCol; _++){
			printf("\033[48;2;0;0;0m ");
		}
		printf("\n");
	}
	//printf("values are : %d %d\n", nbLig, nbCol);

	bool hasWord[nbCol];
	Word words[nbCol];

	for(int i(0); i<nbCol; i++){
		hasWord[i] = false;
	}

	signal(SIGINT, signalHandler);

	while(keepRunning){
		for(int j(0); j<nbCol; j++){
			if(hasWord[j]){
				if(words[j].advance(j)){
					hasWord[j] = false;
				}
			}
			else{
				if(rand()%1000<14){ //probabilité d'apparition d'un mot de 0.014%
					hasWord[j] = true;
					words[j] = Word();
				}
			}
		}
		usleep(90*1000);
	}

	if(tcsetattr(STDIN_FILENO, TCSADRAIN, &old_term) != 0){
		printf("Couldn't revert to old terminal");
		return 1;
	}
	printf("\033[m\033[2J\033[?25h\n"); //visible cursor
	return 0;
}
