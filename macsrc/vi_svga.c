#include <stdio.h>
#include <stdlib.h>

#include <vga.h>
#include <vgakeyboard.h>

#include "wolfdef.h"

Byte *gfxbuf;

void keyboard_handler(int key, int press);

void FixMapList(maplist_t *m)
{
	int i;
	
	m->MaxMap = sMSB(m->MaxMap);
	m->MapRezNum = sMSB(m->MapRezNum);
	
	for (i = 0; i < m->MaxMap; i++) {
		m->InfoArray[i].NextLevel = sMSB(m->InfoArray[i].NextLevel);
		m->InfoArray[i].SecretLevel = sMSB(m->InfoArray[i].SecretLevel);
		m->InfoArray[i].ParTime = sMSB(m->InfoArray[i].ParTime);
		m->InfoArray[i].ScenarioNum = sMSB(m->InfoArray[i].ScenarioNum);
		m->InfoArray[i].FloorNum = sMSB(m->InfoArray[i].FloorNum);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <mac wolf3d resource fork>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	vga_init();
	 
	if (InitResources(argv[1])) {
		fprintf(stderr, "could not load %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
/*
InitSoundMusicSystem(8,8,5, 11025);
SoundListPtr = (Word *) LoadAResource(MySoundList);	
RegisterSounds(SoundListPtr,FALSE);
ReleaseAResource(MySoundList);
*/

	GetTableMemory();
	
	MapListPtr = (maplist_t *) LoadAResource(rMapList);
	FixMapList(MapListPtr);
	
	SongListPtr = (unsigned short *) LoadAResource(rSongList);
	WallListPtr = (unsigned short *) LoadAResource(MyWallList);
	
	NewGameWindow(0); /* 320x200 */
//#ifndef NOVGA
	keyboard_init(); /* keyboard must be init'd after vga_setmode .. */
	keyboard_seteventhandler(keyboard_handler);
//#endif	
	ClearTheScreen(BLACK);
	BlastScreen();
	
	return WolfMain(argc, argv);
}

void Quit(char *str)
{
	keyboard_close();
	vga_setmode(TEXT);
	
	if (str && *str) {
		fprintf(stderr, "%s\n", str);
		exit(EXIT_FAILURE);
	}
	
	exit(EXIT_SUCCESS);
}

void SetPalette(Byte *pal)
{
	int i;

	vga_waitretrace();
#ifndef NOVGA	
	for (i = 0; i < 256; i++) 
		vga_setpalette(i, pal[i*3+0] >> 2, pal[i*3+1] >> 2, pal[i*3+2] >> 2);	
#endif
}
	
void BlastScreen2(Rect *BlastRect)
{
	BlastScreen();
}

int VidWidth, VidHeight, ViewHeight;
#define w VidWidth
#define h VidHeight
#define v ViewHeight

void BlastScreen()
{
	Byte *ptrs = gfxbuf, *ptrd = graph_mem;
 	int i;

#ifndef NOVGA	
	for (i = 0; i < 200; i++) {
		memcpy(ptrd, ptrs, 320);
		ptrs += w;
		ptrd += 320;
	}
#endif
}

Word VidXs[] = {320,512,640,640};       /* Screen sizes to play with */
Word VidYs[] = {200,384,400,480};
Word VidVs[] = {160,320,320,400};
Word VidPics[] = {rFaceShapes,rFace512,rFace640,rFace640};
Word VidSize = -1;

Word ScaleX(Word x) 
{
	switch(VidSize) {
		case 1:
			return x*8/5;
		case 2:
		case 3:
			return x*2;
	}
	return x;
}

Word ScaleY(Word y)
{
	switch(VidSize) {
		case 1:
			y = (y*8/5)+64;
			if (y == 217)
				y++;
			return y;
		case 2:
			return y*2;
		case 3:
			return y*2+80;
	}
	return y;
}

Word NewGameWindow(Word NewVidSize)
{
	LongWord *LongPtr;
	Byte *DestPtr;
	int i;
	
	printf("Called: %d\n", NewVidSize);
	
	if (NewVidSize == VidSize)
		return VidSize;
	
	printf("Setting Size: %d (from %d)\n", NewVidSize, VidSize);
		
	if (NewVidSize < 4) {
		w = VidXs[NewVidSize];
		h = VidYs[NewVidSize];
		v = VidVs[NewVidSize];
	} else {
		fprintf(stderr, "Vid size: %d\n", NewVidSize);
		exit(EXIT_FAILURE);
	}
	
	if (gfxbuf)
		free(gfxbuf);
		
	gfxbuf = (Byte *)malloc(w * h);

#ifndef NOVGA	
	vga_setmode(G320x200x256);
#endif
	
	VideoPointer = gfxbuf;
	VideoWidth = w;
	InitYTable();
	SetAPalette(rBlackPal);
	ClearTheScreen(BLACK);
	BlastScreen();
	
	LongPtr = (LongWord *) LoadAResource(VidPics[NewVidSize]);
	
	if (GameShapes)
		FreeSomeMem(GameShapes);
		
	GameShapes = (Byte **) AllocSomeMem(lMSB(LongPtr[0]));
	DLZSS((Byte *)GameShapes,(Byte *) &LongPtr[1],lMSB(LongPtr[0]));
	ReleaseAResource(rFaceShapes);
	
	LongPtr = (LongWord *)GameShapes;
	DestPtr = (Byte *)GameShapes;
	for (i = 0; i < ((NewVidSize == 1) ? 57 : 47); i++) 
		GameShapes[i] = DestPtr + lMSB(LongPtr[i]);
		
	VidSize = NewVidSize;
	
	return VidSize;
}

LongWord ScaleDiv[2048];

void ScaledDraw(Byte *gfx, Word scale, Byte *vid, LongWord TheFrac, Word TheInt, Word Width, LongWord Delta)
{	
	LongWord OldDelta;
	while (scale--) {
		*vid = *gfx;
		vid += Width;
		OldDelta = Delta;
		Delta += TheFrac;
		gfx += TheInt;
		if (OldDelta > Delta)
			gfx += 1;			
	}
}

void IO_ScaleWallColumn(Word x, Word scale, Word tile, Word column)
{
	LongWord TheFrac;
	Word TheInt;
	LongWord y;
	
	Byte *ArtStart;
	
	if (scale) {
		scale*=2;
		TheFrac = 0x80000000UL / scale;

		ArtStart = &ArtData[tile][column<<7];
		if (scale<VIEWHEIGHT) {
			y = (VIEWHEIGHT-scale)/2;
			TheInt = TheFrac>>24;
			TheFrac <<= 8;
			
			ScaledDraw(ArtStart,scale,&VideoPointer[(y*VideoWidth)+x],
			TheFrac,TheInt,VideoWidth, 0);
			
			return;
			
		}
		y = (scale-VIEWHEIGHT)/2;
		y = y*TheFrac;
		TheInt = TheFrac>>24;
		TheFrac <<= 8;
		
		ScaledDraw(&ArtStart[y>>24],VIEWHEIGHT,&VideoPointer[x],
		TheFrac,TheInt,VideoWidth,y<<8);
	}
}

typedef struct {
	SWord Topy;
	SWord Boty;
	SWord Shape;
} PACKED SpriteRun;
                        
void IO_ScaleMaskedColumn(Word x,Word scale, unsigned short *CharPtr,Word column)
{
	Byte * CharPtr2;
	int Y1,Y2;
	Byte *Screenad;
	SpriteRun *RunPtr;
	LongWord TheFrac;
	LongWord TFrac;
	LongWord TInt;
	Word RunCount;
	int TopY;
	Word Index;
	LongWord Delta;
	
	if (!scale) 
		return;
		
	CharPtr2 = (Byte *) CharPtr;
	TheFrac = ScaleDiv[scale];
	RunPtr = (SpriteRun *) &CharPtr[sMSB(CharPtr[column+1])/2]; 
	Screenad = &VideoPointer[x];
	TFrac = TheFrac<<8;
	TInt = TheFrac>>24;
	TopY = (VIEWHEIGHT/2)-scale;
	
	while (RunPtr->Topy != 0xFFFF) {
		Y1 = scale*(LongWord)sMSB(RunPtr->Topy)/128+TopY;
		if (Y1<(int)VIEWHEIGHT) {
			Y2 = scale*(LongWord)sMSB(RunPtr->Boty)/128+TopY;
			if (Y2>0) {
				if (Y2>(int)VIEWHEIGHT) 
					Y2 = VIEWHEIGHT;
				Index = sMSB(RunPtr->Shape)+sMSB(RunPtr->Topy)/2;
				Delta = 0;
				if (Y1<0) {
					Delta = (0-(LongWord)Y1)*TheFrac;
					Index += (Delta>>24);
					Delta <<= 8;
					Y1 = 0;
				}
				RunCount = Y2-Y1;
				if (RunCount) 
					ScaledDraw(&CharPtr2[Index],RunCount,
					&Screenad[Y1*VideoWidth],TFrac,TInt,VideoWidth, Delta);
			}
		}
		RunPtr++;
	}
}

Boolean SetupScalers(void)
{
	Word i;
	if (!ScaleDiv[1]) { 
		i = 1;
		do {
			ScaleDiv[i] = 0x40000000/i;
		} while (++i<2048);
		MaxScaler = 2048;
	}
	return 1;
}

void ReleaseScalers()
{
}

/* Keyboard Hack */
static int RSJ;
static int keys[128];

void FlushKeys()
{
	while (keyboard_update()) ;
	
	memset(keys, 0, sizeof(keys));
}

#define SC(x) SCANCODE_##x

struct {
	int code[13];
	int flag;
} CheatCodes[] = {
{ { SC(X), SC(U), SC(S), SC(C), SC(N), SC(I), SC(E), SC(L), SC(P), SC(P), SC(A) }, 0 }, /* "XUSCNIELPPA" */
{ { SC(I), SC(D), SC(D), SC(Q), SC(D) }, 0 }, /* "IDDQD" */
{ { SC(B), SC(U), SC(R), SC(G), SC(E), SC(R) }, 0 }, /* "BURGER" */
{ { SC(W), SC(O), SC(W), SC(Z), SC(E), SC(R), SC(S) }, 0 }, /* "WOWZERS" */
{ { SC(L), SC(E), SC(D), SC(O), SC(U), SC(X) }, 0 }, /* "LEDOUX" */
{ { SC(S), SC(E), SC(G), SC(E), SC(R) }, 0 }, /* "SEGER" */
{ { SC(M), SC(C), SC(C), SC(A), SC(L), SC(L) }, 0 }, /* "MCCALL" */
{ { SC(A), SC(P), SC(P), SC(L), SC(E), SC(I), SC(I), SC(G), SC(S) } }, 0 /* "APPLEIIGS" */
};
const int CheatCount = sizeof(CheatCodes) / sizeof(CheatCodes[0]);
int CheatIndex;

void keyboard_handler(int key, int press)
{
	int i;
	
	if (key == SCANCODE_ESCAPE) 
		Quit(0);

	if (RSJ) {
		keys[key] = press;
		
		if (press == 0) {
			for (i = 0; i < CheatCount; i++) {
				if (CheatCodes[i].code[CheatIndex] == key) {
					CheatIndex++;
					if (CheatCodes[i].code[CheatIndex] == 0) {
						PlaySound(SND_BONUS);
						switch (i) {
						case 0:
						case 4:
							GiveKey(0);
							GiveKey(1);
							gamestate.godmode = TRUE;
							break;
						case 1:
							gamestate.godmode^=TRUE;
							break;
						case 2:
							gamestate.machinegun = TRUE;
							gamestate.chaingun = TRUE;
							gamestate.flamethrower = TRUE;
							gamestate.missile = TRUE;
							GiveAmmo(gamestate.maxammo);
							GiveGas(99);
							GiveMissile(99);
							break;
						case 3:
							gamestate.maxammo = 999;
							GiveAmmo(999);
							break;
						case 5:
							GiveKey(0);
							GiveKey(1);
							break;
						case 6:
							playstate=EX_WARPED;
							nextmap = gamestate.mapon+1;
							if (MapListPtr->MaxMap<=nextmap) 
								nextmap = 0;
							break;
						case 7:
							ShowPush ^= TRUE;
							break;
						}
						CheatIndex = 0;
					}
					break;
				} 
			}	
			if (i == CheatCount) 
				CheatIndex = 0;
		}
				
		joystick1 = 0;
		
		if (press == 0) {
			switch(key) {
			case SCANCODE_1:
				gamestate.pendingweapon = WP_KNIFE;
				break;
			case SCANCODE_2:
				if (gamestate.ammo) {
					gamestate.pendingweapon = WP_PISTOL;
				}	
				break;
			case SCANCODE_3:
				if (gamestate.ammo && gamestate.machinegun) {
					gamestate.pendingweapon = WP_MACHINEGUN;
				}
				break;
			case SCANCODE_4:
				if (gamestate.ammo && gamestate.chaingun) {
					gamestate.pendingweapon = WP_CHAINGUN;
				}
				break;
			case SCANCODE_5:
				if (gamestate.gas && gamestate.flamethrower) {
					gamestate.pendingweapon = WP_FLAMETHROWER;
				}
				break;
			case SCANCODE_6:
				if (gamestate.missiles && gamestate.missile) {
					gamestate.pendingweapon = WP_MISSILE;
				}
				break;
			case SCANCODE_PERIOD:
			case SCANCODE_SLASH:
				joystick1 = JOYPAD_START;
				break;
			}
		}
		
		if (keys[SCANCODE_CURSORUPLEFT])
			joystick1 |= (JOYPAD_UP|JOYPAD_LFT);
		if (keys[SCANCODE_CURSORUP])
			joystick1 |= JOYPAD_UP;
		if (keys[SCANCODE_CURSORUPRIGHT])
			joystick1 |= (JOYPAD_UP|JOYPAD_RGT);
		if (keys[SCANCODE_CURSORRIGHT])
			joystick1 |= JOYPAD_RGT;
		if (keys[SCANCODE_CURSORDOWNRIGHT])
			joystick1 |= (JOYPAD_DN|JOYPAD_RGT);
		if (keys[SCANCODE_CURSORDOWN])
			joystick1 |= JOYPAD_DN;
		if (keys[SCANCODE_CURSORDOWNLEFT])
			joystick1 |= (JOYPAD_DN|JOYPAD_LFT);
		if (keys[SCANCODE_CURSORLEFT])
			joystick1 |= JOYPAD_LFT;	
		
		if (keys[SCANCODE_CURSORBLOCKLEFT]) 
			joystick1 |= JOYPAD_LFT;
		if (keys[SCANCODE_CURSORBLOCKRIGHT])
			joystick1 |= JOYPAD_RGT;
		if (keys[SCANCODE_CURSORBLOCKUP])
			joystick1 |= JOYPAD_UP;
		if (keys[SCANCODE_CURSORBLOCKDOWN])
			joystick1 |= JOYPAD_DN;
		
		if (keys[SCANCODE_KEYPADENTER])
			joystick1 |= JOYPAD_A;	
		if (keys[SCANCODE_ENTER])
			joystick1 |= JOYPAD_A;
		if (keys[SCANCODE_SPACE])
			joystick1 |= JOYPAD_A;
		
		if (keys[SCANCODE_LEFTALT]) 
			joystick1 |= JOYPAD_TL;
		if (keys[SCANCODE_RIGHTALT])
			joystick1 |= JOYPAD_TR;
			
		if (keys[SCANCODE_LEFTCONTROL])
			joystick1 |= JOYPAD_B;
		if (keys[SCANCODE_RIGHTCONTROL])
			joystick1 |= JOYPAD_B;
		
		if (keys[SCANCODE_LEFTSHIFT])
			joystick1 |= (JOYPAD_X|JOYPAD_Y);
		if (keys[SCANCODE_RIGHTSHIFT])
			joystick1 |= (JOYPAD_X|JOYPAD_Y);	
	}
							
}

void ReadSystemJoystick(void)
{
	RSJ = 1;
//#ifndef NOVGA
	keyboard_update();
//#endif
}

/* 
Handle events, and return:
last keypress (if any) in ascii
mouse button events == 1
zero means none of the above
*/

int DoEvents()
{
	RSJ = 0;
//#ifndef NOVGA
	keyboard_update();
//#endif
	/* TODO: hack */
	return 'B';
}