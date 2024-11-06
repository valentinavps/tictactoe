#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <ctime>

class TicTacToe {
public:
    TicTacToe() : board(3, std::vector<char>(3, ' ')), jogador_atual('X'), jogo_terminado(false), vencedor(' ') {}

    void exibir_tabuleiro() const {
        std::cout << "Current Board:\n";
        for (const auto& row : board) {
            for (const auto& cell : row) {
                std::cout << (cell == ' ' ? '.' : cell) << " ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }

    bool fazer_jogada(char jogador, int linha, int coluna) {
        std::lock_guard<std::mutex> lock(board_mutex);

        if (linha < 0 || linha >= 3 || coluna < 0 || coluna >= 3 || board[linha][coluna] != ' ') {
            std::cout << "Invalid move by player " << jogador << " at (" << linha << ", " << coluna << ")\n";
            return false;
        }

        board[linha][coluna] = jogador;
        exibir_tabuleiro();

        if (checar_vitoria(jogador)) {
            vencedor = jogador;
            jogo_terminado = true;
            turn_cv.notify_all();
            return true;
        }

        if (checar_empate()) {
            vencedor = 'D';
            jogo_terminado = true;
            turn_cv.notify_all();
            return true;
        }

        alternar_jogador();
        return false;
    }

    bool is_game_over() const {
        std::lock_guard<std::mutex> lock(board_mutex);
        return jogo_terminado;
    }

    char get_winner() const {
        return vencedor;
    }

    char get_jogador_atual() const { return jogador_atual; }

    void esperar_turno(char jogador) {
        std::unique_lock<std::mutex> lock(turn_mutex);
        turn_cv.wait(lock, [&] { return jogador_atual == jogador || jogo_terminado; });
    }

private:
    std::vector<std::vector<char>> board;
    std::mutex board_mutex;
    std::mutex turn_mutex;
    std::condition_variable turn_cv;
    char jogador_atual;
    bool jogo_terminado;
    char vencedor;

    bool checar_vitoria(char jogador) const {
        for (int i = 0; i < 3; ++i) {
            if ((board[i][0] == jogador && board[i][1] == jogador && board[i][2] == jogador) ||
                (board[0][i] == jogador && board[1][i] == jogador && board[2][i] == jogador)) {
                return true;
            }
        }
        return (board[0][0] == jogador && board[1][1] == jogador && board[2][2] == jogador) ||
               (board[0][2] == jogador && board[1][1] == jogador && board[2][0] == jogador);
    }

    bool checar_empate() const {
        for (const auto& row : board) {
            for (const auto& cell : row) {
                if (cell == ' ') {
                    return false;
                }
            }
        }
        return true;
    }

    void alternar_jogador() {
        jogador_atual = (jogador_atual == 'X') ? 'O' : 'X';
        turn_cv.notify_all();
    }
};

class Player {
public:
    Player(TicTacToe& jogo, char simbolo, const std::string& estrategia)
        : jogo(jogo), simbolo(simbolo), estrategia(estrategia) {}

    void play() {
        if (estrategia == "sequencial") {
            play_sequential();
        } else if (estrategia == "aleatorio") {
            play_random();
        }
    }

private:
    TicTacToe& jogo;
    char simbolo;
    std::string estrategia;

    void play_sequential() {
        for (int i = 0; i < 3 && !jogo.is_game_over(); ++i) {
            for (int j = 0; j < 3 && !jogo.is_game_over(); ++j) {
                jogo.esperar_turno(simbolo);
                if (jogo.is_game_over()) return;

                std::cout << "Player " << simbolo << " attempting move at " << i << ", " << j << std::endl;
                jogo.fazer_jogada(simbolo, i, j);
            }
        }
    }

    void play_random() {
        std::srand(std::time(nullptr));
        while (!jogo.is_game_over()) {
            jogo.esperar_turno(simbolo);
            if (jogo.is_game_over()) return;

            int linha = std::rand() % 3;
            int coluna = std::rand() % 3;

            std::cout << "Player " << simbolo << " attempting random move at " << linha << ", " << coluna << std::endl;
            jogo.fazer_jogada(simbolo, linha, coluna);
        }
    }
};

int main() {
    TicTacToe jogo;
    Player playerX(jogo, 'X', "sequencial");
    Player playerO(jogo, 'O', "aleatorio");

    std::thread t1(&Player::play, &playerX);
    std::thread t2(&Player::play, &playerO);

    t1.join();
    t2.join();

    if (jogo.get_winner() == 'D') {
        std::cout << "Game ended in a draw.\n";
    } else {
        std::cout << "Player " << jogo.get_winner() << " wins!\n";
    }

    return 0;
}
