/* D-Side 1 — Ville abandonnée
 * Original Celeste-inspired precision platformer for NumWorks N0120.
 * Arrows move/aim, OK jumps, Back dashes, Toolbox grabs walls, Backspace pauses.
 */
#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "../../common/epsilon_app.h"
#include "../../../launcher/src/gfx.h"
#include "../../common/np_text.h"

#ifndef NP_MODULE
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "D-Side 1";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif
void np_push(int x,int y,int w,int h,const color_t *px) {
  eadk_display_push_rect((eadk_rect_t){(uint16_t)x,(uint16_t)y,(uint16_t)w,(uint16_t)h},px);
}
#define KEY(k) (1ull << (k))
#define W 320
#define H 240
#define TICK 16
#define PW 10
#define PH 14
#define ROOMS 5
#define RW 640
#define DASH_TIME 9
#define MAX_PART 18

typedef struct { int x,y,w,h; } Box;
typedef struct { int x,y; float vx,vy; int life,kind; } Particle;
typedef struct { int x,y; int active; } Orb;

/* Each room is two screens wide. Platforms are hand-built routes, not repeated
   screen-sized obstacle strips. Coordinates are local to the room. */
static const Box platforms[ROOMS][12]={
 {{0,222,150,30},{190,190,58,10},{285,158,58,10},{375,190,54,10},{465,154,60,10},{548,205,92,35},{150,0,14,164},{343,0,14,126},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}},
 {{0,222,95,30},{132,190,50,10},{222,158,52,10},{316,190,48,10},{405,148,54,10},{505,184,48,10},{570,222,70,30},{95,0,14,152},{278,0,14,118},{470,0,14,105},{0,0,1,1},{0,0,1,1}},
 {{0,222,80,30},{112,184,46,10},{198,146,46,10},{284,112,48,10},{376,150,50,10},{470,184,48,10},{558,222,82,30},{80,0,14,144},{245,0,14,112},{430,0,14,126},{0,0,1,1},{0,0,1,1}},
 {{0,222,110,30},{145,184,46,10},{230,148,48,10},{320,112,48,10},{410,148,48,10},{500,184,50,10},{585,222,55,30},{110,0,14,154},{290,0,14,80},{475,0,14,150},{0,0,1,1},{0,0,1,1}},
 {{0,222,82,30},{118,184,44,10},{202,148,44,10},{286,112,44,10},{370,148,44,10},{454,184,44,10},{538,148,42,10},{600,222,40,30},{82,0,14,142},{270,0,14,78},{450,0,14,140},{0,0,1,1}}
};
static const Box spikes[ROOMS][5]={
 {{150,214,40,8},{248,214,37,8},{343,214,42,8},{429,214,36,8},{525,214,23,8}},
 {{95,214,37,8},{182,214,40,8},{274,214,42,8},{364,214,41,8},{459,214,46,8}},
 {{80,214,32,8},{158,214,40,8},{244,214,40,8},{332,214,44,8},{426,214,44,8}},
 {{110,214,35,8},{191,214,39,8},{278,214,42,8},{368,214,42,8},{458,214,42,8}},
 {{82,214,36,8},{162,214,40,8},{246,214,40,8},{330,214,40,8},{414,214,40,8}}
};
static const Orb orbs[ROOMS][2]={
 {{300,132,1},{490,128,1}},{{235,130,1},{510,156,1}},{{300,84,1},{488,128,1}},
 {{330,84,1},{520,156,1}},{{300,84,1},{470,156,1}}
};
static const char *room_names[ROOMS]={"LE DERNIER TOIT","LES RUELLES","LE CHANTIER","LE CLOCHER","AU-DESSUS DES NUAGES"};
static color_t strip[W*STRIP_H];
static Particle particles[MAX_PART];
static int room=0, deaths=0, frames=0, state=0, dash=0, dash_used=0;
static int px=22,py=194,cam=0,ground=0,wall=0,grab=0,coyote=0,jump_buffer=0;
static float vx=0,vy=0;
static int shake=0,orb_taken[ROOMS][2],trail=0,death_flash=0;
static uint64_t previous=0;
static uint32_t rng=0xA5317u;
static int best_room=0;

enum { MENU, PLAY, PAUSE, COMPLETE, HELP };
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool overlap(int x,int y,int w,int h,Box b){return x<b.x+b.w&&x+w>b.x&&y<b.y+b.h&&y+h>b.y;}
static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}
static void particle(int x,int y,int kind){
 for(int i=0;i<MAX_PART;i++)if(!particles[i].life){
  particles[i]=(Particle){x,y,((int)(rnd()%65)-32)/16.0f,-(int)(rnd()%45)/12.0f,14+(int)(rnd()%15),kind};break;
 }
}
static void burst(int x,int y,int n,int kind){for(int i=0;i<n;i++)particle(x,y,kind);}
static void reset_room(void){
 px=room==0?22:24;py=194;vx=vy=0;dash=0;dash_used=0;ground=wall=grab=coyote=jump_buffer=0;shake=0;
}
static void start_game(void){room=0;deaths=0;frames=0;best_room=0;for(int r=0;r<ROOMS;r++)for(int o=0;o<2;o++)orb_taken[r][o]=0;reset_room();state=PLAY;}
static void die(void){deaths++;death_flash=9;shake=7;burst(px+PW/2,py+PH/2,12,0);reset_room();}
static bool solid_at(int x,int y){
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w>1&&b.h>1&&x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return true;}
 return false;
}
static void update_player(uint64_t keys,uint64_t pressed){
 int dir=(!!(keys&KEY(eadk_key_right)))-(!!(keys&KEY(eadk_key_left)));
 int aimx=dir,aimy=(!!(keys&KEY(eadk_key_down)))-(!!(keys&KEY(eadk_key_up)));
 bool grabbing=(keys&KEY(eadk_key_toolbox))!=0;
 if(pressed&KEY(eadk_key_ok))jump_buffer=7;else if(jump_buffer)jump_buffer--;
 if(ground)coyote=7;else if(coyote)coyote--;
 wall=0;
 if(!ground){
  if(solid_at(px-2,py+4)||solid_at(px-2,py+PH-3))wall=-1;
  if(solid_at(px+PW+1,py+4)||solid_at(px+PW+1,py+PH-3))wall=1;
 }
 if(grabbing&&wall&& !ground){grab=1;}else grab=0;
 if(pressed&KEY(eadk_key_back) && !dash_used){
  if(!aimx&&!aimy)aimx=dir?dir:1;
  float len=(aimx&&aimy)?1.41421356f:1.0f;
  vx=aimx*5.8f/len;vy=aimy*5.8f/len;dash=DASH_TIME;dash_used=1;
  shake=2;burst(px+PW/2,py+PH/2,5,1);
 }
 if(dash){
  px+=(int)lrintf(vx);py+=(int)lrintf(vy);dash--;
  if(dash==0){vx*=0.45f;vy*=0.45f;}
 }else{
  float target=dir*2.25f;
  vx+=(target-vx)*(ground?0.42f:0.20f);
  if(!dir&&ground)vx*=0.72f;
  if(grab){vy=vy<1.2f?vy:1.2f;vx=0;}
  else {vy+=0.42f;if(vy>6.2f)vy=6.2f;}
  if(jump_buffer&&(ground||coyote)){
   vy=-7.6f;jump_buffer=0;coyote=0;ground=0;burst(px+PW/2,py+PH,3,0);
  }else if(jump_buffer&&wall){
   vy=-7.2f;vx=-wall*4.8f;jump_buffer=0;burst(px+PW/2,py+PH/2,4,0);
  }
  /* horizontal collision */
  px+=(int)lrintf(vx);
  for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w<2)continue;
   if(overlap(px,py,PW,PH,b)){if(vx>0)px=b.x-PW;else if(vx<0)px=b.x+b.w;vx=0;}
  }
  /* vertical collision */
  int oldy=py;py+=(int)lrintf(vy);ground=0;
  for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w<2)continue;
   if(overlap(px,py,PW,PH,b)){
    if(vy>0&&oldy+PH<=b.y+3){py=b.y-PH;vy=0;ground=1;dash_used=0;}
    else if(vy<0&&oldy>=b.y+b.h-2){py=b.y+b.h;vy=0;}
   }
  }
 }
 px=clampi(px,0,RW-PW);
 if(py>H+20||py<-30){die();return;}
 for(int i=0;i<5;i++)if(spikes[room][i].w&&overlap(px,py,PW,PH,spikes[room][i])){die();return;}
 for(int i=0;i<2;i++)if(!orb_taken[room][i]&&overlap(px,py,PW,PH,(Box){orbs[room][i].x-7,orbs[room][i].y-7,14,14})){
  orb_taken[room][i]=1;dash_used=0;burst(orbs[room][i].x,orbs[room][i].y,10,1);
 }
 if(px>RW-24){
  if(room==ROOMS-1){state=COMPLETE;burst(px,py,MAX_PART,1);}
  else {room++;if(room>best_room)best_room=room;reset_room();}
 }
 if(py<0)py=0;
}
static void update(uint64_t keys,uint64_t pressed){
 if(state==MENU){if(pressed&KEY(eadk_key_ok))start_game();if(pressed&KEY(eadk_key_back))state=HELP;return;}
 if(state==HELP){if(pressed&(KEY(eadk_key_ok)|KEY(eadk_key_back)))state=MENU;return;}
 if(state==COMPLETE){if(pressed&KEY(eadk_key_ok))start_game();return;}
 if(state==PAUSE){if(pressed&KEY(eadk_key_backspace))state=PLAY;else if(pressed&KEY(eadk_key_ok))start_game();else if(pressed&KEY(eadk_key_back))state=MENU;return;}
 if(pressed&KEY(eadk_key_backspace)){state=PAUSE;return;}
 update_player(keys,pressed);
 frames++;
 for(int i=0;i<MAX_PART;i++)if(particles[i].life){particles[i].x+=(int)lrintf(particles[i].vx);particles[i].y+=(int)lrintf(particles[i].vy);particles[i].vy+=0.12f;particles[i].life--;}
 if(shake)shake--;
 if(death_flash)death_flash--;
}
static int screen_x(int x){return x-cam;}
static void draw_spikes(Box b){
 int x0=screen_x(b.x),n=b.w/8;
 for(int i=0;i<n;i++){int x=x0+i*8;gfx_fill(x+3,b.y+1,2,2,0xF6A2A8);gfx_fill(x+2,b.y+3,4,2,0xF05B68);gfx_fill(x+1,b.y+5,6,3,0xD83E5C);}
}
static void draw_city(void){
 /* distant skyline, window lights and drifting clouds */
 for(int i=0;i<9;i++){
  int x=(i*91-(cam/4)%91)-12,h=70+(i*37%75),y=211-h;
  gfx_fill(x,y,45,h,0x1A2742);gfx_fill(x+5,y+7,35,3,0x34435E);
  for(int wy=y+17;wy<205;wy+=17)for(int wx=x+9;wx<x+40;wx+=13)
   if(((wx+wy+i*3)/13)%3)gfx_fill(wx,wy,4,5,0x4B6279);
 }
 for(int i=0;i<5;i++){int x=(i*103-(cam/2)%103+W)%W;gfx_rrect(x,54+i%3*19,32,3,2,0x42516B,100);}
}
static void draw_room(void){
 gfx_vgrad(0,0,W,H,0x0B142B,0x263552);draw_city();
 /* moon */
 gfx_circle(263-(cam/8)%30,52,15,0xE3D9C2,180);gfx_circle(269-(cam/8)%30,46,15,0x17233D,220);
 /* background building silhouettes */
 gfx_fill(0,210,W,30,0x111B31);
 for(int i=0;i<12;i++){int x=(i*31-(cam/2)%31);gfx_fill(x,174+(i*19%30),18,40,0x26334D);}
 /* platforms */
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w<2)continue;int x=screen_x(b.x);
  gfx_fill(x,b.y,b.w,b.h,0x56647B);gfx_fill(x,b.y,b.w,3,0xA5B4C8);
  gfx_fill(x,b.y+b.h-3,b.w,3,0x35445D);
  for(int k=8;k<b.w-5;k+=19){gfx_fill(x+k,b.y+5,2,2,0x8190A5);}
 }
 for(int i=0;i<5;i++)draw_spikes(spikes[room][i]);
 /* crystal dash refills */
 for(int i=0;i<2;i++)if(!orb_taken[room][i]){
  int x=screen_x(orbs[room][i].x),y=orbs[room][i].y;
  gfx_circle(x,y,10,0x48D9D1,36);gfx_rrect(x-4,y-7,8,14,3,0x75F1E4,245);
  gfx_fill(x-2,y-5,2,4,0xE0FFFF);
 }
 /* exit beacon */
 gfx_fill(screen_x(RW-24),150,3,72,0x58E5D4);gfx_circle(screen_x(RW-22),148,5,0xD4FFF7,240);
 /* Madeline: hair, body, face, scarf */
 int x=screen_x(px),y=py;
 if(dash){gfx_rrect(x-(int)(vx*1.3f),y-(int)(vy*1.3f)+3,9,8,4,0x70E9DF,120);}
 gfx_rrect(x-1,y+2,12,12,4,0xD94E6B,255);gfx_fill(x+1,y+1,8,5,0xF08A9A);
 gfx_fill(x+6,y+5,2,2,0xF9E7D5);gfx_fill(x+8,y+8,3,3,0x9B354E);
 gfx_fill(x+1,y+12,3,2,0x26334D);gfx_fill(x+7,y+12,3,2,0x26334D);
 if(dash){for(int i=0;i<3;i++)gfx_fill(x-5-i*4,y+5+i*2,3,2,0x70E9DF);}
 /* particles */
 for(int i=0;i<MAX_PART;i++)if(particles[i].life){Particle p=particles[i];int a=p.life*16;
  gfx_circle(screen_x(p.x),p.y,p.kind?2:1,p.kind?0x70E9DF:0xF4B8C3,a);
 }
 /* HUD */
 gfx_fill(0,0,W,25,0x091124);gfx_fill(0,24,W,1,0x3B4C68);
 gfx_text(&np_font_small,7,16,"VILLE ABANDONNEE",0xFFFF,245);
 gfx_text_right(&np_font_small,312,16,room_names[room],0xFFD875,255);
 gfx_text(&np_font_small,8,40,"DASH",0xD8E6F4,230);
 gfx_rrect(43,30,13,8,3,dash_used?0x536078:0x70E9DF,255);
 gfx_text_right(&np_font_small,311,40,"TOOLBOX: ACCROCHER",0xAFC2D8,210);
 gfx_text(&np_font_small,8,232,"MORTS",0xAFC2D8,210);
 char n[8];int v=deaths,k=0;do{n[k++]=(char)('0'+v%10);v/=10;}while(v&&k<6);for(int i=0;i<k/2;i++){char t=n[i];n[i]=n[k-1-i];n[k-1-i]=t;}n[k]=0;
 gfx_text(&np_font_small,48,232,n,0xFFFFFF,255);
 if(state!=PLAY){
  gfx_fill_alpha(0,0,W,H,0x070D1B,185);gfx_shadow(39,55,242,132,13,8,130);gfx_rrect(39,55,242,132,13,0x263653,255);
  gfx_rrect_outline(39,55,242,132,13,1,0x7187A7,170);
  if(state==MENU){
   gfx_text_center(&np_font_title,160,87,"D-SIDE 1",0xFFD875,255);
   gfx_text_center(&np_font_body,160,111,"VILLE ABANDONNEE",0xFFFFFF,255);
   gfx_paragraph(&np_font_small,160,137,195,13,"Un nouveau defi. Cinq quartiers. Un seul dash.",0xC5D5E8,255);
   gfx_text_center(&np_font_body,160,174,"OK  POUR JOUER",0x70E9DF,255);
  }else if(state==HELP){
   gfx_text_center(&np_font_title,160,83,"COMMANDES",0xFFD875,255);
   gfx_text_center(&np_font_small,160,111,"Fleches : courir et viser",0xFFFFFF,255);
   gfx_text_center(&np_font_small,160,128,"OK : sauter   BACK : dash",0xFFFFFF,255);
   gfx_text_center(&np_font_small,160,145,"TOOLBOX : s'accrocher au mur",0xFFFFFF,255);
   gfx_text_center(&np_font_small,160,174,"OK OU BACK POUR RETOUR",0x70E9DF,255);
  }else if(state==PAUSE){
   gfx_text_center(&np_font_title,160,92,"PAUSE",0xFFD875,255);
   gfx_text_center(&np_font_body,160,121,"OK : recommencer la salle",0xFFFFFF,255);
   gfx_text_center(&np_font_body,160,145,"BACK : menu principal",0xFFFFFF,255);
   gfx_text_center(&np_font_small,160,174,"BACKSPACE : reprendre",0x70E9DF,255);
  }else{
   gfx_text_center(&np_font_title,160,91,"CHAPITRE TERMINE",0xFFD875,255);
   gfx_text_center(&np_font_body,160,120,"Tu as traverse la ville !",0xFFFFFF,255);
   gfx_text_center(&np_font_small,160,148,"OK POUR RECOMMENCER",0x70E9DF,255);
   gfx_text_center(&np_font_small,160,169,"MORTS : regarde si tu peux faire mieux",0xC5D5E8,255);
  }
 }
 if(death_flash)gfx_fill_alpha(0,0,W,H,0xF05B68,death_flash*9);
}
static void scene(void *ctx){(void)ctx;draw_room();}
int main(void){
 np_app_begin();color_t *old=gfx_buf;gfx_buf=strip;
 uint32_t next=(uint32_t)eadk_timing_millis();previous=eadk_keyboard_scan();
 for(;;){
  uint64_t keys=eadk_keyboard_scan(),pressed=keys&~previous;previous=keys;
  if(keys&(KEY(eadk_key_home)|KEY(eadk_key_on_off)))break;
  uint32_t now=(uint32_t)eadk_timing_millis();
  for(int n=0;n<4&&(int32_t)(now-next)>=0;n++,next+=TICK)
   update(keys,pressed);
  if((int32_t)(now-next)>4*TICK)next=now;
  int target=room*RW+clampi(px-118,0,RW-W);
  cam+=(target-cam)/4;
  if(shake)cam+=((int)(rnd()%5)-2);
  cam=clampi(cam,room*RW,room*RW+RW-W);
  gfx_render(scene,0,0,H);eadk_timing_msleep(1);
 }
 gfx_buf=old;return np_app_end();
}
