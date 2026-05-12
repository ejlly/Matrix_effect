#include <cstdio>
#include <termios.h>
#include <csignal>
#include <vector>
#include <random>
#include <string_view>
#include <thread>

class Terminal {
public:
    Terminal() {
        tcgetattr(STDIN_FILENO, &old_);
        new_ = old_;
        new_.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_);
        std::printf("\033[?25l\033[?1049h");   //hide cursor, alternate buffer
    }

    ~Terminal() {
		//reset colors, restore original screen buffer, visible cursor
        std::printf("\033[m\033[?1049l\033[?25h");
        tcsetattr(STDIN_FILENO, TCSADRAIN, &old_);
    }

private:
    termios old_, new_;
};

static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

volatile std::sig_atomic_t keepRunning = true;

void signalHandler(int) {
    keepRunning = false;
}

void getTerminalSize(int& rows, int& cols) {
	//save cursor position, set to down right corner, get pos, reset to saved position
    std::printf("\033[s\033[999;999H\033[6n\033[u");
    std::fflush(stdout);
    std::scanf("\033[%d;%dR", &rows, &cols);
}

bool any_key_pressed() {
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	timeval timeout {0, 0};
	if (select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout) > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
		char ch;
		if (read(STDIN_FILENO, &ch, 1) > 0) {
			return true;
		}
	}

	return false;
}

class Word {
	private:
		int m_size = 0;
		int m_position = 0;
		std::vector<int> m_char_index, m_lv; //lv stands for light level

		static constexpr int max_word_length = 30;
		static int s_screen_height;

	public:
		static const std::vector<int>& color_tab() {
			static const std::vector<int> tab = {
				13,	2,	8,
				0,	59,	0,
				0,	143,	17,
				0,	255,	65
			};
			return tab;
		}

		static const std::vector<std::string_view>& alphabet() {
			static const std::vector<std::string_view> alphabet = {
				"\u65e5", "\uff8a", "\uff90", "\uff8b", "\uff70",
				"\uff73", "\uff7c", "\uff85", "\uff93", "\uff86",
				"\uff7b", "\uff9c", "\uff82", "\uff75", "\uff98",
				"\uff71", "\uff8e", "\uff83", "\uff8f", "\uff79",
				"\uff92", "\uff74", "\uff76", "\uff77", "\uff91",
				"\uff95", "\uff97", "\uff7e", "\uff88", "\uff7d",
				"\uff80", "\uff87", "\uff8d", "\u0054", "\u0048",
				"\u0045", "\u004d", "\u0041", "\u0054", "\u0052",
				"\u0049", "\u0058", "\u005a", "\u003a", "\u30fb",
				"\u002e", "\u0022", "\u003d", "\u002a", "\u002b",
				"\u002d", "\u003c", "\u003e", "\u00a6", "\uff5c"
			};
			return alphabet;
		}

		static void set_screen_height(int h) {
			s_screen_height = h;
		}

		Word() {
			static std::binomial_distribution<int> size_dist(max_word_length, 0.5);
			m_size = size_dist(rng()) + 1;
			m_char_index.resize(m_size);
			m_lv.resize(m_size);

			std::uniform_int_distribution<int> char_dist(0, alphabet().size() - 1);
			std::uniform_int_distribution<int> mod7(0, 6);
			std::uniform_int_distribution<int> coin(0, 1);

			for (int i(0); i<m_size; i++) {
				//1/7 chance to be a char that changes each time
				m_char_index[i] = (mod7(rng()) == 0) ? -1 : char_dist(rng());

				if (i <= m_size/4)
					m_lv[i] = 2 + coin(rng());
				else if (i <= 3*m_size/4)
					m_lv[i] = 1 + coin(rng());
				else
					m_lv[i] = coin(rng());
			}
		}

		bool advance(std::string& buffer, int column) {
			//yields true if needs to be suppressed, and false otherwise
			m_position++;

			static std::uniform_int_distribution<int> rand_char(0, alphabet().size() - 1);
			const auto& color_tab = Word::color_tab();

			for (int i(0); i<m_size; i++) {
				if (0 <= m_position - i && s_screen_height > m_position - i) {
					int const char_value = (m_char_index[i] >= 0)?
						m_char_index[i]:
						rand_char(rng());


					buffer += "\033[" + std::to_string(m_position - i) + ";" + std::to_string(column) + "H";
					buffer += "\033[38;2;" + std::to_string(color_tab[3 * m_lv[i]]) + ";" +
						std::to_string(color_tab[3 * m_lv[i] + 1]) + ";" +
						std::to_string(color_tab[3 * m_lv[i] + 2]) + "m";
					buffer += Word::alphabet()[char_value];
				}
				if (0 <= m_position - m_size) {
					buffer += "\033[" + std::to_string(m_position - m_size) + ";" + std::to_string(column) + "H ";
				}
			}
			if (m_position - m_size == s_screen_height) return true;
			return false;
		}

};

int Word::s_screen_height = 0;

int main() {
	Terminal term;
	std::signal(SIGINT, signalHandler);
	
	int nbCol(0), nbLig(0);
	getTerminalSize(nbLig, nbCol);

	Word::set_screen_height(nbLig);

	//fill the screen with black spaces
	printf("\033[m\033[48;2;0;0;0m"); //reset attributes
	for (int i(0); i<nbLig; i++){
		for (int j(0); j<nbCol; j++){
			std::printf(" ");
		}
		std::printf("\n");
	}
		
	std::vector<bool> hasWord(nbCol, false);
	std::vector<Word> words(nbCol);
	std::uniform_int_distribution<int> spawn_dist(0, 999);
    constexpr int kSpawnThreshold = 6;   //0.6% per column per frame

	std::string buffer;
	buffer.reserve(64*1024);

	while (keepRunning) {
		if (any_key_pressed()) {
			raise(SIGINT);
			break;
		}

		buffer.clear();

		for (int col(0); col<nbCol; col++) {
			if (hasWord[col])
				hasWord[col] = !words[col].advance(buffer, col);
			else {
				if (spawn_dist(rng()) < kSpawnThreshold) {
					hasWord[col] = true;
					words[col] = Word();
				}
			}
		}

		std::printf("%s", buffer.c_str());
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
	}

	return 0;
}
