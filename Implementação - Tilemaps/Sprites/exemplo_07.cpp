#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include "TileMap.h"
#include "DiamondView.h"
#include "ltMath.h"
#include <fstream>

using namespace std;

int g_gl_width = 1024;
int g_gl_height = 768;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
float tw, th, tw2, th2;
int tileSetCols = 9, tileSetRows = 9;
float tileW, tileW2;
float tileH, tileH2;
int cx = -1, cy = -1;
bool gameFinished = false;

TilemapView* tview = new DiamondView();
TileMap* tmap = NULL;

GLFWwindow* g_window = NULL;

bool keyWPressed = false;
bool keyAPressed = false;
bool keySPressed = false;
bool keyDPressed = false;

// Matriz para armazenar as regras de movimento
vector<vector<int>> movementRules;

// Variável para rastrear a pontuação do jogador
int score = 0;

// Variável para rastrear o tempo de início do jogo
double startTime;

// Variável para rastrear se o tile foi selecionado
bool tileSelected = false;

TileMap* readMap(char* filename) {
    ifstream arq(filename);
    int w, h;
    arq >> w >> h;
    TileMap* tmap = new TileMap(w, h, 0);
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            int tid;
            arq >> tid;
            // cout << tid << " ";
            tmap->setTile(c, h - r - 1, tid);
        }
        // cout << endl;
    }
    arq.close();
    return tmap;
}

void readMovementRules(const char* filename) {
    ifstream arq(filename);
    if (!arq.is_open()) {
        cerr << "Failed to open " << filename << endl;
        return;
    }

    int w, h;
    arq >> w >> h;
    movementRules.resize(h, vector<int>(w, 0));

    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            int rule;
            arq >> rule;
            movementRules[r][c] = rule;
        }
    }
    arq.close();
}

int loadTexture(unsigned int& texture, char* filename)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    GLfloat max_aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

    int width, height, nrChannels;

    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data)
    {
        if (nrChannels == 4)
        {
            // cout << "Alpha channel" << endl;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            // cout << "Without Alpha channel" << endl;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return 1; // Sucesso
    }
    else
    {
        // std::cout << "Failed to load texture" << std::endl;
        stbi_image_free(data);
        return 0; // Falha
    }
}

void SRD2SRU(double& mx, double& my, float& x, float& y) {
    x = xi + (mx / g_gl_width) * w;
    y = yi + ((g_gl_height - my) / g_gl_height) * h;
}

void mouse(double& mx, double& my) {
    float y = 0;
    float x = 0;
    SRD2SRU(mx, my, x, y);

    int c, r;
    tview->computeMouseMap(c, r, tw, th, x, y);
    c += (tmap->getWidth() - 1) / 2;
    r += (tmap->getHeight() - 1) / 2;

    float x0, y0;
    tview->computeDrawPosition(c, r, tw, th, x0, y0);
    x0 += xi;

    float point[] = { x, y };

    // Verifica se o ponto está dentro do losango
    if (!tview->pointInDiamond(x, y, x0, y0, tw, th)) {
        // Se o ponto não está no losango, ajusta o tile usando tileWalking
        bool left = x < (x0 + tw / 2.0f);
        if (left) {
            tview->computeTileWalking(c, r, DIRECTION_WEST);
        }
        else {
            tview->computeTileWalking(c, r, DIRECTION_EAST);
        }
    }

    if ((c < 0) || (c >= tmap->getWidth()) || (r < 0) || (r >= tmap->getHeight())) {
        //cout << "wrong click position: " << c << ", " << r << endl;
        return;
    }

    // Verificar se a regra de movimento é do tipo 0, se for, impedir a seleção
    if (movementRules[r][c] == 0) {
        //cout << "Tile com regra de movimento 0 não pode ser selecionado: " << c << ", " << r << endl;
        return;
    }

    // cout << "SELECIONADO c=" << c << "," << r << endl;
    cx = c; cy = r;
    tileSelected = true; // Tile foi selecionado
}



// Função para lidar com as teclas pressionadas
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            keyWPressed = true;
            break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            keyAPressed = true;
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            keySPressed = true;
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            keyDPressed = true;
            break;
        default:
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            keyWPressed = false;
            break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            keyAPressed = false;
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            keySPressed = false;
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            keyDPressed = false;
            break;
        default:
            break;
        }
    }
}

bool allCollectiblesCollected() {
    for (int r = 0; r < movementRules.size(); r++) {
        for (int c = 0; c < movementRules[r].size(); c++) {
            if (movementRules[r][c] == 2) {
                return false;
            }
        }
    }
    return true;
}

void showFinalScore(int score, double elapsedTime) {
    cout << "Jogo Finalizado!" << endl;
    cout << "Pontuacao Final: " << score << endl;
    cout << "Tempo Total: " << elapsedTime << " segundos" << endl;
    gameFinished = true;
}

int main()
{
    restart_gl_log();
    start_gl();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // cout << "Tentando criar tmap" << endl;
    tmap = readMap("terrain1.tmap");
    tw = w / (float)tmap->getWidth();
    th = tw / 2.0f;
    tw2 = th;
    th2 = th / 2.0f;
    tileW = 1.0f / (float)tileSetCols;
    tileW2 = tileW / 2.0f;
    tileH = 1.0f / (float)tileSetRows;
    tileH2 = tileH / 2.0f;

    /* cout << "tw=" << tw << " th=" << th << " tw2=" << tw2 << " th2=" << th2
        << " tileW=" << tileW << " tileH=" << tileH
        << " tileW2=" << tileW2 << " tileH2=" << tileH2
        << endl; */

    GLuint tid;
    loadTexture(tid, "terrain.png");

    tmap->setTid(tid);
    // cout << "Tmap inicializado" << endl;

    // Carregar as regras de movimento
    readMovementRules("rules.txt");

    // Verifique se as dimensões de movementRules são consistentes com as dimensões do mapa
    if (movementRules.size() != tmap->getHeight() || (movementRules.size() > 0 && movementRules[0].size() != tmap->getWidth())) {
        cerr << "Erro: As dimensões de movementRules não são consistentes com as dimensões do mapa." << endl;
        return 1;
    }

    // Inicializar o tempo de início do jogo
    startTime = glfwGetTime();


    float vertices[] = {
        // positions   // texture coords
        xi    , yi + th2, 0.0f, tileH2,
        xi + tw2, yi    , tileW2, 0.0f,
        xi + tw , yi + th2, tileW, tileH2,
        xi + tw2, yi + th , tileW2, tileH,
    };
    unsigned int indices[] = {
        0, 1, 3,
        3, 1, 2
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    char vertex_shader[1024 * 256];
    char fragment_shader[1024 * 256];
    parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
    parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* p = (const GLchar*)vertex_shader;
    glShaderSource(vs, 1, &p, NULL);
    glCompileShader(vs);

    int params = -1;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params)
    {
        fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
        print_shader_info_log(vs);
        return 1;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    p = (const GLchar*)fragment_shader;
    glShaderSource(fs, 1, &p, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
    if (GL_TRUE != params)
    {
        fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
        print_shader_info_log(fs);
        return 1;
    }

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
    if (GL_TRUE != params)
    {
        fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
            shader_programme);
        return false;
    }

    float previous = glfwGetTime();

    for (int r = 0; r < tmap->getHeight(); r++) {
        for (int c = 0; c < tmap->getWidth(); c++) {
            unsigned char t_id = tmap->getTile(c, r);
            // cout << ((int)t_id) << " ";
        }
        // cout << endl;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetKeyCallback(g_window, key_callback);

    while (!glfwWindowShouldClose(g_window)) {
        _update_fps_counter(g_window);
        double current_seconds = glfwGetTime();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, g_gl_width, g_gl_height);

        glUseProgram(shader_programme);

        glBindVertexArray(VAO);
        float x, y;
        for (int r = 0; r < tmap->getHeight(); r++) {
            for (int c = 0; c < tmap->getWidth(); c++) {
                int t_id = (int)tmap->getTile(c, r);
                int u = t_id % tileSetCols;
                int v = t_id / tileSetCols;

                tview->computeDrawPosition(c, r, tw, th, x, y);

                glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW);
                glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH);
                glUniform1f(glGetUniformLocation(shader_programme, "tx"), x);
                glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + 1.0);
                glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), tmap->getZ());

                if (r >= 0 && r < movementRules.size() && c >= 0 && c < movementRules[r].size()) {
                    if (movementRules[r][c] == 2) {
                        glUniform3f(glGetUniformLocation(shader_programme, "color"), 0.0f, 1.0f, 0.0f); // Verde para itens coletáveis
                    }
                    else if (movementRules[r][c] == 3) {
                        glUniform3f(glGetUniformLocation(shader_programme, "color"), 1.0f, 0.0f, 0.0f); // Vermelho para tiles de perda de ponto
                    }
                    else {
                        glUniform3f(glGetUniformLocation(shader_programme, "color"), 1.0f, 1.0f, 1.0f);
                    }
                }
                else {
                    glUniform3f(glGetUniformLocation(shader_programme, "color"), 1.0f, 1.0f, 1.0f);
                }

                glUniform1f(glGetUniformLocation(shader_programme, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);

                glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
                glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }

        glfwPollEvents();
        if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(g_window, 1);
        }

        // Atualiza a posição do tile selecionado com base nas teclas pressionadas
        if (tileSelected && !gameFinished) {
            if (keyWPressed && cy > 0 && (movementRules[cy - 1][cx] == 1 || movementRules[cy - 1][cx] == 2 || movementRules[cy - 1][cx] == 3)) {
                cy--;
                keyWPressed = false;
            }
            if (keySPressed && cy < tmap->getHeight() - 1 && (movementRules[cy + 1][cx] == 1 || movementRules[cy + 1][cx] == 2 || movementRules[cy + 1][cx] == 3)) {
                cy++;
                keySPressed = false;
            }
            if (keyAPressed && cx > 0 && (movementRules[cy][cx - 1] == 1 || movementRules[cy][cx - 1] == 2 || movementRules[cy][cx - 1] == 3)) {
                cx--;
                keyAPressed = false;
            }
            if (keyDPressed && cx < tmap->getWidth() - 1 && (movementRules[cy][cx + 1] == 1 || movementRules[cy][cx + 1] == 2 || movementRules[cy][cx + 1] == 3)) {
                cx++;
                keyDPressed = false;
            }

            // Verificar se há um item coletável ou um tile de perda de ponto no tile atual
            if (cy >= 0 && cy < movementRules.size() && cx >= 0 && cx < movementRules[cy].size()) {
                if (movementRules[cy][cx] == 2) {
                    score++;
                    cout << "Item coletado! Pontuacao: " << score << endl;
                    movementRules[cy][cx] = 1; // Remover o item coletado
                }
                else if (movementRules[cy][cx] == 3) {
                    score--;
                    cout << "Tile perigoso! Pontuacao: " << score << endl;
                    movementRules[cy][cx] = 1; // Desativar o efeito de perda de ponto
                }
            }

            // Verificar se todos os coletáveis foram recolhidos
            if (allCollectiblesCollected()) {
                double elapsedTime = current_seconds - startTime;
                showFinalScore(score, elapsedTime);
            }
        }

        double mx, my;
        glfwGetCursorPos(g_window, &mx, &my);

        const int state = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);

        if (state == GLFW_PRESS && !tileSelected) {
            mouse(mx, my);
        }

        glfwSwapBuffers(g_window);

        // Espera pela entrada do usuário antes de fechar, se o jogo estiver terminado
        if (gameFinished) {
            cout << "Pressione qualquer tecla para fechar a janela..." << endl;
            while (!glfwWindowShouldClose(g_window)) {
                glfwPollEvents();
                if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
                    glfwGetKey(g_window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                    glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    glfwSetWindowShouldClose(g_window, 1);
                }
            }
        }
    }

    glfwTerminate();
    delete tmap;
    return 0;
}
