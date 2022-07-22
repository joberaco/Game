#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "SDL2/SDL.h" 
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_mixer.h"

#define LEVEL_WIDTH 1980
#define LEVEL_HEIGHT 990

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600

#define DELAY_MS 2000

#define SHEET_PATH "imgs/sheet.png"
#define TILE_SHEET_PATH "imgs/tiles.png"
#define BACKGROUND_PATH "imgs/back.png"
#define TEXT_BOX_PATH "imgs/txtbox.png"
#define RECORDER_BUTTON_PATH "imgs/recorder_button.png"
#define SOUND_WAVE_PATH "imgs/sound_wave.png"
#define SPARKLES_PATH "imgs/sparkles_mini.png"
#define POWER_PELLET_PATH "imgs/power.png"
#define TITLE_FONT_PATH "fonts/pac.ttf"
#define TEXT_BOX_FONT_PATH "fonts/slkscr.ttf"
#define WAKA_PATH "sounds/waka.wav"
#define SAVE_FILE_PATH "data/pac_dialog_sf.txt"
#define TILE_MAP_FILE "data/level.map"
#define SAVE_FILE_DELIMITER '\n'
#define SAVED_PROMPT_STR "SAVED!"
#define MAX_RECORDING_SECONDS 3

#define SHEET_INITIAL_POS_X 76
#define SHEET_INITIAL_POS_Y 14
#define SHEET_STANDARD_SPRITE_SIZE 194
#define TILE_MAP_SIZE ((LEVEL_WIDTH/90) * (LEVEL_HEIGHT/90))

#define PAC_SPEED 10
#define TEXT_BOX_BUFFER_SIZE 20
#define TITLE_FONT_SIZE 48
#define TEXT_BOX_FONT_SIZE 36
#define TEXT_BOX_MAX_LINE_SIZE 10
#define AUDIO_DEVICE_NAME_SIZE 30
#define N_SPARKLES_PARTICLES 40
#define POWER_UP_SECONDS 10

typedef struct{
    SDL_Texture *texture;
    const char *filename;
	int w;
	int h;
	bool pixelstream;
	void *pixels;
	int pitch;
} Texture;

typedef struct{
	int x, y;
	int r;
} Circle;

typedef struct Sprite{
    SDL_Rect *clips;
	SDL_Rect *renderRect;
	SDL_Rect collider;
	SDL_Rect *boxColliders;
	Circle circleCollider;
	Texture *sheet;
	SDL_Rect *scaleRect;
	SDL_Point *center;
	SDL_RendererFlip flip;
	int nClips;
	int nBoxColliders;
	int x, y, w, h;
	int velX, velY;
	int frame;
	double angle;
	void (*collisionHandler)(void*);
} Sprite;

typedef struct{
	Sprite sprite;
	Texture textTexture;
	SDL_Color textColor;
	char textBuffer[TEXT_BOX_BUFFER_SIZE];
	int x, y, w, h;
} Textbox;

typedef struct{
	int type;
	SDL_Rect renderRect;
	SDL_Rect collider;
	bool solid;
	bool visible;
	int x, y, w, h;
} Tile;

typedef struct{
	Tile *tiles;
	int size;
	Texture *sheet;
	SDL_Rect *tileClips;
	int nTileTypes;
} TileMap;

typedef enum
{
	PAUSED,
	RECORDING,
	RECORDED,
	PLAYBACK
} AudioDeviceStatesEnum;

typedef struct{
	char name[AUDIO_DEVICE_NAME_SIZE];
	int recordingId;
	int playbackId;
	SDL_AudioSpec recordingSpec;
	SDL_AudioSpec playbackSpec;
	Uint8 *audioBuffer;
	int audiobufferSize;
	int bufferCurrentPos;
	int bufferMaxPos;
	bool available;
	AudioDeviceStatesEnum state;
} AudioDevice;

enum PacPositionsEnum
{
	PAC_CLOSED,
	PAC_HALF_OPENED,
	PAC_OPENED,
	N_PAC_POSITIONS
};

enum GhostPositionsEnum
{
	GHOST_DEFAULT,
	N_GHOST_POSITIONS
};

enum RecorderButtonRendersEnum
{
	OFF,
	ON,
	N_RECORDER_BUTTON_RENDERS
};

enum SoundWaveRendersEnum
{
	FIRST_WAVE,
	SECOND_WAVE,
	FULL_WAVE,
	N_SOUND_WAVE_RENDERS
};

enum SparklesRendersEnum
{
	SMALL_SPARK,
	MEDIUM_SPARK,
	BIG_SPARK,
	N_SPARKLES_RENDERS
};

enum GhostFamilyEnum
{
	BLINKY,
	INKY,
	N_GHOSTS
};

enum TileTypeEnum
{
	UNDEFINED = -1,
	EMPTY,
	STANDARD_BLOCK,
	N_TILE_TYPES
};

bool init();
bool loadMedia();
void close();

SDL_Surface* loadSurface(const char* path);
SDL_Surface* loadPixelSurface(const char* path);
Texture loadTexture(const char* path, SDL_Color* colorKey);
Texture loadPixelTexture(const char* path);
Texture loadRenderedText(const char* text, SDL_Color color, TTF_Font* font);
Tile loadTile(int type, SDL_Rect renderRect, bool solid, bool visible);
TileMap loadTileMap(int size, const char *tileFileName, Texture* tileSheet, SDL_Rect* tileClips, int nTileTypes);
void render(Texture texture, int x, int y, SDL_Rect* clip, SDL_Rect* scaleRect, double angle, SDL_Point* center, SDL_RendererFlip flip, SDL_Rect* camera);
bool checkCollision(SDL_Rect a, SDL_Rect b);
bool checkInnerBoxesCollisions(SDL_Rect* boxCollidersA, int nBoxesA, SDL_Rect* boxCollidersB, int nBoxesB);
void shiftBoxColliders(Sprite* sprite, int velX, int velY);
bool checkCircularCollision(Sprite a, Sprite b);
bool checkLevelBoundsCollision(Sprite sprite);
bool checkTileMapCollisions(Sprite sprite);
bool lockPixelTexture(Texture* texture);
bool unlockPixelTexture(Texture* texture);

Sprite loadSprite(int nClips, Texture* sheet, int x, int y, double angle, SDL_Point* center, SDL_RendererFlip flip, void (*collisionHandler)(void*));
Textbox loadTextBox(const char* defaultText, SDL_Color textColor);
AudioDevice loadAudioDevice(const char* recName, const char* playbackName);
Sprite loadSparklesSprite(SDL_Point initialPosition);
void setDefaultCollider(Sprite* sprite);
void loadSavedText();
void addClip(Sprite* sprite, int index, SDL_Rect clip, bool setRender);
void setScaleRect(Sprite* sprite, int w, int h);
void setRenderRect(Sprite* sprite, int index);
void updateSpriteSize(Sprite* sprite);

void move(Sprite* sprite, Sprite** spritesColliding, int nSpritesColliding);
void moveTo(Sprite* sprite, SDL_Point pos);
void moveAllColliders(Sprite* sprite, int velX, int velY);
void animate(Sprite* sprite, int delayFactor);
void renderSprite(Sprite sprite, SDL_Rect* camera);
void renderTile(TileMap map, int index, SDL_Rect* camera);
void renderTileMap(TileMap map, SDL_Rect* camera);
void renderColliders(Sprite sprite, SDL_Rect* camera, SDL_Color color);
void freeSprite(Sprite* sprite);

void hanndlePacInput();
void handleTextInput(SDL_Event event);
void handleAudioInput();
void handleWindowEvents(SDL_Event event);
void renderTextBox(Textbox* textbox);
void renderPacTextBoxes();
void renderGhostsTextBoxes();
void renderSparkles();
void switchRecorder(SDL_Event event);
void renderPacRecorderButton();
void renderPacSoundWave();
void addSineWaveTexture(TileMap* map, int startPeriod);
void randomizeGhostsVelocity();
void centerCamera();
void print_err(const char* msg);
int distanceSquared(int x1, int y1, int x2, int y2);
bool hasColliders(Sprite sprite);

void defaultAudioRecordingCallback(void* userdata, Uint8* stream, int len);
void defaultAudioPlaybackCallback(void* userdata, Uint8* stream, int len);
void pacCollisionHandler(void* objectColliding);

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Rect *camera = NULL;
Texture sheet, title, background, textBoxSheet, recorderButtonSheet, soundwaveSheet, sparklesSheet, powerUpSheet, tileSheet, tileSheetOrig;
Sprite pac, ghosts[N_GHOSTS], textCursor, pacRecorder, soundwave, sparkles[N_SPARKLES_PARTICLES], powerUp;
Textbox pacTextBox, blinkyTextBox, inkyTextBox, savedPromptTextBox;
TileMap map;
AudioDevice pacAudioDevice;
TTF_Font *titleFont = NULL, *textBoxFont = NULL;
Mix_Chunk *waka = NULL;
SDL_RWops *saveFile;
bool textSaved = false;
SDL_Color black = {0, 0, 0, 0};
SDL_Color yellow = {255, 255, 0, 0};
SDL_Color green = {25, 102, 25, 0};
SDL_Color lightBlack = {80, 80, 80, 0};

int main(int argc, char** argv)
{
	bool quit = false;
	SDL_Event event;
	int gframe = 0;
	//int fps = 0;
	Uint32 stime = 0, time = 0;
	//int backgroundOffset = 0;
	bool powered = false;
	int poweredStartTime = 0;

	camera = (SDL_Rect*)SDL_malloc(sizeof(SDL_Rect));
	*camera = (SDL_Rect){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

	if(!init()){
		printf("Could not initialize!\n");
	}
	else{
		if(!loadMedia()){
			printf("Could not load media");
		}
		else
		{
			stime = SDL_GetTicks();

			while(!quit)
			{
				while(SDL_PollEvent(&event) != 0)
				{
					switch(event.type)
					{
					case SDL_QUIT:
						quit = true;
						break;
					default:
						handleWindowEvents(event);
						handleTextInput(event);
						switchRecorder(event);
						break;
					}
				}

				handleAudioInput();
				hanndlePacInput();
				randomizeGhostsVelocity();
				centerCamera();

				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				SDL_RenderClear(renderer);

				/*if(Mix_Playing(-1) == 0){
					Mix_PlayChannel(-1, waka, 0);
				}*/

				Sprite* colliders[3];
				colliders[0] = &pac;
				colliders[1] = &ghosts[BLINKY];
				colliders[2] = &ghosts[INKY];

				move(&pac, colliders, 3);
				animate(&pac, 2);

				move(&ghosts[BLINKY], colliders, 3);
				move(&ghosts[INKY], colliders, 3);

				pac.frame++;
				gframe++;

				time = (SDL_GetTicks() - stime) / 1000;
				/*if(time > 0)
					fps = gframe / time;*/

				//printf("animation frame = %i, elapsed time = %i, frames = %i, fps = %i, ticks = %i\n", pac.frame, time, gframe, fps, SDL_GetTicks());

				/*--backgroundOffset;
				if(backgroundOffset < -background.w){
					backgroundOffset = 0;
				}*/

				/*render(background, backgroundOffset, 0, NULL, 0, NULL, SDL_FLIP_NONE, camera);
				render(background, backgroundOffset + background.w-1, 0, NULL, 0, NULL, SDL_FLIP_NONE, camera);*/
				render(background, 0, 0, NULL, NULL, 0, NULL, SDL_FLIP_NONE, camera);
				renderTileMap(map, camera);
				render(title, 0, 0, NULL, NULL, 0, NULL, SDL_FLIP_NONE, camera);
				
				renderPacTextBoxes();
				renderGhostsTextBoxes();
				renderPacRecorderButton();

				if(!powered && checkCollision(pac.collider, powerUp.collider)){
					powered = true;
					poweredStartTime = time;
				}

				if(!powered){
					renderSprite(powerUp, camera);
				}
				else{
					renderSparkles();

					if((time-poweredStartTime) > POWER_UP_SECONDS){
						powered = false;

						moveTo(&powerUp, (SDL_Point){rand()%LEVEL_WIDTH - 200, rand()%LEVEL_HEIGHT - 200});

						/*powerUp.x = rand()%LEVEL_WIDTH;
						powerUp.y = rand()%LEVEL_HEIGHT;
						powerUp.collider.x = powerUp.x;
						powerUp.collider.y = powerUp.y;*/
					}
					
				}

				renderSprite(ghosts[BLINKY], camera);
				renderSprite(ghosts[INKY], camera);
				renderSprite(pac, camera);

				renderColliders(pac, camera, (SDL_Color){0, 255, 0});
				renderColliders(ghosts[BLINKY], camera, (SDL_Color){0, 255, 0});
				renderColliders(ghosts[INKY], camera, (SDL_Color){0, 255, 0});

				/*SDL_RenderDrawLine(renderer, pac.circleCollider.x, pac.circleCollider.y, pac.circleCollider.x + pac.circleCollider.r * cos(45*3.14/180), pac.circleCollider.y - pac.circleCollider.r * sin(45*3.14/180));
				SDL_RenderDrawLine(renderer, pac.circleCollider.x, pac.circleCollider.y + pac.circleCollider.r, pac.circleCollider.x, pac.circleCollider.y - pac.circleCollider.r);
				SDL_RenderDrawLine(renderer, pac.circleCollider.x - pac.circleCollider.r, pac.circleCollider.y, pac.circleCollider.x + pac.circleCollider.r, pac.circleCollider.y);*/

				SDL_RenderPresent(renderer);
			}
		}
	}

	close();
	return 0;   
}

/*INIT SDL*/
bool init()
{
	int imgFlags;

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
		print_err("Could not initialize SDL"); 
	}
	else{
		window = SDL_CreateWindow("Mein Window", 300, 300, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

		if(window == NULL){
			print_err("Could not create window");
			return false;
		}
		else{
			//Window resize constraints
			SDL_SetWindowMaximumSize(window, LEVEL_WIDTH, LEVEL_HEIGHT);
			SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

			//random seeds
			srand(time(NULL));

			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

			if(renderer == NULL){
				print_err("Could not create renderer");
				return false;
			}
			else{
				imgFlags = IMG_INIT_PNG;
			
				if(!(IMG_Init(imgFlags) & imgFlags)){
					print_err("Could not initialize SDL_image");
					return false;
				}

				if(TTF_Init() == -1){
					print_err("Could not initialize SDL_ttf");
					return false;
				}

				if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0){
					print_err("Could not initialize SDL_mixer");
					return false;
				}

				SDL_StartTextInput();
			}
		}
	}

	return true;
}

bool loadMedia()
{
	int x, y;

	sheet = loadTexture(SHEET_PATH, &black);
	textBoxSheet = loadTexture(TEXT_BOX_PATH, NULL);
	background = loadTexture(BACKGROUND_PATH, NULL);
	recorderButtonSheet = loadTexture(RECORDER_BUTTON_PATH, NULL);
	soundwaveSheet = loadTexture(SOUND_WAVE_PATH, NULL);
	sparklesSheet = loadTexture(SPARKLES_PATH, NULL);
	powerUpSheet = loadTexture(POWER_PELLET_PATH, NULL);
	tileSheet = loadPixelTexture(TILE_SHEET_PATH);
	tileSheetOrig = loadPixelTexture(TILE_SHEET_PATH);

	if(sheet.texture == NULL || background.texture == NULL || textBoxSheet.texture == NULL || recorderButtonSheet.texture == NULL){
		return false;
	}
	else{
		//******PAC SPRITE
		pac = loadSprite(N_PAC_POSITIONS, &sheet, LEVEL_WIDTH/2, LEVEL_HEIGHT/2, 0, NULL, SDL_FLIP_NONE, pacCollisionHandler);
		
		x = SHEET_INITIAL_POS_X;
		y = SHEET_INITIAL_POS_Y;
		addClip(&pac, PAC_CLOSED, (SDL_Rect){
			x, y, 
			SHEET_STANDARD_SPRITE_SIZE, SHEET_STANDARD_SPRITE_SIZE},
			true
		);
		
		x += SHEET_STANDARD_SPRITE_SIZE + 45;
		addClip(&pac, PAC_HALF_OPENED, (SDL_Rect){
			x, y, 
			SHEET_STANDARD_SPRITE_SIZE - 14, SHEET_STANDARD_SPRITE_SIZE},
			false
		);

		x += SHEET_STANDARD_SPRITE_SIZE + 28;
		addClip(&pac, PAC_OPENED, (SDL_Rect){
			x, y, 
			SHEET_STANDARD_SPRITE_SIZE - 65, SHEET_STANDARD_SPRITE_SIZE},
			false
		);

		setDefaultCollider(&pac);

		//PAC BOX COLLIDERS
		pac.nBoxColliders = 5;
		pac.boxColliders = (SDL_Rect*) SDL_malloc(sizeof(SDL_Rect) * pac.nBoxColliders);

		pac.boxColliders[0] = (SDL_Rect){pac.x + 59, pac.y, 75, 15};
		pac.boxColliders[1] = (SDL_Rect){pac.x + 14, pac.y + 25, 166, 30};
		pac.boxColliders[2] = (SDL_Rect){pac.x, pac.y + 50, 195, 75};
		pac.boxColliders[3] = (SDL_Rect){pac.x + 14, pac.y + 130, 166, 30};
		pac.boxColliders[4] = (SDL_Rect){pac.x + 59, pac.y + 170, 75, 15};

		//PAC CIRCULAR COLLIDER
		/*pac.circleCollider.x = pac.x + 95;
		pac.circleCollider.y = pac.y + 95;
		pac.circleCollider.r = 95;*/

		//******RED GHOST SPRITE
		ghosts[BLINKY] = loadSprite(N_GHOST_POSITIONS, &sheet, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 0, NULL, SDL_FLIP_NONE, NULL);

		x += 211;
		y -= 14;
		addClip(&ghosts[BLINKY], GHOST_DEFAULT, (SDL_Rect){
			x, y, 
			SHEET_STANDARD_SPRITE_SIZE + 18, SHEET_STANDARD_SPRITE_SIZE + 14},
			true
		);

		setDefaultCollider(&ghosts[BLINKY]);
		ghosts[BLINKY].velX = PAC_SPEED;
		ghosts[BLINKY].velY = 0;

		//RED GHOST COLLIDERS
		ghosts[BLINKY].nBoxColliders = 3;
		ghosts[BLINKY].boxColliders = (SDL_Rect*) SDL_malloc(sizeof(SDL_Rect) * ghosts[BLINKY].nBoxColliders);

		ghosts[BLINKY].boxColliders[0] = (SDL_Rect){ghosts[BLINKY].x + 75, ghosts[BLINKY].y, 60, 15};
		ghosts[BLINKY].boxColliders[1] = (SDL_Rect){ghosts[BLINKY].x + 15, ghosts[BLINKY].y + 45, 180, 45};
		ghosts[BLINKY].boxColliders[2] = (SDL_Rect){ghosts[BLINKY].x, ghosts[BLINKY].y + 90, 210, 105};

		//******BLUE GHOST SPRITE
		ghosts[INKY] = loadSprite(N_GHOST_POSITIONS, &sheet, SCREEN_WIDTH/2, SCREEN_HEIGHT-50, 0, NULL, SDL_FLIP_NONE, NULL);

		x += 390;
		y += 234;
		addClip(&ghosts[INKY], GHOST_DEFAULT, (SDL_Rect){
			x, y, 
			SHEET_STANDARD_SPRITE_SIZE + 18, SHEET_STANDARD_SPRITE_SIZE + 14},
			true
		);

		//BLUE GHOST COLLIDERS
		setDefaultCollider(&ghosts[INKY]);
		ghosts[INKY].velX = 0;
		ghosts[INKY].velY = PAC_SPEED;

		//******RECORDER BUTTON
		pacRecorder = loadSprite(N_RECORDER_BUTTON_RENDERS, &recorderButtonSheet, 0, 0, 0, NULL, SDL_FLIP_NONE, NULL);

		x = 50;
		y = 60;
		addClip(&pacRecorder, OFF, (SDL_Rect){x, y, 160, 161}, true);
		addClip(&pacRecorder, ON, (SDL_Rect){240, y, 160, 161}, false);

		setScaleRect(&pacRecorder, pacRecorder.w/2, pacRecorder.h/2);

		//******SOUND WAVE SPRITE
		soundwave = loadSprite(N_SOUND_WAVE_RENDERS, &soundwaveSheet, 0, 0, 0, NULL, SDL_FLIP_NONE, NULL);

		addClip(&soundwave, FIRST_WAVE, (SDL_Rect){261, 4, 131, 191}, true);
		addClip(&soundwave, SECOND_WAVE, (SDL_Rect){151, 4, 131, 191}, false);
		addClip(&soundwave, FULL_WAVE, (SDL_Rect){14, 4, 131, 191}, false);

		setScaleRect(&soundwave, soundwave.w/2, soundwave.h/2);

		//******SPARKLES PARTICLES
		int i;
		for(i = 0; i < N_SPARKLES_PARTICLES; i++)
			sparkles[i] = loadSparklesSprite((SDL_Point){0, 0});

		//******POWER UP
		powerUp = loadSprite(1, &powerUpSheet, rand()%LEVEL_WIDTH, rand()%LEVEL_HEIGHT, 0, NULL, SDL_FLIP_NONE, NULL);
		addClip(&powerUp, 0, (SDL_Rect){0, 0, powerUpSheet.w, powerUpSheet.h}, true);
		setDefaultCollider(&powerUp);

		//******LEVEL TILES
		SDL_Rect tileClips[] = {(SDL_Rect){0, 0, tileSheet.w, tileSheet.h}, (SDL_Rect){0, 0, tileSheet.w, tileSheet.h}};
		map = loadTileMap(TILE_MAP_SIZE, TILE_MAP_FILE, &tileSheet, tileClips, N_TILE_TYPES);
		addSineWaveTexture(&map, 0);
	}

	//******FONT & TEXT TEXTURES
	titleFont = TTF_OpenFont(TITLE_FONT_PATH, TITLE_FONT_SIZE);
	textBoxFont = TTF_OpenFont(TEXT_BOX_FONT_PATH, TEXT_BOX_FONT_SIZE);

	if(titleFont == NULL || textBoxFont == NULL){
		return false;
	}
	else{
		title = loadRenderedText("pacman", yellow, titleFont);

		//******TEXT BOXES
		//PAC TEXT BOX
		pacTextBox = loadTextBox("hey", black);

		//"SAVED" TEXT BOX
		savedPromptTextBox = loadTextBox(SAVED_PROMPT_STR, green);
		setScaleRect(&savedPromptTextBox.sprite, savedPromptTextBox.textTexture.w + 20, savedPromptTextBox.sprite.h);
		savedPromptTextBox.sprite.flip = SDL_FLIP_HORIZONTAL;

		//BLINKY TEXT BOX
		blinkyTextBox = loadTextBox("yoo", black);
		//INKY TEXT BOX
		inkyTextBox = loadTextBox("ronaldinho soccer", black);

		//******TEXT CURSOR
		textCursor = loadSprite(2, (Texture*)SDL_malloc(sizeof(Texture)), 0, 0, 0, NULL, SDL_FLIP_NONE, NULL);
		*textCursor.sheet = loadRenderedText("_", black, textBoxFont);
		addClip(&textCursor, 0, (SDL_Rect){0, 0, textCursor.sheet->w, textCursor.sheet->h}, true);
		addClip(&textCursor, 1, (SDL_Rect){0, 0, 0, 0}, false);
	}

	//******AUDIO INIT
	waka = Mix_LoadWAV(WAKA_PATH);

	if(waka == NULL){
		return false;
	}

	//RECORDING DEVICES
	if(SDL_GetNumAudioDevices(SDL_TRUE) >= 1){
		int i, nDevices;
		const char *recName, *playbackName;

		nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
		for(i = 0; i < nDevices; i++){
			recName = SDL_GetAudioDeviceName(i, SDL_TRUE);
			if(strstr(recName, "High Definition") != NULL)
				break;
		}

		nDevices = SDL_GetNumAudioDevices(SDL_FALSE);
		for(i = 0; i < nDevices; i++){
			playbackName = SDL_GetAudioDeviceName(i, SDL_FALSE);
			if(strstr(playbackName, "High Definition") != NULL)
				break;
		}

		pacAudioDevice = loadAudioDevice(recName, playbackName);
	}

	//******SAVE FILE & RANDOM TEXTBOX PROMPT ASSIGNMENT
	saveFile = SDL_RWFromFile(SAVE_FILE_PATH, "a+");
	loadSavedText();

	return true;
}

/*ADD SPECIFIED clip TO GIVEN sprite ON index (IN nClips RANGE), SET AS CURRENT RENDER IF setRender*/
void addClip(Sprite* sprite, int index, SDL_Rect clip, bool setRender)
{
	if(index < 0 || index >= sprite->nClips)
		return;
	
	sprite->clips[index] = clip;

	if(setRender){
		sprite->renderRect = &sprite->clips[index];
		updateSpriteSize(sprite);
	}
}

/*SET sprite'S SCALE RECT TO SIZE (w,h) (CREATE NEW IF NULL) AND UPDATE sprite INTERNAL SIZE*/
void setScaleRect(Sprite* sprite, int w, int h)
{
	if(sprite->scaleRect == NULL){
		sprite->scaleRect = (SDL_Rect*)SDL_malloc(sizeof(SDL_Rect));
	}

	*sprite->scaleRect = (SDL_Rect){0, 0, w, h};
	updateSpriteSize(sprite);
}

/*UPDATE sprite INTERNAL SIZE(w,h) BASED ON SCALING AND RENDERING COMPONENTS*/
void updateSpriteSize(Sprite* sprite)
{
	if(sprite->scaleRect != NULL){
		sprite->w = sprite->scaleRect->w;
		sprite->h = sprite->scaleRect->h;
	}
	else{
		sprite->w = sprite->renderRect->w;
		sprite->h = sprite->renderRect->h;
	}
}

/*LOAD NEW AUDIO DEVICE STREAMING ON recordingDeviceName & playbackDeviceName*/
AudioDevice loadAudioDevice(const char* recordingDeviceName, const char* playbackDeviceName)
{
	AudioDevice device;

	strncpy(device.name, recordingDeviceName, AUDIO_DEVICE_NAME_SIZE);
	device.available = true;

	//default audio recording spec
	SDL_AudioSpec desiredSpec;
	SDL_zero(desiredSpec);
	desiredSpec.freq = 44100;
	desiredSpec.format = AUDIO_F32;
	desiredSpec.channels = 2;
	desiredSpec.samples = 4096;
	desiredSpec.callback = defaultAudioRecordingCallback;

	device.recordingId = SDL_OpenAudioDevice(recordingDeviceName, SDL_TRUE, &desiredSpec, &device.recordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if(device.recordingId == 0){
		strcpy(device.name, "No recording devices :(");
		device.available = false;
	}

	//default audio playback spec
	desiredSpec.callback = defaultAudioPlaybackCallback;

	device.playbackId = SDL_OpenAudioDevice(playbackDeviceName, SDL_FALSE, &desiredSpec, &device.playbackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if(device.playbackId == 0){
		strcpy(device.name, "No playback devices :(");
		device.available = false;
	}

	if(device.available){
		int bytesPerSample = device.recordingSpec.channels * (SDL_AUDIO_BITSIZE(device.recordingSpec.format) / 8);
		int bytesPerSecond = device.recordingSpec.freq * bytesPerSample;

		device.audiobufferSize = (MAX_RECORDING_SECONDS+1) * bytesPerSecond;
		device.bufferMaxPos = MAX_RECORDING_SECONDS * bytesPerSecond;

		device.audioBuffer = (Uint8*)SDL_calloc(device.audiobufferSize, sizeof(Uint8));
		device.bufferCurrentPos = 0;
	}

	device.state = PAUSED;

	return device;
}

/*DEFAULT RECORDING CALLBACK FOR AudioDevices*/
void defaultAudioRecordingCallback(void* userdata, Uint8* stream, int len)
{
	SDL_memcpy(&pacAudioDevice.audioBuffer[pacAudioDevice.bufferCurrentPos], stream, len);
	pacAudioDevice.bufferCurrentPos += len;
}

/*DEFAULT PLAYBACK CALLBACK FOR AudioDevices*/
void defaultAudioPlaybackCallback(void* userdata, Uint8* stream, int len)
{
	SDL_memcpy(stream, &pacAudioDevice.audioBuffer[pacAudioDevice.bufferCurrentPos], len);
	pacAudioDevice.bufferCurrentPos += len;
}

/*COLLISION HANDLER TO PACMAN'S SPRITE*/
void pacCollisionHandler(void* objectColliding)
{
	Sprite *spriteColliding = (Sprite*)objectColliding;
	Tile *tileColliding = (Tile*)objectColliding;

	const char *pactext = "OHSNAP :(";
	const char *inkytext = ">:)";
	const char *blinkytext = "DUCK YOU";
	int pactextLen = strlen(pactext);
	int inkyTextLen = strlen(inkytext), blinkyTextLen = strlen(blinkytext);

	if(spriteColliding == &ghosts[INKY]){
		strncpy(pacTextBox.textBuffer, pactext, pactextLen);
		pacTextBox.textBuffer[pactextLen] = '\0';

		strncpy(inkyTextBox.textBuffer, inkytext, inkyTextLen);
		inkyTextBox.textBuffer[inkyTextLen] = '\0';
	}

	if(spriteColliding == &ghosts[BLINKY]){
		strncpy(pacTextBox.textBuffer, pactext, pactextLen);
		pacTextBox.textBuffer[pactextLen] = '\0';

		strncpy(blinkyTextBox.textBuffer, blinkytext, blinkyTextLen);
		blinkyTextBox.textBuffer[blinkyTextLen] = '\0';
	}

	if(tileColliding->type == STANDARD_BLOCK){
		addSineWaveTexture(&map, pac.frame);
	}
}

/*LOAD SAVED TEXT FOR RANDOM TEXT PROMTS*/
void loadSavedText()
{
	char textLine[TEXT_BOX_BUFFER_SIZE] = "";
	int fileSize = SDL_RWsize(saveFile), totalRead = 0, lineRead = 0, byteRead = 1;
	int promptsSet = 0, nPrompts;
	char *textPromts[] = {pacTextBox.textBuffer, blinkyTextBox.textBuffer, inkyTextBox.textBuffer};

	nPrompts = sizeof(textPromts) / sizeof(char*);

	if(fileSize <= 0){
		return; //no saved data
	}

	while(promptsSet < nPrompts && totalRead < fileSize && byteRead != 0)
	{
		byteRead = SDL_RWread(saveFile, textLine+lineRead, 1, 1);
		totalRead += byteRead;
		lineRead += byteRead;

		if(byteRead && (lineRead == TEXT_BOX_BUFFER_SIZE || textLine[lineRead-1] == SAVE_FILE_DELIMITER)){ //end of line
			if(rand()%fileSize<totalRead){ //rand select
				strncpy(textPromts[promptsSet], textLine, lineRead);
				textPromts[promptsSet++][lineRead-1] = '\0';
			}
			lineRead = 0;
		}
	}
}

/*CLOSE AND EXIT SDL & SUBSYSTEMS*/
void close()
{
	SDL_DestroyTexture(sheet.texture);
	sheet.texture = NULL;

	SDL_DestroyTexture(background.texture);
	background.texture = NULL;

	SDL_DestroyTexture(title.texture);
	title.texture = NULL;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	Mix_FreeChunk(waka);
	waka = NULL;

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_RWclose(saveFile);
	saveFile = NULL;

	SDL_DestroyTexture(map.sheet->texture);
	SDL_free(map.tiles);
	SDL_free(map.tileClips);
	map.tiles = NULL;
	map.tileClips = NULL;


	freeSprite(&pac);
	freeSprite(&ghosts[BLINKY]);
	freeSprite(&ghosts[INKY]);
	freeSprite(&pacTextBox.sprite);
	freeSprite(&savedPromptTextBox.sprite);
	freeSprite(&pacRecorder);
	freeSprite(&soundwave);

	int i;
	for(i = 0; i < N_SPARKLES_PARTICLES; i++){
		freeSprite(&sparkles[i]);
	}

	if(pacAudioDevice.audioBuffer != NULL)
    {
        SDL_free(pacAudioDevice.audioBuffer);
        pacAudioDevice.audioBuffer = NULL;
    }

	IMG_Quit();
	TTF_Quit();
	Mix_Quit();
	SDL_StopTextInput();
	SDL_Quit();
}

/*LOAD SDL SURFACE FROM PATH*/
SDL_Surface* loadSurface(const char* path)
{
	SDL_Surface *surface = IMG_Load(path);

	if(surface == NULL){
		print_err("Could not load surface");
	}

	return surface;
}

/*LOAD SDL SURFACE FROM PATH WITH PIXEL STREAMING PROPERTIES*/
SDL_Surface* loadPixelSurface(const char* path)
{
	SDL_Surface *surface = loadSurface(path);
	SDL_Surface *formattedSurface = SDL_ConvertSurfaceFormat(surface, SDL_GetWindowPixelFormat(window), 0);

	if(formattedSurface == NULL){
		print_err("Could not load formatted surface for pixel streaming");
	}

	SDL_FreeSurface(surface);

	return formattedSurface;
}

/*LOAD SDL TEXTURE FROM PATH AND COLOR KEY IF NECESSARY*/
Texture loadTexture(const char* path, SDL_Color* colorKey)
{
	SDL_Texture *loadedTexture = NULL;
	SDL_Surface *loadedSurface = loadSurface(path);
	Texture texture;

	if(loadedSurface == NULL){
		print_err("Could not load surface for texture");
	}
	else
	{
		if(colorKey != NULL){
			SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, colorKey->r, colorKey->g, colorKey->b));
		}

		loadedTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);

		if(loadedTexture == NULL){
			print_err("Unable to load texture from surface");
		}
		else{
			texture = (Texture){loadedTexture, path, loadedSurface->w, loadedSurface->h, false, NULL, 0};
		}
	}

	SDL_FreeSurface(loadedSurface);

	return texture;
}

/*LOAD SDL TEXTURE FROM PATH WITH PIXEL STREAMING PROPERTIES*/
Texture loadPixelTexture(const char* path)
{
	SDL_Texture *loadedTexture = NULL;
	SDL_Surface *loadedSurface = loadPixelSurface(path);
	Texture texture;

	if(loadedSurface == NULL){
		print_err("Could not load surface for pixel streaming texture");
	}
	else
	{
		loadedTexture = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(window), SDL_TEXTUREACCESS_STREAMING, loadedSurface->w, loadedSurface->h);

		if(loadedTexture == NULL){
			print_err("Unable to load pixel streaming texture from surface");
		}
		else
		{
			SDL_LockTexture(loadedTexture, NULL, &texture.pixels, &texture.pitch);
			SDL_memcpy(texture.pixels, loadedSurface->pixels, loadedSurface->pitch * loadedSurface->h);
			SDL_UnlockTexture(loadedTexture);
			texture.pixels = NULL;

			texture = (Texture){loadedTexture, path, loadedSurface->w, loadedSurface->h, true, texture.pixels, texture.pitch};
		}
	}

	SDL_FreeSurface(loadedSurface);

	return texture;
}

/*LOCK VALID PIXEL-STREAMING texture*/
bool lockPixelTexture(Texture* texture)
{
	if(!texture->pixelstream){
		print_err("No pixel-streaming texture provided for locking");
		return false;
	}

	if(texture->pixels != NULL){
		print_err("Texture is already locked/pixels in use");
		return false;
	}

	if(SDL_LockTexture(texture->texture, NULL, &texture->pixels, &texture->pitch) != 0){
		print_err("Unable to lock texture");
		return false;
	}
	
	return true;
}

/*UNLOCK VALID PIXEL-STREAMING texture*/
bool unlockPixelTexture(Texture* texture)
{
	if(!texture->pixelstream){
		print_err("No pixel-streaming texture provided for unlocking");
		return false;
	}

	if(texture->pixels == NULL){
		print_err("Texture is already unlocked/pixels not set");
		return false;
	}

	SDL_UnlockTexture(texture->texture);
	texture->pixels = NULL;
	texture->pitch = 0;

	return true;
}

/*LOAD SDL TTF TEXTURE WITH GIVEN TEXT, COLOR AND FONT*/
Texture loadRenderedText(const char *text, SDL_Color color, TTF_Font* font)
{
	SDL_Texture *loadedTTFTexture = NULL;
	SDL_Surface *loadedTTFSurface = NULL;
	Texture ttfText;

	if(strlen(text) == 0) //empty texture on empty string
		loadedTTFSurface = TTF_RenderText_Solid(font, " ", color);
	else
		loadedTTFSurface = TTF_RenderText_Solid(font, text, color);

	if(loadedTTFSurface == NULL){
		print_err("Could not load TTF surface");
	}
	else
	{
		loadedTTFTexture = SDL_CreateTextureFromSurface(renderer, loadedTTFSurface);

		if(loadedTTFTexture == NULL){
			print_err("Unable to load TTF texture from surface");
		}
		else{
			ttfText = (Texture){loadedTTFTexture, "", loadedTTFSurface->w, loadedTTFSurface->h, false, NULL, 0};
		}
	}

	SDL_FreeSurface(loadedTTFSurface);
	loadedTTFSurface = NULL;

	return ttfText;
}

/*LOAD NEW TILE MAP FROM tileFileName WITH UP TO size TILES DEFINED BY nTileTypes AND tileSheet */
TileMap loadTileMap(int size, const char *tileFileName, Texture* tileSheet, SDL_Rect* tileClips, int nTileTypes)
{
	TileMap map;
	Tile currentTile;
	SDL_RWops *tileFile = SDL_RWFromFile(tileFileName, "r");
	int type;
	int i = 0;
	int x = 0, y = 0;
	char byte[1];

	map.tiles = (Tile*)SDL_malloc(sizeof(Tile) * size);
	map.tileClips = (SDL_Rect*)SDL_malloc(sizeof(SDL_Rect) * nTileTypes);

	SDL_memcpy(map.tileClips, tileClips, sizeof(SDL_Rect) * nTileTypes);

	map.size = size;
	map.sheet = tileSheet;
	map.nTileTypes = nTileTypes;

	if(tileFile != NULL)
	{
		while(i < size && SDL_RWread(tileFile, byte, 1, 1))
		{
			if(byte[0] == '\n' || byte[0] == '\r')
				continue;

			type = SDL_atoi(byte);

			if(type > 0 && type < nTileTypes){
				currentTile = loadTile(type, (SDL_Rect){x, y, tileClips[type].w, tileClips[type].h}, true, true);
			}
			else if(type == 0){
				currentTile = loadTile(EMPTY, (SDL_Rect){x, y, tileClips[type].w, tileClips[type].w}, false, false);
			}
			else{
				currentTile = loadTile(UNDEFINED, (SDL_Rect){0, 0, 0, 0}, false, false);
			}

			if(x + currentTile.w >= LEVEL_WIDTH){
				x = 0;
				y += currentTile.h;
			}
			else{
				x += currentTile.w;
			}

			map.tiles[i++] = currentTile;
		}
	}

	SDL_RWclose(tileFile);

	return map;
}

/*LOAD NEW TILE BASED ON TILEMAP type*/
Tile loadTile(int type, SDL_Rect renderRect, bool solid, bool visible)
{
	Tile tile;

	tile = (Tile){.x = renderRect.x, .y = renderRect.y, .w = renderRect.w, .h = renderRect.h};

	tile.type = type;
	tile.renderRect = renderRect;
	tile.collider = renderRect;

	tile.solid = solid;
	tile.visible = visible;

	return tile;
}

/*RENDER texture SCALED BY scaleRect AND clip FROM IT IF NECESSARY (RELATIVE TO camera IF NOT NULL)*/
void render(Texture texture, int x, int y, SDL_Rect* clip, SDL_Rect* scaleRect, double angle, SDL_Point* center, SDL_RendererFlip flip, SDL_Rect* camera)
{
	SDL_Rect renderSpace = {x, y, texture.w, texture.h};

	if(clip != NULL){
		renderSpace.w = clip->w;
		renderSpace.h = clip->h;
	}

	if(scaleRect != NULL){ //prioritize scaleRect
		renderSpace.w = scaleRect->w;
		renderSpace.h = scaleRect->h;
	}

	if(camera != NULL){
		renderSpace.x -= camera->x;
		renderSpace.y -= camera->y;
	}

	SDL_RenderCopyEx(renderer, texture.texture, clip, &renderSpace, angle, center, flip);
}

/*RENDER SPRITE (SCALED BY sprite.scaleRect IF NOT NULL, RELATIVE TO CAMERA IF NOT NULL)*/
void renderSprite(Sprite sprite, SDL_Rect* camera)
{
	render(*sprite.sheet, sprite.x, sprite.y, sprite.renderRect, sprite.scaleRect, sprite.angle, sprite.center, sprite.flip, camera);
}

/*RENDER LEVEL TILE BASED ON INTERNAL POSITION*/
void renderTile(TileMap map, int index, SDL_Rect* camera)
{
	Tile tile = map.tiles[index];
	render(*map.sheet, tile.x, tile.y, &map.tileClips[tile.type], NULL, 0, NULL, SDL_FLIP_NONE, camera);
}

/*RENDER FULL TILE MAP*/
void renderTileMap(TileMap map, SDL_Rect* camera)
{
	int i;

	for(i = 0; i < TILE_MAP_SIZE; i++)
	{
		if(map.tiles[i].visible && checkCollision(map.tiles[i].collider, *camera)){ //visible and inside camera view
			renderTile(map, i, camera);
		}
	}
}

/*LOAD NEW SPRITE AND SET RENDER RECT TO FIRST AVAILABLE CLIP (SET scaleRect TO NULL AND EMPTY collider BY DEFAULT)*/
Sprite loadSprite(int nClips, Texture* sheet, int x, int y, double angle, SDL_Point* center, SDL_RendererFlip flip, void (*collisionHandler)(void*))
{
	Sprite sprite;

	sprite.nClips = nClips > 0 ? nClips : 1;
		
	sprite.clips = (SDL_Rect*)SDL_malloc(sizeof(SDL_Rect) * sprite.nClips);
	sprite.renderRect = &sprite.clips[0];

	sprite.sheet = sheet;
	
	sprite.w = sprite.renderRect->w;
	sprite.h = sprite.renderRect->h;

	sprite.x = x;
	sprite.y = y;

	sprite.velX = 0;
	sprite.velY = 0;

	sprite.angle = angle;
	sprite.center = center;
	sprite.flip = flip;
	sprite.frame = 0;
	sprite.scaleRect = NULL;

	sprite.collider = (SDL_Rect){0, 0, 0, 0};
	sprite.circleCollider.r = 0;
	sprite.nBoxColliders = 0;

	sprite.collisionHandler = collisionHandler;

	return sprite;
}

/*SET A DEFAULT COLLIDER FOR sprite BASED ON ITS SIZE*/
void setDefaultCollider(Sprite *sprite)
{
	sprite->collider = (SDL_Rect){sprite->x, sprite->y, sprite->w, sprite->h};
}


/*LOAD NEW TEXTBOX RENDER OBJECT WITH DEFAULT PROPERTIES*/
Textbox loadTextBox(const char *defaultText, SDL_Color textColor)
{
	Textbox textbox;
	Sprite sprite;
	int textLen = strlen(defaultText);

	textbox.x = 0, textbox.y = 0;
	textbox.w = SHEET_STANDARD_SPRITE_SIZE+100, textbox.h = SHEET_STANDARD_SPRITE_SIZE/2;

	if(textLen > TEXT_BOX_BUFFER_SIZE)
		textLen = TEXT_BOX_BUFFER_SIZE;

	sprite = loadSprite(1, &textBoxSheet, textbox.x, textbox.y, 0, NULL, SDL_FLIP_NONE, NULL);
	addClip(&sprite, 0, (SDL_Rect){120, 150, 780, 190}, true);
	setScaleRect(&sprite, textbox.w, textbox.h);

	textbox.sprite = sprite;

	strncpy(textbox.textBuffer, defaultText, textLen);
	textbox.textBuffer[textLen] = '\0';
	textbox.textColor = textColor;
	textbox.textTexture = loadRenderedText(textbox.textBuffer, textbox.textColor, textBoxFont);

	return textbox;
}

/*DETECT COLLISION BETWEEN BOX A & B*/
bool checkCollision(SDL_Rect a, SDL_Rect b)
{
	if(a.x + a.w <= b.x)
		return false;
	else if(a.x >= b.x + b.w)
		return false;
	else if(a.y + a.h <= b.y)
		return false;
	else if(a.y >= b.y + b.h)
		return false;
	else
		return true; //COLLISION
}

/*DETECT FIRST COLLISION POINT BETWEEN BOX COLLIDER SET A & BOX COLLIDER SET B*/
bool checkInnerBoxesCollisions(SDL_Rect* boxCollidersA, int nBoxesA, SDL_Rect* boxCollidersB, int nBoxesB){
	int i, j;

	if(boxCollidersA == NULL || boxCollidersB == NULL)
		return false;

	for(i = 0; i < nBoxesA; i++){
		for(j = 0; j < nBoxesB; j++){
			if(checkCollision(boxCollidersA[i], boxCollidersB[j])){ //BOX LEVEL COLLISION
				return true;
			}
		}
	}

	//NO COLLISIONS REPORTED
	return false;
}

/*DETECT COLLISION BETWEEN SPRITES A & B CIRCULAR COLLIDERS (IF ANY)*/
bool checkCircularCollision(Sprite a, Sprite b)
{
	if(a.circleCollider.r != 0 && b.circleCollider.r != 0) //A & B CIRCULAR COLLIDERS EXIST
	{
		float radiiSumSquared = a.circleCollider.r + b.circleCollider.r;
		radiiSumSquared *= radiiSumSquared;

		if(distanceSquared(a.circleCollider.x, a.circleCollider.y, b.circleCollider.x, b.circleCollider.y) < radiiSumSquared){
			return true;
		}
	}
	else if((a.circleCollider.r != 0 && b.circleCollider.r == 0) || (a.circleCollider.r == 0 && b.circleCollider.r != 0)) //A OR B CIRCULAR COLLIDER EXISTS (CIRCLE VS RECT COLLISION)
	{
		int cX, cY;
		SDL_Rect boxCollider = a.circleCollider.r != 0 ? b.collider : a.collider;
		Circle circleCollider = a.circleCollider.r != 0 ? a.circleCollider : b.circleCollider;

		if(circleCollider.x < boxCollider.x){
			cX = boxCollider.x;
		}
		else if(circleCollider.x > boxCollider.x + boxCollider.w){
			cX = boxCollider.x + boxCollider.w;
		}
		else{
			cX = circleCollider.x;
		}

		if(circleCollider.y < boxCollider.y){
			cY = boxCollider.y;
		}
		else if(circleCollider.y > boxCollider.y + boxCollider.h){
			cY = boxCollider.y + boxCollider.h;
		}
		else{
			cY = circleCollider.y;
		}

		if(distanceSquared(circleCollider.x, circleCollider.y, cX, cY) < (circleCollider.r * circleCollider.r)){
			return true;
		}
	}

	//else NO CIRCULAR COLLIDERS EXIST/NO CIRCULAR COLLISIONS DETECTED
	return false;
}

/*CHECK COLLISION AGAINST LEVEL BOUNDS*/
bool checkLevelBoundsCollision(Sprite sprite)
{
	return sprite.collider.x < 0 || sprite.collider.x + sprite.collider.w > LEVEL_WIDTH || sprite.collider.y < 0 || sprite.collider.y + sprite.collider.h > LEVEL_HEIGHT;
}

/*CHECK COLLISIONS AGAINST LEVEL TILES MAP*/
bool checkTileMapCollisions(Sprite sprite)
{
	int i;

	for(i = 0; i < TILE_MAP_SIZE; i++){
		if(map.tiles[i].solid && checkCollision(sprite.collider, map.tiles[i].collider)){
			if(sprite.collisionHandler != NULL)
				sprite.collisionHandler(&map.tiles[i]);
			return true;
		}
	}

	return false;
}

/*SHIFT SPRITE'S INNER BOX COLLIDERS BY VELOCITY X & Y*/
void shiftBoxColliders(Sprite* sprite, int velX, int velY){
	int i;

	for(i = 0; i < sprite->nBoxColliders; i++){
		sprite->boxColliders[i].x += velX;
		sprite->boxColliders[i].y += velY;
	}
}

/*MOVE SPRITE IF NOT COLLIDING AGAINST spritesColliding OR LEVEL BOUNDS BASED ON ITS POSITION AND VELOCITY (IF APPLICABLE), SEND COLLISION TO collisionHandler IF NECESSARY*/
void move(Sprite* sprite, Sprite** spritesColliding, int nSpritesColliding)
{
	bool collision = false;
	int i, collidingIndex = -1;

	if(hasColliders(*sprite)) //CHECK FOR COLLISIONS
	{
		moveAllColliders(sprite, sprite->velX, sprite->velY);
		collision = checkLevelBoundsCollision(*sprite) || checkTileMapCollisions(*sprite); //VS LEVEL BOUNDS && LEVEL TILES CHECK

		for(i = 0; i < nSpritesColliding && !collision; i++){  //VS OTHER COLLIDERS
			if(spritesColliding[i] == NULL || spritesColliding[i] == sprite) continue; //SKIP NULL OR CALLER SPRITES 

			collision = checkCircularCollision(*sprite, *spritesColliding[i]) || //CIRCULAR COLLIDERS CHECK
						(checkCollision(sprite->collider, spritesColliding[i]->collider) && //OUTER BOX COLLISIONS CHECK
						(sprite->nBoxColliders == 0 || spritesColliding[i]->nBoxColliders == 0 || 
						checkInnerBoxesCollisions(sprite->boxColliders, sprite->nBoxColliders, spritesColliding[i]->boxColliders, spritesColliding[i]->nBoxColliders))); //INNER BOXES PER-PIXEL COLLISIONS CHECK
		}

		collidingIndex = i-1;
	}

	if(!collision){
		//MOVE SPRITE
		sprite->x += sprite->velX;
		sprite->y += sprite->velY;
	}
	else{
		//COLLISION HANDLER CALL
		if(sprite->collisionHandler != NULL && spritesColliding != NULL && collidingIndex >= 0)
			sprite->collisionHandler(spritesColliding[collidingIndex]);
		//MOVE COLLIDERS BACK
		moveAllColliders(sprite, -sprite->velX, -sprite->velY);
	}
}

/*MOVE sprite AND ITS INTERNAL COLLIDERS TO ABSOLUTE pos (NO COLLISION CHECKING)*/
void moveTo(Sprite* sprite, SDL_Point pos)
{
	moveAllColliders(sprite, pos.x - sprite->x, pos.y - sprite->y);
	sprite->x = pos.x;
	sprite->y = pos.y;
}

/*MOVE ALL APPLICABLE sprite COLLIDERS BY VELOCITY X & Y*/
void moveAllColliders(Sprite* sprite, int velX, int velY)
{
	if(sprite->collider.w != 0 || sprite->collider.h != 0){
		sprite->collider.x += velX;
		sprite->collider.y += velY;
	}
	
	if(sprite->boxColliders != NULL && sprite->nBoxColliders != 0){
		shiftBoxColliders(sprite, velX, velY);
	}

	if(sprite->circleCollider.r != 0){
		sprite->circleCollider.x += velX;
		sprite->circleCollider.y += velY;
	}
}

/*RENDER ALL sprite'S AVAILABLE COLLIDERS RELATIVE TO camera IF NOT NULL BY SHADES OF SPECIFIED color*/
void renderColliders(Sprite sprite, SDL_Rect* camera, SDL_Color color)
{
	SDL_Rect boxCollider;
	Circle circleCollider;
	SDL_Point cameraOffset = {0,0};
	int i;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	if(camera != NULL){
		cameraOffset.x = camera->x;
		cameraOffset.y = camera->y;
	}

	if(sprite.collider.w != 0 || sprite.collider.h != 0){
		boxCollider = sprite.collider;
		boxCollider.x -= cameraOffset.x;
		boxCollider.y -= cameraOffset.y;
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 30);
		SDL_RenderFillRect(renderer, &boxCollider);
	}

	for(i = 0; i < sprite.nBoxColliders; i++){
		boxCollider = sprite.boxColliders[i];
		boxCollider.x -= cameraOffset.x;
		boxCollider.y -= cameraOffset.y;
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 150);
		SDL_RenderFillRect(renderer, &boxCollider);
	}
	
	if(sprite.circleCollider.r != 0){
		circleCollider = sprite.circleCollider;
		circleCollider.x -= cameraOffset.x;
		circleCollider.y -= cameraOffset.y;
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
		SDL_RenderDrawLine(renderer, circleCollider.x, circleCollider.y, circleCollider.x + circleCollider.r, circleCollider.y);
	}
}

/*CHANGE sprite ANIMATION RECT BASED ON INTERNAL FRAME COUNT & delayFactor*/
void animate(Sprite* sprite, int delayFactor)
{
	int currentClipIndex = sprite->frame / (sprite->nClips*delayFactor);

	if(currentClipIndex >= sprite->nClips){
		sprite->frame = 0;
		currentClipIndex = sprite->nClips - 1;
	}

	setRenderRect(sprite, currentClipIndex);
}

/*SET sprite CURRENT RENDER RECT AND UPDATE INTERNAL SIZE*/
void setRenderRect(Sprite* sprite, int index)
{
	if(index < 0 || index >= sprite->nClips)
		return;
	
	sprite->renderRect = &sprite->clips[index];
	updateSpriteSize(sprite);
}

/*DELETE GIVEN SPRITE*/
void freeSprite(Sprite* sprite)
{
	SDL_free(sprite->clips);
	sprite->clips = NULL;
	sprite->renderRect = NULL;

	sprite->sheet = NULL;
}

/*PRINT SDL ERRS*/
void print_err(const char* msg)
{
	printf("%s, %s\n", msg, SDL_GetError());
}

/*GET SQUARED DISTANCE BETWEN POINTS (x1, y1) and (x2, y2)*/
int distanceSquared(int x1, int y1, int x2, int y2)
{
	int deltaX = x1 - x2;
	int deltaY = y1 - y2;

	return (deltaX * deltaX) + (deltaY * deltaY);
}

/*CHECK FOR ACTIVE COLLIDERS IN sprite*/
bool hasColliders(Sprite sprite)
{
	return sprite.collider.w != 0 || sprite.collider.h != 0 || (sprite.boxColliders != NULL && sprite.nBoxColliders != 0) || sprite.circleCollider.r != 0;
}

/*HANDLE PLAYER INPUT FOR PAC-MAN*/
void hanndlePacInput()
{
	const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);

	if(currentKeyStates[SDL_SCANCODE_UP]){
		pac.velY = -PAC_SPEED;
		pac.velX = 0;
		pac.angle = -90;
		pac.flip = SDL_FLIP_NONE;
	} 
	else if(currentKeyStates[SDL_SCANCODE_DOWN]){
		pac.velY = PAC_SPEED;
		pac.velX = 0;
		pac.angle = 90;
		pac.flip = SDL_FLIP_NONE;
	}
	else if(currentKeyStates[SDL_SCANCODE_LEFT]){
		pac.velX = -PAC_SPEED;
		pac.velY = 0;
		pac.angle = 0;
		pac.flip = SDL_FLIP_HORIZONTAL;
	}
	else if(currentKeyStates[SDL_SCANCODE_RIGHT]){
		pac.velX = PAC_SPEED;
		pac.velY = 0;
		pac.angle = 0;
		pac.flip = SDL_FLIP_NONE;
	}
	else{
		pac.velX = 0;
		pac.velY = 0;
	}
}

/*HANDLE TEXT INPUT (buffer) FOR PAC TEXTBOX*/
void handleTextInput(SDL_Event e)
{
	int textLen = strlen(pacTextBox.textBuffer);
	
	if(e.type == SDL_KEYDOWN){
		if(e.key.keysym.scancode == SDL_SCANCODE_BACKSPACE && textLen > 0){
			pacTextBox.textBuffer[textLen-1] = '\0';
			textSaved = false;
		}

		if(e.key.keysym.scancode == SDL_SCANCODE_RETURN && textLen > 0){
			pacTextBox.textBuffer[textLen] = SAVE_FILE_DELIMITER; //temp line separator for save file
			SDL_RWwrite(saveFile, pacTextBox.textBuffer, textLen+1, 1);
			pacTextBox.textBuffer[textLen] = '\0';
			textSaved = true;
		}

		return;
	}

	if(e.type == SDL_TEXTINPUT && textLen < TEXT_BOX_BUFFER_SIZE-1){
		strncat(pacTextBox.textBuffer, e.text.text, TEXT_BOX_BUFFER_SIZE-textLen);
		textSaved = false;
		return;
	}
}

/*HANDLE AUDIO RECORDING/PLAYBACK FOR PAC*/
void handleAudioInput()
{
	if(!pacAudioDevice.available){
		strncpy(pacTextBox.textBuffer, pacAudioDevice.name, TEXT_BOX_BUFFER_SIZE);
		return;
	}

	switch(pacAudioDevice.state){
	case PAUSED:
		if(pacRecorder.renderRect == &pacRecorder.clips[ON]) //if switch on
		{
			pacAudioDevice.bufferCurrentPos = 0;
			SDL_PauseAudioDevice(pacAudioDevice.recordingId, SDL_FALSE);
			pacAudioDevice.state = RECORDING;
		}
		break;
	case RECORDING:
		SDL_LockAudioDevice(pacAudioDevice.recordingId);

		if(pacAudioDevice.bufferCurrentPos > pacAudioDevice.bufferMaxPos){
			SDL_PauseAudioDevice(pacAudioDevice.recordingId, SDL_TRUE);
			pacAudioDevice.state = RECORDED;

			pacRecorder.renderRect = &pacRecorder.clips[OFF];
		}

		SDL_UnlockAudioDevice(pacAudioDevice.recordingId);
		break;
	case RECORDED:
		if(pacRecorder.renderRect == &pacRecorder.clips[ON]){
			pacAudioDevice.state = PAUSED;
		}
		else{
			if(textSaved){ //playback along saving text XD
				pacAudioDevice.bufferCurrentPos = 0;
				SDL_PauseAudioDevice(pacAudioDevice.playbackId, SDL_FALSE);
				pacAudioDevice.state = PLAYBACK;
			}
		}
		break;
	case PLAYBACK:
		SDL_LockAudioDevice(pacAudioDevice.playbackId);

		if(pacAudioDevice.bufferCurrentPos > pacAudioDevice.bufferMaxPos){
			SDL_PauseAudioDevice(pacAudioDevice.playbackId, SDL_TRUE);
			pacAudioDevice.state = RECORDED;
			textSaved = false;
		}

		SDL_UnlockAudioDevice(pacAudioDevice.playbackId);
		renderPacSoundWave();
		break;
	}
}

/*HANDLE WINDOW FOCUS AND SIZE EVENTS*/
void handleWindowEvents(SDL_Event e)
{
	if(e.type == SDL_WINDOWEVENT){
		switch(e.window.event)
		{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			camera->w = e.window.data1; //new width
			camera->h = e.window.data2; //new height
			break;
		default:
			break;
		}
	}
}

void switchRecorder(SDL_Event e)
{
	if(e.type == SDL_MOUSEBUTTONDOWN && pacAudioDevice.state != PLAYBACK)
	{
		int x, y;

		SDL_GetMouseState(&x, &y);
		x += camera->x;
		y += camera->y;

		//Recorder button pressed
		if((x > pacRecorder.x) && (x < pacRecorder.x + pacRecorder.w) && (y > pacRecorder.y) && (y < pacRecorder.y + pacRecorder.h)){
			setRenderRect(&pacRecorder, ON);
		}
	}
}

/*RENDER AND RESIZE textbox AND ITS COMPONENTS BASED ON INTERNAL TEXT BUFFER*/
void renderTextBox(Textbox* textbox)
{
	char line[TEXT_BOX_MAX_LINE_SIZE+1];
	int textLen = strlen(textbox->textBuffer);
	int nLines = (textLen/TEXT_BOX_MAX_LINE_SIZE)+1;
	int i, lineHeightOffset = textbox->textTexture.h*(nLines-1);

	textbox->y -= lineHeightOffset;
	textbox->h += lineHeightOffset;

	textbox->sprite.x = textbox->x;
	textbox->sprite.y = textbox->y;

	setScaleRect(&textbox->sprite, textbox->sprite.w, textbox->h);

	renderSprite(textbox->sprite, camera);

	//clear text texture
	SDL_DestroyTexture(textbox->textTexture.texture);
	textbox->textTexture.texture = NULL;

	if(nLines == 1) //no split needed
	{
		textbox->textTexture = loadRenderedText(textbox->textBuffer, textbox->textColor, textBoxFont);
		render(textbox->textTexture, textbox->x+10, textbox->y+10, NULL, NULL, 0, NULL, SDL_FLIP_NONE, camera);
	}
	else //split buffer lines
	{
		for(i = 0; i < nLines; i++)
		{
			strncpy(line, textbox->textBuffer+(i*TEXT_BOX_MAX_LINE_SIZE), TEXT_BOX_MAX_LINE_SIZE);
			line[TEXT_BOX_MAX_LINE_SIZE] = '\0';

			//clear temp text texture each loop
			SDL_DestroyTexture(textbox->textTexture.texture);
			textbox->textTexture.texture = NULL;

			textbox->textTexture = loadRenderedText(line, textbox->textColor, textBoxFont);
			render(textbox->textTexture, textbox->x+10, textbox->y+10+(i*textbox->textTexture.h), NULL, NULL, 0, NULL, SDL_FLIP_NONE, camera);
		}
	}
}

/*RENDER AND RESIZE PACMAN'S TEXTBOXES AND ITS COMPONENTS BASED ON INPUT BUFFER AND CURRENT SAVED TEXT STATE*/
void renderPacTextBoxes()
{
	int textLen = strlen(pacTextBox.textBuffer);

	pacTextBox.x = pac.x + (SHEET_STANDARD_SPRITE_SIZE/2);
	pacTextBox.y = pac.y - (SHEET_STANDARD_SPRITE_SIZE/2);
	pacTextBox.h = (SHEET_STANDARD_SPRITE_SIZE/2);
	pacTextBox.textColor = textSaved ? black : lightBlack;

	renderTextBox(&pacTextBox);

	textCursor.x = pacTextBox.x+10 + (textLen%TEXT_BOX_MAX_LINE_SIZE > 0 ? pacTextBox.textTexture.w : 0);
	textCursor.y = (pac.y - (SHEET_STANDARD_SPRITE_SIZE/2))+11;
	animate(&textCursor, 8);
	renderSprite(textCursor, camera);
	textCursor.frame++;

	//"saved" promt
	if(textSaved){
		savedPromptTextBox.x = pac.x - (SHEET_STANDARD_SPRITE_SIZE/2);
		savedPromptTextBox.y = pac.y - (SHEET_STANDARD_SPRITE_SIZE/2);
		renderTextBox(&savedPromptTextBox);
	}
}

/*RENDER PACMAN'S AUDIO RECORDING BUTTON*/
void renderPacRecorderButton()
{
	pacRecorder.x = pac.x - (SHEET_STANDARD_SPRITE_SIZE/2);
	pacRecorder.y = pac.y + (SHEET_STANDARD_SPRITE_SIZE - pacRecorder.h);

	renderSprite(pacRecorder, camera);
}

/*RENDER PACMAN'S SOUND WAVE ANIMATION FOR SOUND PLAYBACK*/
void renderPacSoundWave()
{
	if(pacAudioDevice.state == PLAYBACK){
		soundwave.x = pac.x + SHEET_STANDARD_SPRITE_SIZE;
		soundwave.y = pac.y + (SHEET_STANDARD_SPRITE_SIZE/2 - soundwave.h/2);

		animate(&soundwave, 8);
		renderSprite(soundwave, camera);
		soundwave.frame++;
	}
	else{
		soundwave.frame = 0;
		setRenderRect(&soundwave, FIRST_WAVE);
	}
}

/*MODIFY map'S INNER TILE SHEET TEXTURE TO INCLUDE SINE WAVE-ALIKE PIXELS*/
void addSineWaveTexture(TileMap* map, int startPeriod)
{
	Texture *tileTexture = map->sheet;
	SDL_PixelFormat *pixelFormat;
	Uint32 *pixels, *origPixels, greenPixel;
	int /*nPixels = 0,*/ i;

	if(!lockPixelTexture(tileTexture) || !lockPixelTexture(&tileSheetOrig))
		return;

	pixelFormat = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));
	pixels = (Uint32*)tileTexture->pixels;
	origPixels = (Uint32*)tileSheetOrig.pixels;
	//nPixels = (tileTexture->pitch / 4) * tileTexture->h;
	greenPixel = SDL_MapRGB(pixelFormat, green.r, green.g, green.b);
	//int end = (tileTexture->h*(tileTexture->h/2)) + ((tileTexture->pitch / 4)*5), 
	int x, y, j, index;

	for(x = 0, i = tileTexture->h*(tileTexture->h/2); x < (tileTexture->pitch / 4); x++){
		y = (int)(sin((double)((x + startPeriod)/5))*5) * (tileTexture->pitch / 4);

		for(j = 1; j <= 13; j++){
			index = (i-(5*(tileTexture->pitch / 4)))+(j*(tileTexture->pitch / 4));
			pixels[index+x] = origPixels[index+x];
		}

		for(j = 0; j < 5; j++){
			index = (i-y)+(j*(tileTexture->pitch / 4));
			pixels[index+x] = greenPixel;
		}
	}

	/*for(i = tileTexture->h*(tileTexture->h/2); i < end; i++){
		pixels[i] = greenPixel;
	}*/

	unlockPixelTexture(tileTexture);
	unlockPixelTexture(&tileSheetOrig);
	SDL_FreeFormat(pixelFormat);
}

/*RENDER AND RESIZE GHOSTS TEXTBOXES AND ITS COMPONENTS BASED ON INPUT BUFFER*/
void renderGhostsTextBoxes()
{
	blinkyTextBox.x = ghosts[BLINKY].x + (SHEET_STANDARD_SPRITE_SIZE/2);
	blinkyTextBox.y = ghosts[BLINKY].y - (SHEET_STANDARD_SPRITE_SIZE/2);
	blinkyTextBox.h = (SHEET_STANDARD_SPRITE_SIZE/2);

	renderTextBox(&blinkyTextBox);

	inkyTextBox.x = ghosts[INKY].x + (SHEET_STANDARD_SPRITE_SIZE/2);
	inkyTextBox.y = ghosts[INKY].y - (SHEET_STANDARD_SPRITE_SIZE/2);
	inkyTextBox.h = (SHEET_STANDARD_SPRITE_SIZE/2);

	renderTextBox(&inkyTextBox);
}

/*LOAD SEMI-PARTICLE SPARKLES ANIMATION SPRITE*/
Sprite loadSparklesSprite(SDL_Point initialPosition)
{
	Sprite spark;

	spark = loadSprite(N_SPARKLES_RENDERS, &sparklesSheet, initialPosition.x, initialPosition.y, 0, NULL, SDL_FLIP_NONE, NULL);
	
	addClip(&spark, SMALL_SPARK, (SDL_Rect){35, 27, 10, 10}, true);
	addClip(&spark, MEDIUM_SPARK, (SDL_Rect){68, 32, 14, 14}, false);
	addClip(&spark, BIG_SPARK, (SDL_Rect){8, 43, 20, 20}, false);

	spark.frame = rand()%N_SPARKLES_RENDERS;

	return spark;
}

/*RENDER SEMI-PARTICLE SPARKLES AS PACMAN'S "TRAIL"*/
void renderSparkles()
{
	int i;
	SDL_Point initialPos;

	if(pac.flip == SDL_FLIP_HORIZONTAL)
		initialPos = (SDL_Point){pac.x + pac.w, pac.y + (pac.h/2)};
	else if(pac.angle == -90)
		initialPos = (SDL_Point){pac.x + (pac.w/2), pac.y + pac.h};
	else if(pac.angle == 90)
		initialPos = (SDL_Point){pac.x + (pac.w/2), pac.y};
	else
		initialPos = (SDL_Point){pac.x, pac.y + (pac.h/2)};

	for(i = 0; i < N_SPARKLES_PARTICLES; i++)
	{
		if(rand()%10 == 0 || abs(sparkles[i].x - initialPos.x) > SHEET_STANDARD_SPRITE_SIZE || abs(sparkles[i].y - initialPos.y) > SHEET_STANDARD_SPRITE_SIZE){
			sparkles[i].x = initialPos.x - 50 + (rand()%100);
			sparkles[i].y = initialPos.y - 50 + (rand()%100);
			sparkles[i].frame = rand()%N_SPARKLES_RENDERS;
		}
		
		animate(&sparkles[i], 2);
		sparkles[i].frame++;

		renderSprite(sparkles[i], camera);
	}
}

/*ADJUST GHOSTS'S VELOCITY FOR RANDOM MOVEMENT*/
void randomizeGhostsVelocity()
{
	int i = 0;
	//Sprite testCollider;

	for(i = 0; i < N_GHOSTS; i++)
	{
		/*testCollider = ghosts[i];
		testCollider.collider.x -= 10;
		testCollider.collider.y -= 10;
		testCollider.collider.w += 20;
		testCollider.collider.h += 20;

		if(checkLevelBoundsCollision(testCollider) || checkTileMapCollisions(testCollider)){
			if(ghosts[i].velX != 0) ghosts[i].velX *= -1;
			if(ghosts[i].velY != 0) ghosts[i].velY *= -1;
		}*/

		if(rand()%60 == 1){
			ghosts[i].velX *= -1;
			ghosts[i].velY *= -1;
		}
	}
}

/*CENTER CAMERA RELATIVE TO PAC-MAN*/
void centerCamera()
{
	camera->x = (pac.x + SHEET_STANDARD_SPRITE_SIZE / 2) - camera->w / 2;
	camera->y = (pac.y + SHEET_STANDARD_SPRITE_SIZE / 2) - camera->h / 2;

	//Keep in level bounds
	if(camera->x < 0){
		camera->x = 0;
	}

	if(camera->y < 0){
		camera->y = 0;
	}

	if(camera->x + camera->w > LEVEL_WIDTH){
		camera->x = LEVEL_WIDTH - camera->w;
	}

	if(camera->y + camera->h > LEVEL_HEIGHT){
		camera->y = LEVEL_HEIGHT - camera->h;
	}
}