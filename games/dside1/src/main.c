/* D-Side 1: Ville abandonnee.
 * Original, self-contained platforming challenge for NumWorks N0120.
 * Controls: arrows move/aim, OK jumps, Back dashes, Back while paused resumes. */
#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../common/epsilon_app.h"
#include "../../../launcher/src/gfx.h"
#include "../../common/np_text.h"

#ifndef NP_MODULE
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "D-Side 1";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif

/* Standalone app: send each rendered strip to the EADK display. */
void np_push(int x, int y, int w, int h, const color_t *px) {
  eadk_display_push_rect((eadk_rect_t){(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h}, px);
}

#define KEY(k) (1ull << (k))
#define SKY 0x101A35
#define NIGHT 0x202B4A
#define STONE 0x687184
#define STONE2 0x424D62
#define RED 0xF05B68
#define WHITE 0xFFFF
#define CYAN 0x70E0D0
#define GOLD 0xFFD45A
#define TICK_MS 20
#define PW 10
#define PH 14

typedef struct { int x,y,w,h; } Rect;
typedef struct { const Rect *platforms; int np; const Rect *spikes; int ns; int spawn_y; } Room;

/* Five original rooms: deliberately compact and readable on a 320x240 screen. */
static const Rect p0[]={{0,222,100,18},{112,178,42,8},{210,222,110,18}};
static const Rect s0[]={{100,214,110,8}};
static const Rect p1[]={{0,222,320,18},{54,184,48,8},{139,151,48,8},{226,184,54,8}};
static const Rect s1[]={{104,214,116,8}};
static const Rect p2[]={{0,222,72,18},{96,190,42,8},{166,157,42,8},{238,124,82,8}};
static const Rect s2[]={{72,214,24,8},{138,214,28,8},{208,214,30,8}};
static const Rect p3[]={{0,222,70,18},{105,184,48,8},{177,151,48,8},{252,222,68,18}};
static const Rect s3[]={{70,214,35,8},{153,214,99,8}};
static const Rect p4[]={{0,222,82,18},{108,188,44,8},{178,153,44,8},{246,222,74,18}};
static const Rect s4[]={{82,214,26,8},{152,214,26,8},{222,214,24,8}};
static const Room rooms[]={
 {p0,3,s0,1,196},{p1,4,s1,1,196},{p2,4,s2,3,196},{p3,4,s3,2,196},{p4,4,s4,3,196}
};
static color_t strip[SCREEN_W*STRIP_H];
static int room=0, px=12, py=196, vx=0, vy=0, dash_ticks=0, dash_used=0;
static int grounded=0, state=0, deaths=0, ticks=0;
static uint64_t previous=0;

static bool hit(int x,int y,int w,int h,Rect r) {
 return x<r.x+r.w && x+w>r.x && y<r.y+r.h && y+h>r.y;
}
static void reset_room(void) {
 px=12; py=rooms[room].spawn_y; vx=vy=dash_ticks=dash_used=grounded=0;
}
static void restart(void) { room=0; deaths=0; ticks=0; reset_room(); state=1; }
static void die(void) { deaths++; reset_room(); }
static bool on_floor(void) {
 const Room *r=&rooms[room];
 for(int i=0;i<r->np;i++) if(px+PW>r->platforms[i].x && px<r->platforms[i].x+r->platforms[i].w &&
   py+PH<=r->platforms[i].y+3 && py+PH>=r->platforms[i].y-2) return true;
 return false;
}
static void step(uint64_t keys,uint64_t pressed) {
 const Room *r=&rooms[room];
 int dx=(keys&KEY(eadk_key_right)?1:0)-(keys&KEY(eadk_key_left)?1:0);
 if(dash_ticks>0) {
   px+=vx; py+=vy; dash_ticks--;
 } else {
   vx=dx*2;
   px+=vx;
   if(px<0)px=0;
   if(px>SCREEN_W-PW)px=SCREEN_W-PW;
   if(pressed&KEY(eadk_key_ok) && grounded) {vy=-9;grounded=0;}
   if((pressed&KEY(eadk_key_back)) && !dash_used) {
     int ax=(keys&KEY(eadk_key_right)?1:0)-(keys&KEY(eadk_key_left)?1:0);
     int ay=(keys&KEY(eadk_key_down)?1:0)-(keys&KEY(eadk_key_up)?1:0);
     if(!ax&&!ay)ax=1;
     vx=ax*7; vy=ay*7;
     if(ax&&ay){vx=vx*181/256;vy=vy*181/256;}
     dash_ticks=8;dash_used=1;
   } else {
     if(vy<7)vy++;
     int old=py; py+=vy; grounded=0;
     if(vy>=0) for(int i=0;i<r->np;i++) {
       Rect q=r->platforms[i];
       if(px+PW>q.x && px<q.x+q.w && old+PH<=q.y && py+PH>=q.y) {
         py=q.y-PH;vy=0;grounded=1;dash_used=0;break;
       }
     }
   }
 }
 if(px<0)px=0;
 if(px>SCREEN_W-PW)px=SCREEN_W-PW;
 if(py>SCREEN_H+10){die();return;}
 for(int i=0;i<r->ns;i++) if(hit(px,py,PW,PH,r->spikes[i])){die();return;}
 /* A room ends at its right edge; each transition starts a new original room. */
 if(px>=SCREEN_W-PW-2) {
   if(room==4){state=3;return;}
   room++;reset_room();
 }
 if(py<0)py=0;
}
static void spike_draw(Rect r) {
 int n=r.w/8;
 for(int i=0;i<n;i++) {
   int x=r.x+i*8;
   for(int h=0;h<8;h++) gfx_fill(x+h/2,r.y+7-h,8-h,1,RED);
 }
}
static void scene(void *ctx) {
 (void)ctx;
 const Room *r=&rooms[room];
 gfx_vgrad(0,0,SCREEN_W,SCREEN_H,0x101A35,0x26324F);
 /* distant silhouettes */
 for(int i=0;i<8;i++) {
   int x=i*48-12;
   gfx_fill(x,78+(i%3)*13,26,144,0x202B43);
   gfx_fill(x+5,91+(i%3)*13,3,7,0x46516A);
 }
 gfx_fill(0,0,SCREEN_W,23,0x0B1227);
 gfx_text(&np_font_small,7,15,"VILLE ABANDONNEE",WHITE,256);
 char label[20]="";
 label[0]='0'+room+1;label[1]='/';label[2]='5';label[3]=0;
 gfx_text_right(&np_font_small,312,15,label,GOLD,256);
 for(int i=0;i<r->np;i++) {
   Rect q=r->platforms[i];
   gfx_fill(q.x,q.y,q.w,q.h,STONE);
   gfx_fill(q.x,q.y,q.w,2,0xB5C0D0);
   gfx_fill(q.x,q.y+q.h-2,q.w,2,STONE2);
 }
 for(int i=0;i<r->ns;i++) spike_draw(r->spikes[i]);
 /* exit marker */
 gfx_fill(300,178,4,44,CYAN);gfx_fill(294,178,16,3,WHITE);
 /* player and dash trail */
 if(dash_ticks) gfx_rrect(px-vx,py-vy+3,8,8,3,CYAN,120);
 gfx_rrect(px,py,PW,PH,3,RED,256);
 gfx_fill(px+2,py+3,2,2,WHITE);
 /* dash availability */
 gfx_text(&np_font_small,8,39,"DASH",WHITE,220);
 gfx_rrect(42,29,13,7,3,dash_used?STONE2:CYAN,256);
 gfx_text_right(&np_font_small,312,39,"OK: SAUT   BACK: DASH",WHITE,220);
 if(state!=1) {
   gfx_fill_alpha(0,0,SCREEN_W,SCREEN_H,0x080D1B,170);
   gfx_rrect(43,66,234,112,12,NIGHT,256);
   if(state==0) {
     gfx_text_center(&np_font_title,160,94,"D-SIDE 1",GOLD,256);
     gfx_text_center(&np_font_body,160,119,"VILLE ABANDONNEE",WHITE,256);
     gfx_paragraph(&np_font_small,160,143,190,13,"Arrows: move/aim   OK: jump   Back: dash",WHITE,256);
     gfx_text_center(&np_font_body,160,168,"OK POUR COMMENCER",CYAN,256);
   } else if(state==2) {
     gfx_text_center(&np_font_title,160,105,"PAUSE",GOLD,256);
     gfx_text_center(&np_font_body,160,137,"OK: reprendre   Back: menu",WHITE,256);
   } else {
     gfx_text_center(&np_font_title,160,105,"CHAPITRE TERMINE",GOLD,256);
     gfx_text_center(&np_font_body,160,133,"Tu as traverse les 5 salles !",WHITE,256);
     gfx_text_center(&np_font_body,160,160,"OK: recommencer",CYAN,256);
   }
 }
}
int main(void) {
 np_app_begin();
 color_t *old=gfx_buf;gfx_buf=strip;
 restart();state=0;
 uint32_t next=(uint32_t)eadk_timing_millis();
 for(;;) {
   uint64_t keys=eadk_keyboard_scan(),pressed=keys&~previous;previous=keys;
   if(keys&(KEY(eadk_key_home)|KEY(eadk_key_on_off)))break;
   if(state==0 && (pressed&KEY(eadk_key_ok))) {restart();}
   else if(state==1 && (pressed&KEY(eadk_key_backspace)))state=2;
   else if(state==2 && (pressed&(KEY(eadk_key_ok)|KEY(eadk_key_back))))state=1;
   else if(state==3 && (pressed&KEY(eadk_key_ok)))restart();
   uint32_t now=(uint32_t)eadk_timing_millis();
   for(int n=0;n<4 && (int32_t)(now-next)>=0;n++,next+=TICK_MS)
     if(state==1)step(keys,pressed);
   if((int32_t)(now-next)>4*TICK_MS)next=now;
   gfx_render(scene,0,0,SCREEN_H);
   eadk_timing_msleep(1);
 }
 gfx_buf=old;
 return np_app_end();
}
