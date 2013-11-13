/* Copyright (c) 2010 Vítor Baptista
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "parapin.h"
#include "main.h"

// Ainda não usamos todas as 60 possibilidades para facilitar o cálculo
// de qual botão está ativo, recebendo os valores do pino de status e
// de dados.
// A fórmula é: (status - 9) + [(dados - 2) * 5]
#define QNTD_BOTOES 40

#define NUM_BOTAO(status, dados)								\
	((((status == 15) ? 14 : status) - 9) +				\
	 (dados - 2) * 5)

void setupBotoes() {
  // Moedeiro
  KEYCODE_BOTAO[40] = XK_9;

  // Player 1
  // Direcionais
	KEYCODE_BOTAO[17] = XK_Up;
	KEYCODE_BOTAO[20] = XK_Down;
	KEYCODE_BOTAO[19] = XK_Left;
	KEYCODE_BOTAO[18] = XK_Right;

  // Start
	KEYCODE_BOTAO[16] = XK_space;

  // Botões de cima
	KEYCODE_BOTAO[29] = XK_1;
	KEYCODE_BOTAO[28] = XK_2;
	KEYCODE_BOTAO[27] = XK_3;
	KEYCODE_BOTAO[26] = XK_4;

  // Botões de baixo
	KEYCODE_BOTAO[34] = XK_5;
	KEYCODE_BOTAO[32] = XK_6;
	KEYCODE_BOTAO[33] = XK_7;
	KEYCODE_BOTAO[31] = XK_8;

  // Botões laterais
	KEYCODE_BOTAO[35] = KEYCODE_BOTAO[29];
	KEYCODE_BOTAO[30] = KEYCODE_BOTAO[27];


  // Player 2
  // Direcionais
	KEYCODE_BOTAO[24] = XK_w;
	KEYCODE_BOTAO[22] = XK_s;
	KEYCODE_BOTAO[21] = XK_a;
	KEYCODE_BOTAO[23] = XK_d;

  // Start
	KEYCODE_BOTAO[25] = XK_Return;

  // Botões de cima
	KEYCODE_BOTAO[7] = XK_t;
	KEYCODE_BOTAO[9] = XK_y;
	KEYCODE_BOTAO[8] = XK_u;
	KEYCODE_BOTAO[10] = XK_i;

  // Botões de baixo
	KEYCODE_BOTAO[14] = XK_g;
	KEYCODE_BOTAO[12] = XK_h;
	KEYCODE_BOTAO[13] = XK_j;
	KEYCODE_BOTAO[15] = XK_k;

  // Botões laterais
	KEYCODE_BOTAO[11] = KEYCODE_BOTAO[28];
	KEYCODE_BOTAO[6] = KEYCODE_BOTAO[34];
}

// Function to create a keyboard event
XKeyEvent createKeyEvent(Display *display, Window &win,
												 Window &winRoot, bool press,
												 int keycode, int modifiers)
{
	XKeyEvent event;

	event.display     = display;
	event.window      = win;
	event.root        = winRoot;
	event.subwindow   = None;
	event.time        = CurrentTime;
	event.x           = 1;
	event.y           = 1;
	event.x_root      = 1;
	event.y_root      = 1;
	event.same_screen = True;
	event.keycode     = XKeysymToKeycode(display, keycode);
	event.state       = modifiers;

	if(press)
		event.type = KeyPress;
	else
		event.type = KeyRelease;

	return event;
}

void enviarEvento(Display *display, Window &winRoot, int keyCode, bool isKeyPressed) {
  // Variáveis para guardar a janela com foco
  Window winFocus;
  int revert;
  XGetInputFocus(display, &winFocus, &revert);

  // Send a fake key press event to the window.
  XKeyEvent event = createKeyEvent(display, winFocus, winRoot, isKeyPressed, keyCode, 0);
  XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

  // TODO: Não sei porque, mas esta segunda chamada do método
  // parece dar um "flush" no buffer...
  XGetInputFocus(display, &winFocus, &revert);
}

int main() {
  setupBotoes();

  // Contadores
  int i, j;

  // Inicializa porta paralela
  if (pin_init_user(LPT1) < 0)
    return -1;

  pin_output_mode(LP_DATA_PINS);

  // Status anteriores dos pinos
  int OLD_PIN_STATUS[26];
  for (i = 1; i <= 25; i++)
    OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);

  // Estado anterior dos botões
  bool ESTADO_BOTAO[QNTD_BOTOES];
  for (i = 0; i < QNTD_BOTOES; i++)
    ESTADO_BOTAO[i] = false;

  // Obtém o display do X11
  Display *display = XOpenDisplay(0);
  if (!display)
    return -1;

  // Pega a raiz do display atual
  Window winRoot = XDefaultRootWindow(display);

  while (true) {
    // Seta todos os pinos de dados para 0
    // Assim conseguimos ver se os pinos de status estão
    // em curto ou não
    clear_pin(LP_DATA_PINS);

    // Loop entre os pinos de status
    for (i = 10; i <= 15; i++) {
      if (i == 14) {
        // Se estiver em curto
        if (!pin_is_set(LP_PIN[i])) {
          // E não estava em curto
          if (OLD_PIN_STATUS[i])
            enviarEvento(display, winRoot, KEYCODE_BOTAO[40], true);
        }
        else {
          if (!OLD_PIN_STATUS[i])
            enviarEvento(display, winRoot, KEYCODE_BOTAO[40], false);
        }

        OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);
        continue;
      }

      // Atualiza o estado do pino de status atual
      OLD_PIN_STATUS[i] = pin_is_set(LP_PIN[i]);

      // Para cada pino de dados
      for (j = 2; j <= 9; j++) {
        // Coloca todos em 1. Desta forma, o pino de status não
        // Ficará em curto. Depois disto, a ideia é colocar os
        // pinos de dados, um por um, em zero. Caso você coloque
        // o pino x em 0 e o pino de status fique em curto, significa
        // que o x e o de status estão ligados.
        set_pin(LP_DATA_PINS);

        clear_pin(LP_PIN[j]);

        if (!pin_is_set(LP_PIN[i])) {
          // Significa que o pino i está em curto com j

          // Se ele não estava em curto, então o jogador
          // acabou de apertá-lo.
          if (!ESTADO_BOTAO[NUM_BOTAO(i, j)]) {
            // Lança um evento KeyPress
            enviarEvento(display, winRoot, KEYCODE_BOTAO[NUM_BOTAO(i, j)], true);
          }
          ESTADO_BOTAO[NUM_BOTAO(i, j)] = true;
        } else {
          // O pino i não está em curto com j

          // Se ele estava em curto, então o jogador
          // acabou de soltá-lo.
          if (ESTADO_BOTAO[NUM_BOTAO(i, j)]) {
            // Lança um evento KeyRelease
            enviarEvento(display, winRoot, KEYCODE_BOTAO[NUM_BOTAO(i, j)], false);
          }
          ESTADO_BOTAO[NUM_BOTAO(i, j)] = false;
        }

			  usleep(10);
      }
    }
  }

  // Done.
  XCloseDisplay(display);

  return 0;
}
