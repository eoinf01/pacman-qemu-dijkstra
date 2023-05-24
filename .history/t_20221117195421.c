/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include "defines.h"
#include "string.c"

int spriteMove = 0;
char *tab = "0123456789ABCDEF";
int color;

#include "timer.c"

#include "interrupts.c"
#include "kbd.c"
#include "uart.c"
#include "vid.c"

extern char _binary_panda_bmp_start;
extern char _binary_pacman_bmp_start;
extern char _binary_speedy_bmp_start;
extern char _binary_pokey_bmp_start;
extern char _binary_shadow_bmp_start;
extern char _binary_power_bmp_start;
extern char _binary_mur_bmp_start;
extern char _binary_point_bmp_start;

enum Objet
{
   V,
   M,
   P,
   W
};
#define Width 19
#define Height 21
// Tableau de la carte
enum Objet table[Height][Width] =
    {
        {M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M},
        {M, P, P, P, P, P, P, P, P, M, P, P, P, P, P, P, P, P, M},
        {M, M, M, M, P, M, M, M, P, M, P, M, M, M, P, M, M, W, M},
        {M, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, M},
        {M, P, M, M, P, M, P, M, M, M, M, M, P, M, P, M, M, P, M},
        {M, P, P, P, P, M, P, P, P, M, P, P, P, M, P, P, P, P, M},
        {M, M, M, M, P, M, M, M, V, M, V, M, M, M, P, M, M, M, M},
        {V, V, V, M, P, M, V, V, V, V, V, V, V, M, P, M, V, V, V},
        {M, M, M, M, P, M, V, M, M, V, M, M, V, M, P, M, M, M, M},
        {V, V, V, V, P, V, V, M, V, V, V, M, V, V, P, V, V, V, V},
        {M, M, M, M, P, M, V, M, M, M, M, M, V, M, P, M, M, M, M},
        {V, V, V, M, P, M, V, V, V, V, V, V, V, M, P, M, V, V, V},
        {M, M, M, M, P, M, V, M, M, M, M, M, V, M, P, M, M, M, M},
        {M, P, P, P, P, P, P, P, P, M, P, P, P, P, P, P, P, P, M},
        {M, P, M, M, P, M, M, M, P, M, P, M, M, M, P, M, M, P, M},
        {M, W, P, M, P, P, P, P, P, P, P, P, P, P, P, M, P, W, M},
        {M, M, P, M, P, M, P, M, M, M, M, M, P, M, P, M, P, M, M},
        {M, P, P, P, P, M, P, P, P, M, P, P, P, M, P, P, P, P, M},
        {M, P, M, M, M, M, M, M, P, M, P, M, M, M, M, M, M, P, M},
        {M, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, M},
        {M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M, M},
};

#include "dijkstra.c"
struct sprite
{
   int x, y;
   int buff[16][16];
   int replacePix;
   char *p;
   int oldstartRow;
   int oldstartCol;
   int pathpos;
   int pathlen;
   int pathV[NoV];
   int path;
};
struct sprite sprites[3];
void copy_vectors(void)
{
   extern u32 vectors_start;
   extern u32 vectors_end;
   u32 *vectors_src = &vectors_start;
   u32 *vectors_dst = (u32 *)0;

   while (vectors_src < &vectors_end)
      *vectors_dst++ = *vectors_src++;
}
int kprintf(char *fmt, ...);
void timer_handler();

void uart0_handler()
{
   uart_handler(&uart[0]);
}
void uart1_handler()
{
   uart_handler(&uart[1]);
}

// IRQ interrupts handler entry point
// void __attribute__((interrupt)) IRQ_handler()
// timer0 base=0x101E2000; timer1 base=0x101E2020
// timer3 base=0x101E3000; timer1 base=0x101E3020
// currentValueReg=0x04
TIMER *tp[4];

void IRQ_handler()
{
   int vicstatus, sicstatus;
   int ustatus, kstatus;

   // read VIC SIV status registers to find out which interrupt
   vicstatus = VIC_STATUS;
   sicstatus = SIC_STATUS;
   // kprintf("vicstatus=%x sicstatus=%x\n", vicstatus, sicstatus);
   //  VIC status BITs: timer0,1=4, uart0=13, uart1=14, SIC=31: KBD at 3
   /**************
   if (vicstatus & 0x0010){   // timer0,1=bit4
     if (*(tp[0]->base+TVALUE)==0) // timer 0
        timer_handler(0);
     if (*(tp[1]->base+TVALUE)==0)
        timer_handler(1);
   }
   if (vicstatus & 0x0020){   // timer2,3=bit5
      if(*(tp[2]->base+TVALUE)==0)
        timer_handler(2);
      if (*(tp[3]->base+TVALUE)==0)
        timer_handler(3);
   }
   if (vicstatus & 0x80000000){
      if (sicstatus & 0x08){
         kbd_handler();
      }
   }
   *********************/
   /******************
   if (vicstatus & (1<<4)){   // timer0,1=bit4
     if (*(tp[0]->base+TVALUE)==0) // timer 0
        timer_handler(0);
     if (*(tp[1]->base+TVALUE)==0)
        timer_handler(1);
   }
   if (vicstatus & (1<<5)){   // timer2,3=bit5
      if(*(tp[2]->base+TVALUE)==0)
        timer_handler(2);
      if (*(tp[3]->base+TVALUE)==0)
        timer_handler(3);
   }
   *********************/
   if (vicstatus & (1 << 4))
   { // timer0,1=bit4
      timer_handler(0);
   }
   if (vicstatus & (1 << 12))
   { // bit 12: uart0
      uart0_handler();
   }
   if (vicstatus & (1 << 13))
   { // bit 13: uart1
      uart1_handler();
   }

   if (vicstatus & (1 << 31))
   {
      if (sicstatus & (1 << 3))
      {
         //   kbd_handler();
      }
   }
}

int abs(int a)
{
   return a < 0 ? -a : a;
}
extern int oldstartR;
extern int oldstartC;
extern int replacePix;
extern int buff[16][16];
struct sprite sprites[3];

int main()
{
   int i;
   int k, j, l;
   char line[128], string[32];
   UART *up;
   char score[16];
   int scoreValue = 0;
   strcpy((char *)score, "Score: ");
   // start pacman at 3,1 in the map
   int x = 16;
   int y = 3 * 16;
   int x1, y1;
   sprites[0].x = 16;
   sprites[0].y = 16;
   sprites[0].replacePix = 0;
   sprites[0].p = &_binary_speedy_bmp_start;
   sprites[1].x = 16 * 4;
   sprites[1].y = 16 * 4;
   sprites[1].replacePix = 0;
   sprites[1].p = &_binary_pokey_bmp_start;
   sprites[2].x = 16 * 16;
   sprites[2].y = 16 * 8;
   sprites[2].replacePix = 0;
   sprites[2].p = &_binary_shadow_bmp_start;
   char *p;
   color = YELLOW;
   row = col = 0;
   fbuf_init();

   /* enable timer0,1, uart0,1 SIC interrupts */
   VIC_INTENABLE |= (1 << 4); // timer0,1 at bit4
   VIC_INTENABLE |= (1 << 5); // timer2,3 at bit5

   VIC_INTENABLE |= (1 << 12); // UART0 at 12
   VIC_INTENABLE |= (1 << 13); // UART1 at 13

   UART0_IMSC = 1 << 4; // enable UART0 RXIM interrupt
   UART1_IMSC = 1 << 4; // enable UART1 RXIM interrupt

   VIC_INTENABLE |= 1 << 31; // SIC to VIC's IRQ31

   /* enable KBD IRQ */
   // SIC_ENSET = 1<<3;  // KBD int=3 on SIC
   // SIC_PICENSET = 1<<3;  // KBD int=3 on SIC

   // kprintf("C3.3 start: Interrupt-driven drivers for Timer KBD UART\n");
   timer_init();
   /***********
   for (i=0; i<4; i++){
      tp[i] = &timer[i];
      timer_start(i);
   }
   ************/
   timer_start(0);
   kbd_init();

   uart_init();
   up = &uart[0];
   for (int j = 0; j < Height; j++)
      for (int i = 0; i < Width; i++)
      {
         if (table[j][i] == M)
         {
            show_bmp1(&_binary_mur_bmp_start, j * 16, i * 16);
         }
         if (table[j][i] == P)
         {
            show_bmp1(&_binary_point_bmp_start, j * 16, i * 16);
         }
      }
   show_bmp1(p, 0, 80);

   p = &_binary_pacman_bmp_start;
   show_bmp(p, y, x, buff, replacePix, &oldstartR, &oldstartC);
   show_bmp(sprites[0].p, sprites[0].y, sprites[0].x, sprites[0].buff, sprites[0].replacePix,
            &(sprites[0].oldstartRow), &(sprites[0].oldstartCol));
   show_bmp(sprites[1].p, sprites[1].y, sprites[1].x, sprites[0].buff, sprites[1].replacePix,
            &(sprites[1].oldstartRow), &(sprites[1].oldstartCol));
   show_bmp(sprites[2].p, sprites[2].y, sprites[2].x, sprites[2].buff, sprites[2].replacePix,
            &(sprites[2].oldstartRow), &(sprites[2].oldstartCol));

   /* TODO manually set the following two edges for the drawing below. Remove. */
   graph[21][22] = 1;
   graph[22][23] = 1;
   for (int j = 0; j < Height; j++)
   {
      for (int i = 0; i < Width; i++)
      {
         // Construct the edges in the graph
         if (table[j][i] != M)
         {
            if (i + 1 < Width && table[j][i + 1] != M)
            {
               /*TODO */
               // need and edge from the vertex associated with table[j][i] to the vertex associated
               // with table[j][i+1]
               // graph[n][m] = 1; creates an edge from vertex n to vertex m
               // the unique vertex for a square that pacman or the ghost at j,i  is j*Width +i
               // graph[i][j+1] = 1;
               uprintf("Edge between %d and %d", (j)*Width + (i), (j)*Width + ((i + 1)));
               graph[(j)*Width + (i)][(j)*Width + ((i + 1))] = 1;
            }
            if (j + 1 < Height && table[j + 1][i] != M)
            {
               /*TODO */
               // need and edge from the vertex associated with table[j][i] to the vertex associated
               // with table[j+1][i]
               // graph[n][m] = 1; creates an edge from vertex n to vertex m
               uprintf("Edge between %d and %d", (j)*Width + (i), ((j + 1)) * Width + (i));
               graph[(j)*Width + (i)][((j + 1)) * Width + (i)] = 1;
            }
         }
      }
   }
   /* the following code will draw in a line for every edge set can be removed */
   for (int j = 0; j < NoV; j++)
      for (int i = 0; i < NoV; i++)
         if (graph[j][i] == 1)
         {
            // uprintf("%d %d\n", j, i);
            if (i > j ? i - j < Width : j - i < Width)
               line_fast((j % Width) * 16 + 12, (j / Width) * 16 + 4, (i % Width) * 16 + 4, (i / Width) * 16 + 4);
            else if (i > j ? i - j >= Width : j - i >= Width)
               line_fast((j % Width) * 16 + 4, (j / Width) * 16 + 12, (i % Width) * 16 + 4, (i / Width) * 16 + 4);
            graph[i][j] = 2;
            // graph[i][j] =2;
         }
   unlock();

   int move = 0;
   int key;
   int hit = 0;
   int dead = 0;

   while (1)
   {
      // uprintf("enter a key from this UART : ");
      // uprintf("rand %d\n",(rand()+1)%5);
      move = 0;
      replacePix = 1;
      if (upeek(up) && dead != 1)
      {
         key = ugetc(up);
         switch (key)
         {
         case 'a':
            if (x > 0 && table[(y) >> 4][(x - 16) >> 4] != M)
            {
               uprintf("a key entered");
               x -= 16;
               move = 1;
            }

            break;
         case 'd':
            if (x < 420 && table[(y) >> 4][(x + 16) >> 4] != M)
            {
               x += 16;
               move = 1;
            }
            break;

         case 'w':
            if (y > 0 && table[(y - 16) >> 4][(x) >> 4] != M)
            {
               y -= 16;
               move = 1;
            }
            break;
         case 's':
            if (y < 420 && table[(y + 16) >> 4][(x) >> 4] != M)
            {
               y += 16;
               move = 1;
            }
            break;
         default:
            move = 0;
         }
      }
      if (move)
      {

         if (table[y >> 4][x >> 4] == P)
         {
            scoreValue++;

            for (i = 0; i < 11; i++)
            {
               unkpchar(score[i], 1, 69 + i);
            }
               if (scoreValue < 100)
               {
                  score[8] = '0' + (scoreValue / 10);
                  score[9] = '0' + (scoreValue % 10);
               }
               if(scoreValue >=100){
                  score[8] = '0' + ((scoreValue / 10) / 10);
                  score[9] = '0' + ((scoreValue / 10) % 10);
                  score[10] = '0' + (scoreValue % 10);
               }
               
               for (i = 0; i < 11; i++)
               {
                  kpchar(score[i], 1, 69 + i);
               }
               // kprintf("Score:%d", score);
               // clrcursor();
               black_point(x, y);
               table[y >> 4][x >> 4] = V;
               uprintf("point\n");
            }
            show_bmp(p, y, x, buff, replacePix, &oldstartR, &oldstartC);
         }
         if (spriteMove)
         {
            for (int l = 0; l < 2; l++)
            {
               if (sprites[l].x == x && sprites[l].y == y)
               {
                  if (!dead)
                     // ead
                     dead = 0;
                  continue;
               }
               dead = 0;
               if (!sprites[l].path)
               {
                  // TODO calculate the vertex where the ghost is and the vertex where pacman is and
                  // call dijkstra accordingly - hint y>>4 divides y by 16 and x>>4 divides x by 16.
                  // Note that the x,y is the absolute pixel position of the ghost or pacman respectively.
                  // The dijkstra function will fill the soln
                  // array with the vertices of the optimal path. solnlen will hold the number of vertices in the path.
                  // If solnlen is zero then there is no path.
                  //
                  int vpac = (y >> 4) * Width + (x >> 4);
                  int vghost = (((sprites[l].y) >> 4) * Width) + ((sprites[l].x) >> 4);

                  dijkstra(graph, vghost, vpac);

                  if (solnlen)
                  {
                     // copy the solution to the ghost's path list
                     for (i = 0; i < solnlen; i++)
                     {
                        sprites[l].pathV[i] = soln[i];
                     }
                     /* TODO set the x,y position of the ghost to absolute pixel position associated with the
                            first vertex in the soln path.
                            Note:- the unique vertex for a square that pacman or the ghost at j,i  is j*Width +i
                            Note also that x,y is the absolute pixel position of the ghost
              */
                     int pos = sprites[l].pathpos;
                     int vertex = sprites[l].pathV[pos];

                     sprites[l].x = (sprites[l].pathV[pos] % Width) * 16;
                     sprites[l].y = (sprites[l].pathV[pos] / Width) * 16;
                     sprites[l].pathpos = 1;
                     sprites[l].pathlen = solnlen;
                     sprites[l].path = 1;
                     sprites[l].replacePix = 1;
                     show_bmp(sprites[l].p, sprites[l].y, sprites[l].x, sprites[l].buff, sprites[l].replacePix,
                              &(sprites[l].oldstartRow), &(sprites[l].oldstartCol));

                     speed = 0;
                     spriteMove = 0;
                  }
                  else
                  {
                     uprintf("fuck you");
                     sprites[l].x += 16;
                  }
               }
               else if (sprites[l].pathpos == sprites[l].pathlen)
               {
                  sprites[l].path = 0;
                  sprites[l].pathpos = 0; // get new path next time
               }
               else if (sprites[l].pathpos < sprites[l].pathlen)
               {

                  int pos = sprites[l].pathpos;
                  int vertex = sprites[l].pathV[pos];
                  /* TODO set the x,y position of the ghost to x,y position associated with the
                               next vertex in the path.
                               Note:- the unique vertex for a square that pacman or the ghost at j,i  is j*Width +i
                 */
                  sprites[l].x = (sprites[l].pathV[pos] % Width) * 16;
                  sprites[l].y = (sprites[l].pathV[pos] / Width) * 16;

                  sprites[l].pathpos++;
                  sprites[l].replacePix = 1;
                  show_bmp(sprites[l].p, sprites[l].y, sprites[l].x, sprites[l].buff, sprites[l].replacePix,
                           &(sprites[l].oldstartRow), &(sprites[l].oldstartCol));

                  speed = 1;
                  spriteMove = 0;
               }
            }
         }
      }
   }
