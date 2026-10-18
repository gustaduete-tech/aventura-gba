#include <stdint.h>
#include <stdbool.h>
#include "title_data.h"

#define REG_DISPCNT (*(volatile uint16_t*)0x04000000)
#define REG_KEYINPUT (*(volatile uint16_t*)0x04000130)
#define MODE3 3
#define BG2 (1<<10)
#define KEY_A      (1<<0)
#define KEY_B      (1<<1)
#define KEY_SELECT (1<<2)
#define KEY_START  (1<<3)
#define KEY_RIGHT  (1<<4)
#define KEY_LEFT   (1<<5)
#define KEY_UP     (1<<6)
#define KEY_DOWN   (1<<7)
#define KEY_R      (1<<8)
#define KEY_L      (1<<9)
#define SCREEN_W 240
#define SCREEN_H 160
#define TILE 16
#define MAP_W 64
#define MAP_H 64
#define FB ((volatile uint16_t*)0x06000000)

static inline uint16_t rgb(int r,int g,int b){return (uint16_t)((r>>3)|((g>>3)<<5)|((b>>3)<<10));}
static inline void pixel(int x,int y,uint16_t c){if((unsigned)x<SCREEN_W&&(unsigned)y<SCREEN_H) FB[y*SCREEN_W+x]=c;}
static void rect(int x,int y,int w,int h,uint16_t c){int yy,xx; for(yy=y;yy<y+h;yy++) for(xx=x;xx<x+w;xx++) pixel(xx,yy,c);}
static void wait_vblank(void){while(*(volatile uint16_t*)0x04000006>=160){} while(*(volatile uint16_t*)0x04000006<160){}}
static uint16_t keys(void){return (uint16_t)(~REG_KEYINPUT)&0x03ff;}
static uint32_t rng=0xC0FFEEu;
static uint32_t rand32(void){rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng;}
static uint8_t world[MAP_H][MAP_W];
static uint8_t visited[MAP_H][MAP_W];
enum {GRASS, DARKGRASS, WATER, SAND, TREE, ROCK, PATH, FLOWER, CRYSTAL};
static int px=32*TILE, py=32*TILE, facing=0, camera_x, camera_y;
static int hearts=3, crystals=0, swing=0, message_timer=0;
static bool started=false, win=false;
static char message[48]="Explore o mundo e encontre 3 cristais!";

static void set_message(const char *s){int i=0; while(s[i]&&i<46){message[i]=s[i];i++;}message[i]=0; message_timer=180;}
static void make_world(void){
    int x,y; rng=0x5A17BEEFu;
    for(y=0;y<MAP_H;y++)for(x=0;x<MAP_W;x++){
        uint32_t n=rand32(); uint8_t t=GRASS;
        if((x<3||y<3||x>MAP_W-4||y>MAP_H-4) && (n%5==0)) t=WATER;
        else if(n%31==0)t=TREE; else if(n%53==0)t=ROCK; else if(n%17==0)t=DARKGRASS; else if(n%23==0)t=FLOWER;
        world[y][x]=t;
    }
    // winding river on the east; a few safe crossings
    for(y=4;y<60;y++){int rx=45+(y/9%2); for(x=rx-1;x<=rx+1;x++) world[y][x]=WATER;}
    for(x=42;x<50;x++)world[18][x]=PATH;
    for(x=42;x<50;x++)world[42][x]=PATH;
    // starting clearing and path
    for(y=29;y<=35;y++)for(x=29;x<=35;x++)world[y][x]=GRASS;
    for(x=20;x<40;x++)world[32][x]=PATH;
    // landmarks and collectible crystals
    world[12][12]=CRYSTAL; world[50][15]=CRYSTAL; world[20][53]=CRYSTAL;
    world[31][34]=GRASS; world[32][34]=GRASS; world[33][34]=GRASS;
}
static bool solid(int tx,int ty){if(tx<0||ty<0||tx>=MAP_W||ty>=MAP_H)return true; uint8_t t=world[ty][tx]; return t==WATER||t==TREE||t==ROCK;}
static void draw_tile(int sx,int sy,uint8_t t,int wx,int wy){
    uint16_t grass=rgb(30,175,94), dg=rgb(17,133,83), water=rgb(45,158,210), sand=rgb(216,195,105), tree=rgb(10,83,65), rock=rgb(100,125,135), path=rgb(174,150,91), flower=rgb(40,180,98), crystal=rgb(90,240,235);
    uint16_t c=grass;
    switch(t){case DARKGRASS:c=dg;break;case WATER:c=water;break;case SAND:c=sand;break;case TREE:c=dg;break;case ROCK:c=grass;break;case PATH:c=path;break;case FLOWER:c=grass;break;case CRYSTAL:c=grass;break;}
    rect(sx,sy,TILE,TILE,c);
    if(t==GRASS||t==DARKGRASS){pixel(sx+3+(wx*7+wy)%9,sy+4+(wy*3+wx)%8,t==GRASS?rgb(50,199,103):rgb(25,151,92));}
    if(t==WATER){rect(sx+2,sy+4,6,1,rgb(105,214,226));rect(sx+8,sy+11,5,1,rgb(105,214,226));}
    if(t==TREE){rect(sx+6,sy+9,4,7,rgb(104,73,47));rect(sx+2,sy+2,12,9,tree);rect(sx+4,sy,8,9,rgb(12,104,68));pixel(sx+5,sy+3,rgb(24,145,85));}
    if(t==ROCK){rect(sx+3,sy+7,10,7,rock);rect(sx+5,sy+4,6,5,rgb(135,157,160));pixel(sx+5,sy+7,rgb(185,199,192));}
    if(t==PATH){pixel(sx+4,sy+4,rgb(198,174,116));pixel(sx+11,sy+12,rgb(198,174,116));}
    if(t==FLOWER){rect(sx+7,sy+7,1,5,rgb(18,110,59));pixel(sx+7,sy+6,rgb(255,100,175));pixel(sx+6,sy+6,rgb(255,210,230));}
    if(t==CRYSTAL){pixel(sx+8,sy+3,crystal);rect(sx+6,sy+5,5,6,crystal);pixel(sx+7,sy+11,rgb(31,132,214));pixel(sx+8,sy+6,rgb(235,255,255));}
}
static void draw_player(int sx,int sy){
    // tiny 12x14 adventurer with direction-facing sword
    rect(sx+3,sy+2,8,5,rgb(238,194,140)); rect(sx+2,sy+1,10,3,rgb(27,118,67));
    pixel(sx+5,sy+5,rgb(20,35,45)); pixel(sx+9,sy+5,rgb(20,35,45));
    rect(sx+3,sy+7,8,5,rgb(39,163,102)); rect(sx+3,sy+12,3,3,rgb(70,67,111)); rect(sx+8,sy+12,3,3,rgb(70,67,111));
    if(facing==0){rect(sx+11,sy+8,5,2,rgb(210,220,227));pixel(sx+15,sy+7,rgb(245,250,255));}
    if(facing==1){rect(sx-4,sy+8,5,2,rgb(210,220,227));pixel(sx-4,sy+7,rgb(245,250,255));}
    if(facing==2){rect(sx+8,sy-3,2,5,rgb(210,220,227));pixel(sx+7,sy-3,rgb(245,250,255));}
    if(facing==3){rect(sx+8,sy+14,2,5,rgb(210,220,227));pixel(sx+7,sy+18,rgb(245,250,255));}
    if(swing>0){if(facing==0)rect(sx+13,sy+4,5,7,rgb(245,245,200));if(facing==1)rect(sx-6,sy+4,5,7,rgb(245,245,200));if(facing==2)rect(sx+4,sy-6,7,4,rgb(245,245,200));if(facing==3)rect(sx+4,sy+15,7,4,rgb(245,245,200));}
}
static void draw_text(int x,int y,const char *s,uint16_t c){
    // compact 3x5 pixel font, enough for HUD and prompts
    static const uint8_t glyph[36][5]={
      {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},
      {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
    while(*s){char ch=*s++; if(ch==' '){x+=4;continue;} if(ch>='a'&&ch<='z')ch-=32; int idx=-1;if(ch>='0'&&ch<='9')idx=ch-'0';else if(ch>='A'&&ch<='Z')idx=10+ch-'A'; if(idx>=0){for(int yy=0;yy<5;yy++)for(int xx=0;xx<3;xx++)if(glyph[idx][yy]&(1<<(2-xx)))pixel(x+xx,y+yy,c);}x+=4;}
}
static void draw_hud(void){rect(0,0,240,13,rgb(9,42,65));draw_text(4,4,"HP",rgb(255,220,130));for(int i=0;i<hearts;i++){rect(17+i*8,3,5,5,rgb(245,70,100));pixel(18+i*8,2,rgb(245,70,100));pixel(20+i*8,2,rgb(245,70,100));}draw_text(60,4,"CRYSTALS",rgb(155,245,235));draw_text(105,4,crystals==0?"0":crystals==1?"1":crystals==2?"2":"3",rgb(255,255,190));draw_text(139,4,"A SWING",rgb(220,235,245));}
static void render(void){
    int tx0=camera_x/TILE, ty0=camera_y/TILE;
    for(int sy=0;sy<11;sy++)for(int sx=0;sx<16;sx++){int tx=tx0+sx,ty=ty0+sy;int dx=sx*TILE-(camera_x%TILE),dy=sy*TILE-(camera_y%TILE);if(tx>=0&&ty>=0&&tx<MAP_W&&ty<MAP_H)draw_tile(dx,dy,world[ty][tx],tx,ty);else rect(dx,dy,TILE,TILE,rgb(9,65,88));}
    int psx=px-camera_x,psy=py-camera_y; draw_player(psx,psy);
    draw_hud();
    if(message_timer>0){rect(7,137,226,17,rgb(8,39,58));rect(7,137,226,1,rgb(104,226,210));draw_text(12,143,message,rgb(232,247,220));}
    if(win){rect(36,60,168,37,rgb(8,39,58));draw_text(56,68,"ALL CRYSTALS FOUND",rgb(255,233,125));draw_text(64,80,"WORLD IS YOURS",rgb(160,245,230));}
}
static void title(void){for(int i=0;i<240*160;i++)FB[i]=title_bitmap[i];draw_text(62,145,"PRESS START",rgb(255,255,180));}
int main(void){REG_DISPCNT=MODE3|BG2; make_world(); title();
    while(1){wait_vblank();uint16_t k=keys();if(!started){if(k&KEY_START){started=true;set_message("Encontre os 3 cristais antigos!");}else{title();}continue;}
        int nx=px,ny=py; bool moved=false;
        if(k&KEY_LEFT){nx-=2;facing=1;moved=true;}else if(k&KEY_RIGHT){nx+=2;facing=0;moved=true;}else if(k&KEY_UP){ny-=2;facing=2;moved=true;}else if(k&KEY_DOWN){ny+=2;facing=3;moved=true;}
        if(moved){int cx=(nx+5)/TILE, cy=(ny+8)/TILE;if(!solid(cx,cy)&&!solid((nx+10)/TILE,cy)&&!solid(cx,(ny+13)/TILE)&&!solid((nx+10)/TILE,(ny+13)/TILE)){px=nx;py=ny;}}
        if(k&KEY_A){swing=5;int tx=(px+5)/TILE,ty=(py+8)/TILE;if(facing==0)tx++;if(facing==1)tx--;if(facing==2)ty--;if(facing==3)ty++;if(tx>=0&&ty>=0&&tx<MAP_W&&ty<MAP_H){if(world[ty][tx]==CRYSTAL){world[ty][tx]=GRASS;crystals++;set_message(crystals>=3?"Voce reuniu os 3 cristais!":"Cristal antigo coletado!");if(crystals>=3)win=true;}else if(world[ty][tx]==TREE||world[ty][tx]==ROCK){world[ty][tx]=GRASS;set_message("Voce abriu caminho!");}}}
        if(swing>0)swing--; if(message_timer>0)message_timer--;
        // camera follows the hero while respecting map boundaries
        camera_x=px-112;camera_y=py-72;if(camera_x<0)camera_x=0;if(camera_y<0)camera_y=0;if(camera_x>MAP_W*TILE-SCREEN_W)camera_x=MAP_W*TILE-SCREEN_W;if(camera_y>MAP_H*TILE-SCREEN_H)camera_y=MAP_H*TILE-SCREEN_H;
        render();
    }return 0;
}
