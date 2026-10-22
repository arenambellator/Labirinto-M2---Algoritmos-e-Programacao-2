#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <termios.h>
#include <unistd.h>

using namespace std;

constexpr int TAM = 30;
constexpr int TAM_MIN = 3;
constexpr int NUM_OPCOES = 6;
constexpr int TOP_RECORDES = 5;
constexpr int JANELA_LISTA = 10;

enum Opcao { NOVO_JOGO, CONTINUAR, VER_SOLUCAO, RECORDES, SOBRE, FIM };

const char *opcoesMenu[NUM_OPCOES] = {
    "Novo jogo", "Continuar", "Ver solução", "Recordes", "Sobre", "Fim"
};

enum class Tela { Menu, Escolha, Jogo, Recordes, Sobre };

template <typename T>
void Trocar(T &a, T &b) {
    T temp = a;
    a = b;
    b = temp;
}

template <typename T>
void CopiarMatriz(T (&origem)[TAM][TAM], T (&destino)[TAM][TAM]) {
    for (int i = 0; i < TAM; i++)
        for (int j = 0; j < TAM; j++)
            destino[i][j] = origem[i][j];
}

struct Posicao {
    int x = 0;
    int y = 0;
};

struct Jogo {
    int mat[TAM][TAM];
    int n = 0;
    Posicao jogador;
    int rerun1 = 0;
    int jogoAtual = 1;
    int faseCarregada = -1;
    int contador = 0;
    int movimentos = 0;
    bool giro = true;
};

struct Recorde {
    int movimentos = 0;
    int rotacoes = 0;
};

int getch(void) {
    int ch;
    struct termios oldt;
    struct termios newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    cout << flush;
    ch = cin.get();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void SetCursorPos(int XPos, int YPos) {
    cout << "\033[" << YPos + 1 << ";" << XPos + 1 << "H";
}

void LimparTela() {
    cout << "\033[2J\033[H";
}

void LimparFimDeLinha(string &s) {
    while (!s.empty() && s.back() == '\r') s.pop_back();
}

bool ParaInteiro(const string &s, int &valor) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        valor = stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

bool ProximoItem(const string &linha, size_t &pos, string &item) {
    if (pos >= linha.size()) return false;

    size_t fim = linha.find(',', pos);
    if (fim == string::npos) fim = linha.size();

    item = linha.substr(pos, fim - pos);
    pos = fim + 1;
    return true;
}

bool DividirInteiros(const string &linha, vector<int> &valores) {
    valores.clear();
    size_t pos = 0;
    string item;
    while (ProximoItem(linha, pos, item)) {
        int v = 0;
        if (!ParaInteiro(item, v)) return false;
        valores.push_back(v);
    }
    return !valores.empty();
}

int CodigoParaTile(int codigo) {
    switch (codigo) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 9;
    case 3: return 2;
    case 4: return 3;
    case 5: return 5;
    case 6: return 4;
    case 7: return 8;
    }
    return -1;
}

string NomeMapa(int mapa) {
    return "mapa" + to_string(mapa) + ".csv";
}

string NomeSolucao(int mapa) {
    return "mapa" + to_string(mapa) + ".sol";
}

bool CarregarMapa(int mapa, Jogo &j, string &titulo, string &erro) {
    ifstream arq(NomeMapa(mapa));
    if (!arq) {
        erro = "arquivo nao encontrado";
        return false;
    }

    string linha;

    int n = 0;
    if (!getline(arq, linha)) {
        erro = "arquivo vazio";
        return false;
    }
    LimparFimDeLinha(linha);
    if (!ParaInteiro(linha, n) || n < TAM_MIN || n > TAM) {
        erro = "linha 1: a dimensao deve ser um inteiro entre " + to_string(TAM_MIN) +
               " e " + to_string(TAM);
        return false;
    }

    if (!getline(arq, titulo)) {
        erro = "falta o titulo (linha 2)";
        return false;
    }
    LimparFimDeLinha(titulo);
    if (titulo.empty() || titulo.find(',') != string::npos) {
        erro = "linha 2: o titulo nao pode ser vazio nem ter virgulas";
        return false;
    }

    int temp[TAM][TAM] = {};
    int jogadores = 0;
    int saidas = 0;

    for (int i = 0; i < n; i++) {
        if (!getline(arq, linha)) {
            erro = "faltam linhas do cenario (esperadas " + to_string(n) + ")";
            return false;
        }
        LimparFimDeLinha(linha);

        size_t pos = 0;
        string celula;
        for (int c = 0; c < n; c++) {
            if (!ProximoItem(linha, pos, celula)) {
                erro = "linha " + to_string(i + 3) + ": esperados " + to_string(n) + " codigos";
                return false;
            }

            if (celula.size() != 1 || celula[0] < '0' || celula[0] > '7') {
                erro = "linha " + to_string(i + 3) + ": codigo invalido '" + celula +
                       "' (use 0 a 7, separados por virgula, sem espacos)";
                return false;
            }
            int codigo = celula[0] - '0';

            if (codigo == 2) jogadores++;
            if (codigo == 5) saidas++;
            temp[i][c] = CodigoParaTile(codigo);
        }

        if (ProximoItem(linha, pos, celula)) {
            erro = "linha " + to_string(i + 3) + ": ha mais de " + to_string(n) + " codigos";
            return false;
        }
    }

    if (jogadores != 1) {
        erro = "deve haver exatamente um codigo 2 (posicao inicial do jogador)";
        return false;
    }
    if (saidas < 1) {
        erro = "falta a saida (codigo 5)";
        return false;
    }

    j.n = n;
    CopiarMatriz(temp, j.mat);
    return true;
}

bool DescobrirMapas(vector<string> &titulos) {
    titulos.clear();

    for (int m = 1; ; m++) {
        ifstream existe(NomeMapa(m));
        if (!existe) break;
        existe.close();

        Jogo tmp = {};
        string titulo, erro;
        if (!CarregarMapa(m, tmp, titulo, erro)) {
            cout << "Mapa invalido (" << NomeMapa(m) << "): " << erro << endl;
            return false;
        }
        titulos.push_back(titulo);
    }

    if (titulos.empty()) {
        cout << "Nenhum mapa encontrado: coloque mapa1.csv na pasta do jogo." << endl;
        return false;
    }
    return true;
}

void GirarMatrizE(int (&mat)[TAM][TAM], int n) {
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            Trocar(mat[i][j], mat[j][i]);

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n / 2; j++)
            Trocar(mat[i][j], mat[i][n - 1 - j]);
}

void GirarMatrizQ(int (&mat)[TAM][TAM], int n) {
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            Trocar(mat[i][j], mat[j][i]);

    for (int j = 0; j < n; j++)
        for (int i = 0; i < n / 2; i++)
            Trocar(mat[i][j], mat[n - 1 - i][j]);
}

bool ehPassavel(int valor) {
    return valor == 6 || valor == 0 || valor == 3 || valor == 5 || valor == 8;
}

void EncontrarJogador(int (&mat)[TAM][TAM], int n, Posicao &p) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (mat[i][j] == 9) {
                p.x = i;
                p.y = j;
            }
}

bool MoverJogador(int (&mat)[TAM][TAM], int n, Posicao &p, int &rerun1, char direcao) {
    int dx = 0, dy = 0;

    switch (direcao) {
    case 'w': dx = -1; break;
    case 's': dx = 1;  break;
    case 'a': dy = -1; break;
    case 'd': dy = 1;  break;
    default: return false;
    }

    int nx = p.x + dx;
    int ny = p.y + dy;

    if (nx < 0 || nx >= n || ny < 0 || ny >= n) return false;
    if (!ehPassavel(mat[nx][ny])) return false;

    int rerun = mat[nx][ny];
    mat[p.x][p.y] = 0;
    p.x = nx;
    p.y = ny;
    mat[p.x - dx][p.y - dy] = rerun1;
    mat[p.x][p.y] = 9;
    rerun1 = rerun;

    return true;
}

void AlternarTerrenoOculto(int (&mat)[TAM][TAM], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (mat[i][j] == 4) mat[i][j] = 6;
            else if (mat[i][j] == 6) mat[i][j] = 4;

            if (mat[i][j] == 8) mat[i][j] = 7;
            else if (mat[i][j] == 7) mat[i][j] = 8;
        }
    }
}

void AplicarGravidadeBlocos(int (&mat)[TAM][TAM], int n) {
    for (int j = 0; j < n; j++) {
        for (int i = n - 2; i >= 0; i--) {
            if (mat[i][j] == 2) {
                int x = i;
                while (x + 1 < n && ehPassavel(mat[x + 1][j])) {
                    mat[x][j] = mat[x + 1][j];
                    mat[x + 1][j] = 2;
                    x++;
                }
            }
        }
    }
}

void GirarMapa(int (&mat)[TAM][TAM], int n, Posicao &p, bool &giro, char direcao) {
    if (direcao == 'q') {
        GirarMatrizQ(mat, n);
    } else if (direcao == 'e') {
        GirarMatrizE(mat, n);
    } else {
        return;
    }

    EncontrarJogador(mat, n, p);
    AlternarTerrenoOculto(mat, n);
    giro = !giro;
    AplicarGravidadeBlocos(mat, n);
}

bool VerificarVitoria(int rerun1) {
    return rerun1 == 5;
}

bool ExecutarAcao(Jogo &e, char tecla) {
    switch (tecla) {
    case 'w':
    case 's':
    case 'a':
    case 'd':
        if (MoverJogador(e.mat, e.n, e.jogador, e.rerun1, tecla)) {
            e.movimentos++;
            return true;
        }
        return false;

    case 'q':
    case 'e':
        if (e.rerun1 == 3) {
            GirarMapa(e.mat, e.n, e.jogador, e.giro, tecla);
            e.contador++;
            return true;
        }
        return false;
    }
    return false;
}

void CarregarFase(Jogo &e) {
    string titulo, erro;
    CarregarMapa(e.jogoAtual, e, titulo, erro);

    EncontrarJogador(e.mat, e.n, e.jogador);
    e.rerun1 = 0;
    e.contador = 0;
    e.movimentos = 0;
    e.giro = true;
}

string Codificar(const Jogo &e) {
    string s;
    s.reserve(e.n * e.n + 1);
    for (int i = 0; i < e.n; i++)
        for (int j = 0; j < e.n; j++)
            s += (char)e.mat[i][j];
    s += (char)e.rerun1;
    return s;
}

void Decodificar(const string &s, Jogo &e) {
    for (int i = 0; i < e.n; i++)
        for (int j = 0; j < e.n; j++)
            e.mat[i][j] = s[i * e.n + j];
    e.rerun1 = s[e.n * e.n];
    EncontrarJogador(e.mat, e.n, e.jogador);
}

unsigned int Aleatorio() {
    static unsigned int estado = ((unsigned int)getpid() * 2654435761u + 12345u) | 1u;
    estado ^= estado << 13;
    estado ^= estado >> 17;
    estado ^= estado << 5;
    return estado;
}

int TamanhoMinimo(int mapa) {
    const string acoes = "wsadqe";

    Jogo atual = {};
    atual.jogoAtual = mapa;
    CarregarFase(atual);

    unordered_set<string> visitados;
    vector<string> estados;
    vector<int> profundidade;

    estados.push_back(Codificar(atual));
    profundidade.push_back(0);
    visitados.insert(estados[0]);

    for (size_t i = 0; i < estados.size(); i++) {
        Decodificar(estados[i], atual);

        if (VerificarVitoria(atual.rerun1)) return profundidade[i];

        for (char a : acoes) {
            Jogo prox = atual;
            if (!ExecutarAcao(prox, a)) continue;

            string chave = Codificar(prox);
            if (visitados.insert(chave).second) {
                estados.push_back(chave);
                profundidade.push_back(profundidade[i] + 1);
            }
        }
    }
    return -1;
}

string ChaveComProfundidade(const string &estado, int profundidade) {
    string chave = estado;
    chave += (char)(profundidade % 256);
    chave += (char)(profundidade / 256);
    return chave;
}

string BuscarComLimite(int mapa, int limite) {
    const string acoes = "wsadqe";

    Jogo atual = {};
    atual.jogoAtual = mapa;
    CarregarFase(atual);

    unordered_set<string> visitados;
    vector<string> pilhaEstado;
    vector<string> pilhaOrdem;
    vector<int> pilhaProx;
    vector<char> pilhaTecla;

    auto empilhar = [&](const string &estado, char tecla) {
        string ordem = acoes;
        for (int k = (int)ordem.size() - 1; k > 0; k--)
            Trocar(ordem[k], ordem[Aleatorio() % (k + 1)]);
        pilhaEstado.push_back(estado);
        pilhaOrdem.push_back(ordem);
        pilhaProx.push_back(0);
        pilhaTecla.push_back(tecla);
    };

    string inicial = Codificar(atual);
    visitados.insert(ChaveComProfundidade(inicial, 0));
    empilhar(inicial, 0);

    while (!pilhaEstado.empty()) {
        size_t topo = pilhaEstado.size() - 1;
        Decodificar(pilhaEstado[topo], atual);

        if (VerificarVitoria(atual.rerun1)) {
            string sol;
            for (size_t k = 1; k < pilhaTecla.size(); k++)
                sol += pilhaTecla[k];
            return sol;
        }

        if ((int)topo >= limite || pilhaProx[topo] >= (int)acoes.size()) {
            pilhaEstado.pop_back();
            pilhaOrdem.pop_back();
            pilhaProx.pop_back();
            pilhaTecla.pop_back();
            continue;
        }

        char a = pilhaOrdem[topo][pilhaProx[topo]++];
        Jogo prox = atual;
        if (!ExecutarAcao(prox, a)) continue;

        string chave = Codificar(prox);
        if (visitados.insert(ChaveComProfundidade(chave, (int)topo + 1)).second)
            empilhar(chave, a);
    }
    return "";
}

string ResolverAleatorio(int mapa) {
    int minimo = TamanhoMinimo(mapa);
    if (minimo < 0) return "";

    int limite = minimo + minimo / 2 + 2;
    string sol;
    for (int tentativa = 0; tentativa < 20; tentativa++) {
        sol = BuscarComLimite(mapa, limite);
        if ((int)sol.size() > minimo) break;
    }
    return sol;
}

string LerSolucao(int mapa) {
    ifstream arq(NomeSolucao(mapa));
    string sol, linha;
    while (getline(arq, linha)) {
        for (char c : linha) {
            if (c >= 'A' && c <= 'Z') c = c + ('a' - 'A');
            if (c != ' ' && c != '\t' && c != '\r') sol += c;
        }
    }
    return sol;
}

void SalvarSolucao(int mapa, const string &sol) {
    ofstream arq(NomeSolucao(mapa));
    if (!arq) return;
    arq << sol << "\n";
}

string ObterSolucao(int mapa) {
    string sol = LerSolucao(mapa);
    if (!sol.empty()) return sol;

    sol = ResolverAleatorio(mapa);
    if (!sol.empty()) SalvarSolucao(mapa, sol);
    return sol;
}

void SalvarProgresso(const Jogo &e) {
    ofstream arq("jogo_salvo.csv");
    if (!arq) return;

    int giro = 0;
    if (e.giro) giro = 1;

    arq << e.jogoAtual << "," << e.contador << "," << e.movimentos << ","
        << e.rerun1 << "," << giro << "," << e.n << "\n";

    for (int i = 0; i < e.n; i++) {
        for (int j = 0; j < e.n; j++) {
            arq << e.mat[i][j];
            if (j + 1 < e.n) arq << ",";
            else arq << "\n";
        }
    }
}

bool LerProgresso(Jogo &e, int numMapas) {
    ifstream arq("jogo_salvo.csv");
    string linha;
    vector<int> v;

    if (!getline(arq, linha)) return false;
    LimparFimDeLinha(linha);
    if (!DividirInteiros(linha, v) || v.size() != 6) return false;

    Jogo tmp = {};
    tmp.jogoAtual = v[0];
    tmp.contador = v[1];
    tmp.movimentos = v[2];
    tmp.rerun1 = v[3];
    tmp.giro = (v[4] != 0);
    tmp.n = v[5];

    if (tmp.jogoAtual < 1 || tmp.jogoAtual > numMapas) return false;
    if (tmp.contador < 0 || tmp.movimentos < 0) return false;
    if (tmp.rerun1 < 0 || tmp.rerun1 > 9) return false;
    if (tmp.n < TAM_MIN || tmp.n > TAM) return false;

    int jogadores = 0;
    for (int i = 0; i < tmp.n; i++) {
        if (!getline(arq, linha)) return false;
        LimparFimDeLinha(linha);
        if (!DividirInteiros(linha, v) || (int)v.size() != tmp.n) return false;

        for (int j = 0; j < tmp.n; j++) {
            if (v[j] < 0 || v[j] > 9) return false;
            if (v[j] == 9) jogadores++;
            tmp.mat[i][j] = v[j];
        }
    }
    if (jogadores != 1) return false;

    EncontrarJogador(tmp.mat, tmp.n, tmp.jogador);
    tmp.faseCarregada = tmp.jogoAtual;
    e = tmp;
    return true;
}

void ApagarProgresso() {
    ofstream arq("jogo_salvo.csv", ios::trunc);
}

bool ExisteProgresso(int numMapas) {
    Jogo tmp = {};
    return LerProgresso(tmp, numMapas);
}

int AtualizarRecorde(vector<Recorde> &lista, int movimentos, int rotacoes) {
    int total = movimentos + rotacoes;

    size_t pos = 0;
    while (pos < lista.size() && lista[pos].movimentos + lista[pos].rotacoes <= total)
        pos++;

    if (pos >= (size_t)TOP_RECORDES) return -1;

    Recorde novo;
    novo.movimentos = movimentos;
    novo.rotacoes = rotacoes;
    lista.insert(lista.begin() + pos, novo);

    if (lista.size() > (size_t)TOP_RECORDES) lista.pop_back();
    return (int)pos;
}

void CarregarRecordes(vector<vector<Recorde>> &r) {
    ifstream arq("recordes.csv");
    string linha;
    vector<int> v;

    while (getline(arq, linha)) {
        LimparFimDeLinha(linha);
        if (!DividirInteiros(linha, v) || v.size() != 3) continue;

        int mapa = v[0];
        if (mapa < 1 || mapa > (int)r.size() || v[1] < 0 || v[2] < 0) continue;
        AtualizarRecorde(r[mapa - 1], v[1], v[2]);
    }
}

void SalvarRecordes(const vector<vector<Recorde>> &r) {
    ofstream arq("recordes.csv");
    if (!arq) return;

    arq << "mapa,movimentos,rotacoes\n";
    for (size_t m = 0; m < r.size(); m++)
        for (const Recorde &rec : r[m])
            arq << m + 1 << "," << rec.movimentos << "," << rec.rotacoes << "\n";
}

void DesenharCenario(const Jogo &e) {
    cout << "\033[37m";

    for (int i = 0; i < e.n; i++) {
        for (int j = 0; j < e.n; j++) {
            switch (e.mat[i][j]) {
            case 0:  cout << " "; break;
            case 1:  cout << "\033[42m"; cout << " "; cout << "\033[0m"; break;
            case 2:  cout << "\033[41m"; cout << " "; cout << "\033[0m"; break;
            case 3:  cout << "\033[35m"; cout << "/"; break;
            case 4:  cout << "\033[31m"; cout << "="; break;
            case 5:  cout << "\033[33m"; cout << "S"; break;
            case 6:  cout << "\033[37m"; cout << ":"; break;
            case 7:  cout << "\033[31m"; cout << "|"; break;
            case 8:  cout << "\033[37m"; cout << ";"; break;
            case 9:  cout << "\033[36m"; cout << "@"; break;
            default: cout << " ";
            }
        }
        cout << "\033[0m";
        cout << endl;
    }
}

void DesenharMapa(const Jogo &e, const string &titulo) {
    DesenharCenario(e);

    cout << "\033[36m";
    cout << "[r] reiniciar  [m] menu  [Enter] concluir" << endl << endl;
    cout << "\033[37m";

    cout << titulo << "    " << endl;
    cout << "Movimentos: " << e.movimentos << "    " << endl;
    cout << "Rotacoes:   " << e.contador << "    " << endl;
}

void DesenharMenu(int opcao, bool temSalvo) {
    cout << "\033[33m";
    cout << "====================================" << endl
         << "          QUEBRA-CABECA" << endl
         << "====================================" << endl << endl;

    for (int i = 0; i < NUM_OPCOES; i++) {
        if (i == CONTINUAR && !temSalvo) cout << "\033[90m";
        else if (i == opcao) cout << "\033[1;36m";
        else cout << "\033[37m";

        if (i == opcao) cout << "  > ";
        else cout << "    ";
        cout << opcoesMenu[i] << endl;
    }

    cout << "\033[36m";
    cout << endl << "[w/s] mover  [Enter] escolher" << endl;
    cout << "\033[37m";
}

void DesenharEscolha(const vector<string> &titulos, int selecionado, bool solucao) {
    cout << "\033[33m";
    cout << "====================================" << endl;
    if (solucao) cout << "     VER SOLUÇÃO - ESCOLHA O MAPA" << endl;
    else cout << "          ESCOLHA O MAPA" << endl;
    cout << "====================================" << endl << endl;

    int total = (int)titulos.size();
    int inicio = selecionado - JANELA_LISTA / 2;
    if (inicio > total - JANELA_LISTA) inicio = total - JANELA_LISTA;
    if (inicio < 0) inicio = 0;
    int fim = inicio + JANELA_LISTA;
    if (fim > total) fim = total;

    if (inicio > 0) {
        cout << "\033[90m";
        cout << "      ..." << endl;
    }
    for (int i = inicio; i < fim; i++) {
        if (i == selecionado) cout << "\033[1;36m";
        else cout << "\033[37m";
        if (i == selecionado) cout << "  > ";
        else cout << "    ";
        cout << i + 1 << ". " << titulos[i] << endl;
    }
    if (fim < total) {
        cout << "\033[90m";
        cout << "      ..." << endl;
    }

    cout << "\033[36m";
    cout << endl << "[w/s] mover  [Enter] escolher  [m] voltar" << endl;
    cout << "\033[37m";
}

void DesenharRecordes(const vector<vector<Recorde>> &r, const vector<string> &titulos) {
    cout << "\033[33m";
    cout << "====================================" << endl
         << "             RECORDES" << endl
         << "====================================" << endl << endl;
    cout << "\033[37m";

    for (size_t m = 0; m < r.size(); m++) {
        cout << " " << titulos[m] << ":" << endl;
        for (int k = 0; k < TOP_RECORDES; k++) {
            cout << "   " << k + 1 << ". ";
            if (k < (int)r[m].size())
                cout << r[m][k].movimentos << " movimentos, " << r[m][k].rotacoes << " rotacoes" << endl;
            else
                cout << "--" << endl;
        }
        cout << endl;
    }

    cout << "\033[36m";
    cout << "[qualquer tecla] voltar" << endl;
    cout << "\033[37m";
}

void DesenharSobre() {
    cout << "\033[33m";

    cout << "====================================" << endl
         << "               SOBRE" << endl
         << "====================================" << endl
         << " Jogo de quebra-cabeca com rotacao" << endl
         << " de mapa, alavancas e gravidade." << endl << endl
         << " Desenvolvido em C++" << endl << endl
         << " Por Vitor Chilanti" << endl;

    cout << "\033[36m";
    cout << endl << "[qualquer tecla] voltar" << endl;
    cout << "\033[37m";
}

void TelaVitoriaFinal(const Jogo &e, int posicaoRecorde) {
    LimparTela();
    cout << "\033[33m";
    cout << "\n\n       Parabéns! Você ganhou!\n\n";
    cout << "       Movimentos: " << e.movimentos << "   Rotacoes: " << e.contador << endl;
    if (posicaoRecorde >= 0)
        cout << "       Voce entrou no top " << TOP_RECORDES << ": " << posicaoRecorde + 1 << "o lugar!" << endl;
    cout << "\033[36m";
    cout << endl << "       [qualquer tecla] voltar ao menu" << endl;
    cout << "\033[37m";
    getch();
}

void ReproduzirSolucao(int mapa, const string &titulo) {
    LimparTela();

    string sol = ObterSolucao(mapa);
    if (sol.empty()) {
        cout << "\033[31mNão foi possível obter uma solução para este mapa.\n"
             << "\033[36m\n[qualquer tecla] voltar\n\033[37m" << flush;
        getch();
        return;
    }

    string texto = "\033[33m" + titulo + "\033[37m\n\n";
    for (size_t i = 0; i < sol.size(); i += 30)
        texto += " " + sol.substr(i, 30) + "\n";
    texto += "\033[36m\n[qualquer tecla] voltar\n\033[37m";

    cout << texto << flush;
    getch();
}

int main() {
    vector<string> titulos;
    if (!DescobrirMapas(titulos)) return 1;
    const int numMapas = (int)titulos.size();

    Jogo jogo = {};
    vector<vector<Recorde>> recordes(numMapas);
    CarregarRecordes(recordes);

    Tela tela = Tela::Menu;
    int opcao = NOVO_JOGO;
    int selecionado = 0;
    bool escolhendoSolucao = false;
    bool temSalvo = ExisteProgresso(numMapas);
    bool jogando = true;

    cout << "\033[?25l";
    LimparTela();

    while (jogando) {
        if (tela != Tela::Jogo) LimparTela();

        switch (tela) {
        case Tela::Menu:
            DesenharMenu(opcao, temSalvo);
            break;
        case Tela::Escolha:
            DesenharEscolha(titulos, selecionado, escolhendoSolucao);
            break;
        case Tela::Jogo:
            if (jogo.jogoAtual != jogo.faseCarregada) {
                CarregarFase(jogo);
                jogo.faseCarregada = jogo.jogoAtual;
                LimparTela();
            }
            DesenharMapa(jogo, titulos[jogo.jogoAtual - 1]);
            break;
        case Tela::Recordes:
            DesenharRecordes(recordes, titulos);
            break;
        case Tela::Sobre:
            DesenharSobre();
            break;
        }

        char tecla = getch();

        switch (tela) {
        case Tela::Menu:
            if (tecla == 'w') {
                opcao = (opcao + NUM_OPCOES - 1) % NUM_OPCOES;
            } else if (tecla == 's') {
                opcao = (opcao + 1) % NUM_OPCOES;
            } else if (tecla == '\n') {
                switch (opcao) {
                case NOVO_JOGO:
                    escolhendoSolucao = false;
                    selecionado = 0;
                    tela = Tela::Escolha;
                    break;
                case CONTINUAR:
                    if (LerProgresso(jogo, numMapas)) {
                        tela = Tela::Jogo;
                        LimparTela();
                    } else {
                        temSalvo = false;
                    }
                    break;
                case VER_SOLUCAO:
                    escolhendoSolucao = true;
                    selecionado = 0;
                    tela = Tela::Escolha;
                    break;
                case RECORDES: tela = Tela::Recordes; break;
                case SOBRE:    tela = Tela::Sobre;    break;
                case FIM:      jogando = false;       break;
                }
            }
            break;

        case Tela::Escolha:
            if (tecla == 'w') {
                selecionado = (selecionado + numMapas - 1) % numMapas;
            } else if (tecla == 's') {
                selecionado = (selecionado + 1) % numMapas;
            } else if (tecla == 'm') {
                tela = Tela::Menu;
            } else if (tecla == '\n') {
                if (escolhendoSolucao) {
                    ReproduzirSolucao(selecionado + 1, titulos[selecionado]);
                } else {
                    jogo = Jogo();
                    jogo.jogoAtual = selecionado + 1;
                    ApagarProgresso();
                    temSalvo = false;
                    tela = Tela::Jogo;
                }
            }
            break;

        case Tela::Jogo:
            if (tecla == '\n') {
                if (VerificarVitoria(jogo.rerun1)) {
                    int posicao = AtualizarRecorde(recordes[jogo.jogoAtual - 1],
                                                   jogo.movimentos, jogo.contador);
                    if (posicao >= 0) SalvarRecordes(recordes);

                    if (jogo.jogoAtual < numMapas) {
                        jogo.jogoAtual++;
                    } else {
                        TelaVitoriaFinal(jogo, posicao);
                        ApagarProgresso();
                        temSalvo = false;
                        tela = Tela::Menu;
                    }
                }
            } else if (tecla == 'r') {
                jogo.faseCarregada = -1;
            } else if (tecla == 'm') {
                SalvarProgresso(jogo);
                temSalvo = true;
                tela = Tela::Menu;
            } else {
                ExecutarAcao(jogo, tecla);
            }
            break;

        case Tela::Recordes:
        case Tela::Sobre:
            tela = Tela::Menu;
            break;
        }

        if (tela == Tela::Jogo) SetCursorPos(0, 0);
    }

    LimparTela();
    cout << "\033[?25h\033[0m";
    return 0;
}
