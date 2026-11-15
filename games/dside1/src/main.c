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
 /* 0 — first ascent: short gaps teach the buffered jump, then a wall-top route. */
 {{0,222,126,30},{160,190,44,10},{245,158,44,10},{330,126,14,96},{360,126,48,10},{450,94,48,10},{535,126,42,10},{590,222,50,30},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}},
 /* 1 — staggered roofs with a tall wall for a wall-slide / wall-jump recovery. */
 {{0,222,88,30},{120,190,44,10},{205,158,44,10},{290,126,44,10},{365,94,14,128},{395,126,48,10},{485,158,48,10},{575,222,65,30},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}},
 /* 2 — longer dash gaps and a high refill route. */
 {{0,222,92,30},{122,190,42,10},{205,158,42,10},{285,126,42,10},{365,94,42,10},{450,126,14,96},{480,158,44,10},{570,222,70,30},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}},
 /* 3 — alternating heights, designed around chaining jump, dash and wall-jump. */
 {{0,222,100,30},{130,190,42,10},{210,158,42,10},{290,126,42,10},{370,94,14,128},{400,126,42,10},{485,158,42,10},{575,222,65,30},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,0,1,1}},
 /* 4 — final ascent and a narrow landing before the exit. */
 {{0,222,84,30},{114,190,42,10},{194,158,42,10},{274,126,14,96},{304,126,42,10},{384,94,42,10},{464,126,42,10},{544,94,42,10},{600,222,40,30},{0,0,1,1},{0,0,1,1},{0,0,1,1}}
};
static const Box spikes[ROOMS][5]={
 {{126,214,34,8},{204,214,41,8},{289,214,41,8},{408,214,42,8},{498,214,37,8}},
 {{88,214,32,8},{164,214,41,8},{249,214,41,8},{334,214,31,8},{443,214,42,8}},
 {{92,214,30,8},{164,214,41,8},{247,214,38,8},{327,214,38,8},{407,214,43,8}},
 {{100,214,30,8},{172,214,38,8},{252,214,38,8},{332,214,38,8},{412,214,43,8}},
 {{84,214,30,8},{156,214,38,8},{236,214,38,8},{316,214,38,8},{396,214,38,8}}
};
static const Orb orbs[ROOMS][2]={
 {{270,132,1},{470,66,1}},{{310,98,1},{510,130,1}},{{325,98,1},{500,130,1}},
 {{330,98,1},{505,130,1}},{{395,66,1},{560,66,1}}
};
static const char *room_names[ROOMS]={"LE PREMIER ELAN","LES TOITS","LE VIDE","LES ANTENNES","LE DERNIER SAUT"};
static color_t strip[W*STRIP_H];
/* The drawing API takes RGB565; keep the level palette readable as RGB888. */
static void fillc(int x,int y,int w,int h,uint32_t c){gfx_fill(x,y,w,h,gfx_rgb(c));}
static void fillac(int x,int y,int w,int h,uint32_t c,int a){gfx_fill_alpha(x,y,w,h,gfx_rgb(c),a);}
static void roundc(int x,int y,int w,int h,int r,uint32_t c,int a){gfx_rrect(x,y,w,h,r,gfx_rgb(c),a);}
static void circlec(int x,int y,int r,uint32_t c,int a){gfx_circle(x,y,r,gfx_rgb(c),a);}
static void outlinec(int x,int y,int w,int h,int r,int t,uint32_t c,int a){gfx_rrect_outline(x,y,w,h,r,t,gfx_rgb(c),a);}
static void textc(const np_font_t *f,int x,int y,const char *s,uint32_t c,int a){gfx_text(f,x,y,s,gfx_rgb(c),a);}
static void centerc(const np_font_t *f,int x,int y,const char *s,uint32_t c,int a){gfx_text_center(f,x,y,s,gfx_rgb(c),a);}
static void rightc(const np_font_t *f,int x,int y,const char *s,uint32_t c,int a){gfx_text_right(f,x,y,s,gfx_rgb(c),a);}
static int paragraphc(const np_font_t *f,int x,int y,int w,int line,const char *s,uint32_t c,int a){return gfx_paragraph(f,x,y,w,line,s,gfx_rgb(c),a);}

static Particle particles[MAX_PART];
static int room=0, deaths=0, frames=0, state=0, dash=0, dash_used=0;
static int px=22,py=194,cam=0,ground=0,wall=0,grab=0,coyote=0,jump_buffer=0,facing=1,jump_hold=0;
static float vx=0,vy=0;
static float dash_target_x=0,dash_target_y=0,dash_accel_x=0,dash_accel_y=0;
static float rem_x=0,rem_y=0;

static int shake=0,orb_taken[ROOMS][2],trail=0,death_flash=0;
static uint64_t previous=0;
static uint32_t rng=0xA5317u;
static int best_room=0;

enum { MENU, PLAY, PAUSE, COMPLETE, HELP };
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static bool overlap(int x,int y,int w,int h,Box b){return x<b.x+b.w&&x+w>b.x&&y<b.y+b.h&&y+h>b.y;}
static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}
static int iround(float v){return (int)(v+(v>=0?0.5f:-0.5f));}
static void particle(int x,int y,int kind){
 for(int i=0;i<MAX_PART;i++)if(!particles[i].life){
  particles[i]=(Particle){x,y,((int)(rnd()%65)-32)/16.0f,-(int)(rnd()%45)/12.0f,14+(int)(rnd()%15),kind};break;
 }
}
static void burst(int x,int y,int n,int kind){for(int i=0;i<n;i++)particle(x,y,kind);}
static void reset_room(void){
 px=room==0?22:24;py=194;vx=vy=0;rem_x=rem_y=0;dash_target_x=dash_target_y=0;dash_accel_x=dash_accel_y=0;dash=0;dash_used=0;ground=wall=grab=coyote=jump_buffer=jump_hold=0;shake=0;
}
static void start_game(void){room=0;deaths=0;frames=0;best_room=0;for(int r=0;r<ROOMS;r++)for(int o=0;o<2;o++)orb_taken[r][o]=0;reset_room();state=PLAY;}
static void die(void){deaths++;death_flash=9;shake=7;burst(px+PW/2,py+PH/2,12,0);reset_room();}
static bool solid_at(int x,int y){
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w>1&&b.h>1&&x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return true;}
 return false;
}
static bool solid_box(int x,int y){
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w>1&&b.h>1&&overlap(x,y,PW,PH,b))return true;}
 return false;
}
static bool grounded_now(void){
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w>1&&b.h>1&&overlap(px,py+1,PW,PH,b))return true;}
 return false;
}
static int touching_wall(void){
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w<2||b.h<2)continue;
  if(overlap(px-1,py+2,1,PH-4,b))return -1;
  if(overlap(px+PW,py+2,1,PH-4,b))return 1;
 }
 return 0;
}
static bool move_x(int amount){
 int step=amount>0?1:-1;
 for(int n=0;n<(amount>0?amount:-amount);n++){
  px+=step;
  if(solid_box(px,py)){px-=step;vx=0;return true;}
 }
 return false;
}
static bool move_y(int amount){
 int step=amount>0?1:-1;
 for(int n=0;n<(amount>0?amount:-amount);n++){
  py+=step;
  if(solid_box(px,py)){
   py-=step;vy=0;
   if(step>0){ground=1;dash_used=0;}
   return true;
  }
 }
 return false;
}
static float approach(float value,float target,float amount){
 if(value<target){value+=amount;if(value>target)value=target;}
 else if(value>target){value-=amount;if(value<target)value=target;}
 return value;
}
static void move_player_x(void){
 rem_x+=vx;
 int dx=(int)rem_x;
 rem_x-=dx;
 if(dx&&move_x(dx))rem_x=0;
}
static void move_player_y(void){
 rem_y+=vy;
 int dy=(int)rem_y;
 rem_y-=dy;
 if(dy&&move_y(dy))rem_y=0;
}
static void update_player(uint64_t keys,uint64_t pressed){
 int dir=(!!(keys&KEY(eadk_key_right)))-(!!(keys&KEY(eadk_key_left)));
 int aimx=dir,aimy=(!!(keys&KEY(eadk_key_down)))-(!!(keys&KEY(eadk_key_up)));
 bool grabbing=(keys&KEY(eadk_key_toolbox))!=0;
 bool jump_pressed=(pressed&KEY(eadk_key_ok))!=0;
 bool dash_pressed=(pressed&KEY(eadk_key_back))!=0;
 bool on_ground=grounded_now();
 ground=on_ground;
 if(jump_pressed)jump_buffer=4;else if(jump_buffer>0)jump_buffer--;
 if(dir)facing=dir;
 if(on_ground)coyote=6;else if(coyote>0)coyote--;
 wall=on_ground?0:touching_wall();

 /* Celeste Classic's published movement model, rescaled for this game's
    larger rooms and 60 Hz update: acceleration, grace frames, jump buffer,
    wall slide/jump and the short accelerating dash. */
 grab=grabbing&&wall&&!on_ground&&!dash;
 if(dash>0){
  dash--;
  vx=approach(vx,dash_target_x,dash_accel_x);
  vy=approach(vy,dash_target_y,dash_accel_y);
  bool hit_x=move_x((int)(vx+(vx>=0?0.5f:-0.5f)));
  bool hit_y=move_y((int)(vy+(vy>=0?0.5f:-0.5f)));
  if(hit_x||hit_y){dash=0;rem_x=rem_y=0;}
 }else{
  const float maxrun=1.75f;
  float accel=on_ground?0.525f:0.35f;
  const float deccel=0.13125f;
  if(fabsf(vx)>maxrun)vx=approach(vx,(vx<0?-maxrun:maxrun),deccel);
  else vx=approach(vx,dir*maxrun,accel);
  if(vx< -0.05f)facing=-1;else if(vx>0.05f)facing=1;

  float maxfall=3.5f;
  float gravity=0.18375f;
  if(fabsf(vy)<=0.2625f)gravity*=0.5f;
  if(wall&&dir==wall&&!on_ground)maxfall=0.70f;
  if(grab){vx=0;if(vy>0.35f)vy=0.35f;}
  if(!on_ground)vy=approach(vy,maxfall,gravity);
  else if(vy>0)vy=0;

  if(jump_buffer>0){
   if(coyote>0){
    jump_buffer=0;coyote=0;vy=-3.5f;ground=0;
    burst(px+PW/2,py+PH,4,0);
   }else if(wall){
    jump_buffer=0;vy=-3.5f;vx=-wall*3.5f;facing=-wall;grab=0;
    rem_x=0;burst(px+PW/2,py+PH/2,5,0);
   }
  }

  move_player_x();
  ground=0;
  move_player_y();

  if(dash_pressed&&!dash_used&&!grab){
   if(!aimx&&!aimy)aimx=facing;
   const float full=8.75f,half=6.187184f;
   if(aimx&&aimy){vx=aimx*half;vy=aimy*half;}
   else if(aimx){vx=aimx*full;vy=0;}
   else {vx=0;vy=aimy*full;}
   dash_used=1;dash=8;jump_buffer=0;rem_x=rem_y=0;
   dash_target_x=(vx>0?3.5f:(vx<0?-3.5f:0));
   dash_target_y=(vy>0?3.5f:(vy<0?-3.5f:0));
   dash_accel_x=1.3125f;dash_accel_y=1.3125f;
   if(vy<0)dash_target_y*=0.75f;
   if(vy!=0)dash_accel_x*=0.70710678f;
   if(vx!=0)dash_accel_y*=0.70710678f;
   shake=2;burst(px+PW/2,py+PH/2,8,1);
  }
 }

 ground=grounded_now();
 if(ground){if(!on_ground)dash_used=0;if(vy>0)vy=0;}
 wall=ground?0:touching_wall();
 px=clampi(px,0,RW-PW);
 if(py>H+20||py<-30){die();return;}
 for(int i=0;i<5;i++)if(overlap(px,py,PW,PH,spikes[room][i])){die();return;}
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
 for(int i=0;i<MAX_PART;i++)if(particles[i].life){particles[i].x+=(int)iround(particles[i].vx);particles[i].y+=(int)iround(particles[i].vy);particles[i].vy+=0.12f;particles[i].life--;}
 if(shake)shake--;
 if(death_flash)death_flash--;
}
static int screen_x(int x){return x-(cam-room*RW);}
static void draw_spikes(Box b){
 int x0=screen_x(b.x),n=b.w/8;
 for(int i=0;i<n;i++){int x=x0+i*8;fillc(x+3,b.y+1,2,2,0xF6A2A8);fillc(x+2,b.y+3,4,2,0xF05B68);fillc(x+1,b.y+5,6,3,0xD83E5C);}
}
static void draw_city(void){
 /* distant skyline, window lights and drifting clouds */
 for(int i=0;i<9;i++){
  int x=(i*91-(cam/4)%91)-12,h=70+(i*37%75),y=211-h;
  fillc(x,y,45,h,0x1A2742);fillc(x+5,y+7,35,3,0x34435E);
  for(int wy=y+17;wy<205;wy+=17)for(int wx=x+9;wx<x+40;wx+=13)
   if(((wx+wy+i*3)/13)%3)fillc(wx,wy,4,5,0x4B6279);
 }
 for(int i=0;i<5;i++){int x=(i*103-(cam/2)%103+W)%W;roundc(x,54+i%3*19,32,3,2,0x42516B,100);}
}
static void draw_room(void){
 gfx_vgrad(0,0,W,H,0x0B142B,0x263552);draw_city();
 /* moon */
 circlec(263-(cam/8)%30,52,15,0xE3D9C2,180);circlec(269-(cam/8)%30,46,15,0x17233D,220);
 /* Layered ruined skyline, lit windows and foreground silhouettes. */
 fillc(0,210,W,30,0x101A30);
 for(int i=0;i<12;i++){
  int x=(i*31-(cam/2)%31),top=158+(i*19%34);
  fillc(x,top,18,58,0x202D47);fillc(x+2,top-3,14,3,0x3A4A65);
  for(int wy=top+9;wy<207;wy+=12)for(int wx=x+4;wx<x+16;wx+=7)
   if(((i+wy+wx)/5)%3)fillc(wx,wy,2,3,0x71849A);
 }
 /* distant stars */
 for(int i=0;i<22;i++){int x=(i*47-(cam/5)%47+W)%W;int y=31+(i*29%116);fillc(x,y,1,1,(i%4)?0x8195B1:0xD7D8CB);}
 /* Hand-shaded stone platforms: bright chipped rim, dark sides and pixel seams. */
 for(int i=0;i<12;i++){Box b=platforms[room][i];if(b.w<2)continue;int x=screen_x(b.x);
  fillc(x,b.y,b.w,b.h,0x35435B);
  fillc(x,b.y,b.w,2,0xB9C5D4);
  fillc(x+1,b.y+2,b.w-2,2,0x8295AA);
  if(b.h>12){
   fillc(x+2,b.y+5,b.w-4,2,0x4B5D75);
   for(int k=7;k<b.w-4;k+=13){
    fillc(x+k,b.y+9,1,4,0x26344C);
    if(k+5<b.w)fillc(x+k+4,b.y+6,2,2,0x657991);
   }
   fillc(x,b.y+b.h-3,b.w,3,0x202C43);
  }else{
   for(int k=12;k<b.w-4;k+=18){fillc(x+k,b.y+4,1,4,0x26344C);fillc(x+k+2,b.y+5,4,1,0x71849B);}
  }
  fillc(x+3,b.y+1,5,1,0xE2EAF1);
 }
 for(int i=0;i<5;i++)draw_spikes(spikes[room][i]);
 /* crystal dash refills */
 for(int i=0;i<2;i++)if(!orb_taken[room][i]){
  int x=screen_x(orbs[room][i].x),y=orbs[room][i].y;
  circlec(x,y,10,0x48D9D1,36);roundc(x-4,y-7,8,14,3,0x75F1E4,245);
  fillc(x-2,y-5,2,4,0xE0FFFF);
 }
 /* exit beacon */
 fillc(screen_x(RW-24),150,3,72,0x58E5D4);circlec(screen_x(RW-22),148,5,0xD4FFF7,240);
 /* 16x16 hand-drawn pixel sprite, with a second running-leg frame. */
 static const char *hero[16]={
  "......HHHH......",".....HHHHHH.....","....HHHHHHH.....",
  "....HSSSSSH.....","....HSSSSEH.....",".....SSSSSS.....",
  "......SSS.......",".....CCCCCC.....","....CCCCCCCC....",
  "...PPCCCCCCC....","..PPPCCCCCC.....","...PPCCCC.......",
  "....CC..CC......","....DD..DD......","...DDD..DDD.....",
  "................"
 };
 static const char *run[16]={
  "......HHHH......",".....HHHHHH.....","....HHHHHHH.....",
  "....HSSSSSH.....","....HSSSSEH.....",".....SSSSSS.....",
  "......SSS.......",".....CCCCCC.....","....CCCCCCCC....",
  "...PPCCCCCCC....","..PPPCCCCCC.....","...PPCCCC.......",
  "...CC...CC......","..DDD...DDD.....","..DD.....DD.....",
  "................"
 };
 const char **spr=((frames/7)&1)&&ground?run:hero;
 int x=screen_x(px)-3,y=py-1;
 if(dash){roundc(x-(int)(vx*1.5f),y-(int)(vy*1.5f)+3,17,12,6,0x70E9DF,80);}
 for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++){
  char q=spr[yy][xx];uint32_t col=0;
  if(q=='H')col=0x542A43;else if(q=='S')col=0xE9B99A;
  else if(q=='E')col=0x18243B;else if(q=='C')col=0xD94E6B;
  else if(q=='P')col=0x70E9DF;else if(q=='D')col=0x26334D;
  if(col)fillc(x+xx,y+yy,1,1,col);
 }
 if(dash){for(int i=0;i<5;i++)fillc(x-3-i*3,y+6+(i&1)*2,3,2,0x70E9DF);}
 /* particles */
 for(int i=0;i<MAX_PART;i++)if(particles[i].life){Particle p=particles[i];int a=p.life*16;
  circlec(screen_x(p.x),p.y,p.kind?2:1,p.kind?0x70E9DF:0xF4B8C3,a);
 }
 /* HUD */
 fillc(0,0,W,25,0x091124);fillc(0,24,W,1,0x3B4C68);
 textc(&np_font_small,7,16,"VILLE ABANDONNEE",0xFFFF,245);
 rightc(&np_font_small,312,16,room_names[room],0xFFD875,255);
 textc(&np_font_small,8,40,"DASH",0xD8E6F4,230);
 roundc(43,30,13,8,3,dash_used?0x536078:0x70E9DF,255);
 rightc(&np_font_small,311,40,"TOOLBOX: ACCROCHER",0xAFC2D8,210);
 textc(&np_font_small,8,232,"MORTS",0xAFC2D8,210);
 char n[8];int v=deaths,k=0;do{n[k++]=(char)('0'+v%10);v/=10;}while(v&&k<6);for(int i=0;i<k/2;i++){char t=n[i];n[i]=n[k-1-i];n[k-1-i]=t;}n[k]=0;
 textc(&np_font_small,48,232,n,0xFFFFFF,255);
 if(state!=PLAY){
  fillac(0,0,W,H,0x070D1B,185);gfx_shadow(39,55,242,132,13,8,130);roundc(39,55,242,132,13,0x263653,255);
  outlinec(39,55,242,132,13,1,0x7187A7,170);
  if(state==MENU){
   centerc(&np_font_title,160,87,"D-SIDE 1",0xFFD875,255);
   centerc(&np_font_body,160,111,"VILLE ABANDONNEE",0xFFFFFF,255);
   paragraphc(&np_font_small,160,137,195,13,"Un nouveau defi. Cinq quartiers. Un seul dash.",0xC5D5E8,255);
   centerc(&np_font_body,160,174,"OK  POUR JOUER",0x70E9DF,255);
  }else if(state==HELP){
   centerc(&np_font_title,160,83,"COMMANDES",0xFFD875,255);
   centerc(&np_font_small,160,111,"Fleches : courir et viser",0xFFFFFF,255);
   centerc(&np_font_small,160,128,"OK : sauter   BACK : dash",0xFFFFFF,255);
   centerc(&np_font_small,160,145,"TOOLBOX : s'accrocher au mur",0xFFFFFF,255);
   centerc(&np_font_small,160,174,"OK OU BACK POUR RETOUR",0x70E9DF,255);
  }else if(state==PAUSE){
   centerc(&np_font_title,160,92,"PAUSE",0xFFD875,255);
   centerc(&np_font_body,160,121,"OK : recommencer la salle",0xFFFFFF,255);
   centerc(&np_font_body,160,145,"BACK : menu principal",0xFFFFFF,255);
   centerc(&np_font_small,160,174,"BACKSPACE : reprendre",0x70E9DF,255);
  }else{
   centerc(&np_font_title,160,91,"CHAPITRE TERMINE",0xFFD875,255);
   centerc(&np_font_body,160,120,"Tu as traverse la ville !",0xFFFFFF,255);
   centerc(&np_font_small,160,148,"OK POUR RECOMMENCER",0x70E9DF,255);
   centerc(&np_font_small,160,169,"MORTS : regarde si tu peux faire mieux",0xC5D5E8,255);
  }
 }
 if(death_flash)fillac(0,0,W,H,0xF05B68,death_flash*9);
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
