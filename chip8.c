#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>


#define FREQUENTY_SCREEN (1.0f/60.0f)
#define FREQUENTY_INSTRUCTIONS (1.0f/700.0f)
#define FREQUENTY_TIMERS (1.0f/60.0f)
#define CHARACTER_SET_START_POSITION 0x050
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_SCREEN_WIDTH 64

uint8_t memory[4096] = {0};
SDL_Window* window = NULL;
SDL_Renderer* gRenderer = NULL;
clock_t startTimeScreen;
clock_t startTimeInstructions;
clock_t startTimeTimers;
int quitProgram = 0;
const uint8_t *keystate;

int SCREEN_HEIGHT = 320;
int SCREEN_WIDTH = 640;


char screenBuffer[CHIP8_SCREEN_WIDTH][CHIP8_SCREEN_HEIGHT] ={};


struct Registers{
	uint8_t V[16]; 	//Dataregister
	uint16_t SS[16];	//Stack segment
	uint16_t I; 		//index register
	uint8_t DT;		//delay timer
	uint8_t ST;		//Sound timer
	uint16_t PC;		//Program counter
	uint8_t SP;		//Stack pointer
};

struct Registers registersInstance = {{},{},0,0,0,0x200,0};

struct DecodedInstruction{
	uint8_t X : 4; // second nibble
	uint8_t Y : 4; // third nibble
	uint8_t N : 4; // last nibble
	uint8_t NN; // second byte
	uint16_t NNN : 12; // last 3 nibbles
};

uint8_t characterSet[80] = {
	0xF0,0x90,0x90,0x90,0xF0,	// 0
	0x20,0x60,0x20,0x20,0x70,  	// 1
	0xF0,0x10,0xF0,0x80,0xF0,	// 2
	0xF0,0x10,0xF0,0x10,0xF0,	// 3
	0x90,0x90,0xF0,0x10,0x10, 	// 4
	0xF0,0x80,0xF0,0x10,0xF0,	// 5
	0xF0,0x80,0xF0,0x90,0xF0,	// 6
	0xF0,0x10,0x20,0x40,0x40,	// 7
	0xF0,0x90,0xF0,0x90,0xF0,	// 8
	0xF0,0x90,0xF0,0x10,0xF0,	// 9
	0xF0,0x90,0xF0,0x90,0x90,	// A
	0xE0,0x90,0xE0,0x90,0xE0,	// B
	0xF0,0x80,0x80,0x80,0xF0,	// C
	0xE0,0x90,0x90,0x90,0xE0,	// D
	0xF0,0x80,0xF0,0x80,0xF0,	// E
	0xF0,0x80,0xF0,0x80,0x80,	// F
};

void writeCharactersetToMemory(){
	for(int i=0;i<=80;i++){
		memory[CHARACTER_SET_START_POSITION + i] = characterSet[i];
	}
}

void initScreen(){
	startTimeScreen = clock();
	// Attempt to initialize graphics and timer system.
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
        //return 1;
    }

    window = SDL_CreateWindow("chip8 on 🍎",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                    	SCREEN_WIDTH,SCREEN_HEIGHT, 0);
    if (!window)
    {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        //return 1;
    }
	gRenderer = SDL_CreateRenderer( window	, -1, SDL_RENDERER_ACCELERATED );
	if( gRenderer == NULL )
	{
		printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
	}

	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
	SDL_RenderClear( gRenderer );
	SDL_RenderPresent( gRenderer );

}

void removeScreenResources(){
	 // clean up resources before exiting.
	 SDL_DestroyRenderer( gRenderer );
	 SDL_DestroyWindow(window);
	 SDL_Quit();
}

void handleKeyInput(){
	SDL_Event e;
	if (SDL_PollEvent(&e) != 0 && e.type == SDL_QUIT){
		quitProgram = 1;
	}

}



void drawPixel(int x, int y){
	int pixelSizeX = SCREEN_WIDTH / CHIP8_SCREEN_WIDTH;
	int pixelSizeY = SCREEN_HEIGHT / CHIP8_SCREEN_HEIGHT;

	int inGameX = x * pixelSizeX;
	int inGameY = y * pixelSizeY;
	
	printf("location of pixel x: %d  y: %d  - ingame: x: %d  y:%d\n", x,y,inGameX,inGameY);

	SDL_Rect fillRect = { inGameX,inGameY,pixelSizeX,pixelSizeY};       
	SDL_RenderFillRect( gRenderer, &fillRect );

}

void drawPixelState(int x, int y, char pixelState){
	int pixelSizeX = SCREEN_WIDTH / CHIP8_SCREEN_WIDTH;
	int pixelSizeY = SCREEN_HEIGHT / CHIP8_SCREEN_HEIGHT;

	int inGameX = x * pixelSizeX;
	int inGameY = y * pixelSizeY;

	if(pixelState == 1){
		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF );
		//printf("location of pixel x: %d  y: %d  - ingame: x: %d  y:%d - pixel state ON \n" , x,y,inGameX,inGameY);
	}
	else{
		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
		//printf("location of pixel x: %d  y: %d  - ingame: x: %d  y:%d - pixel state OFF \n" , x,y,inGameX,inGameY);
	}

	SDL_Rect fillRect = { inGameX,inGameY,pixelSizeX,pixelSizeY};       
	SDL_RenderFillRect( gRenderer, &fillRect );

}

void drawSprite(int x, int y, uint8_t sprite[], size_t spriteSize){
	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF );
	for(int i = 0; i< spriteSize;i++){
		uint8_t spriteLine = sprite[i];
		for(int j=7;j>=0;j--){
			uint8_t pixel = (spriteLine >> (j)) & 1;
			int drawX = x + (7-j); 
			int drawY = y + i;
			if(pixel){
				drawPixel(drawX,drawY);
			}
		}
	}
}

void binprintf(uint8_t v)
{
    uint8_t mask=1<<((sizeof(uint8_t)<<3)-1);
    while(mask) {
        printf("%d", (v&mask ? 1 : 0));
        mask >>= 1;
    }
}
void updateScreenBuffer(int x, int y, uint16_t location, uint8_t spriteSize){
	registersInstance.V[0xF] = 0;
	for(int i = 0; i< spriteSize;i++){
		uint8_t spriteLine = memory[location + i];
		//binprintf(spriteLine);
		//printf("\n");
		for(int j=7;j>=0;j--){
			char pixel = (spriteLine >> (j)) & 1;
			int drawX = x + (7-j); // add the location of the bit we are processing
			int drawY = y + i;
			char currentPixel = screenBuffer[drawX][drawY];
			if(currentPixel != pixel){
				registersInstance.V[0xF] = 1;
			}
			screenBuffer[drawX][drawY]=pixel;
		}
	}
}

void drawScreen(){
	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
	SDL_RenderClear( gRenderer );
	for(int y=0; y<CHIP8_SCREEN_HEIGHT; y++){
		//printf("\n" );
		for(int x =0; x< CHIP8_SCREEN_WIDTH; x++){
			char pixel = screenBuffer[x][y];
			//printf("%d " , pixel);
			drawPixelState(x,y,pixel);
		}
	}
	SDL_RenderPresent( gRenderer );
	//printf("\n" );
	//printf("\n" );

}

void drawCharacterSet(){
	for(int i = 0; i< 16;i++){
		int drawX= i * 5;
		int drawY= 0;
		if(i >=8){
			drawX = drawX - 40;
			drawY = 7;
		}
		printf("sprite location x: %d and y: %d \n", drawX,drawY);
		uint8_t sprite[5];
		for(int j=0;j<5;j++){
			sprite[j] = characterSet[(i*5)+j];
		}
		
		drawSprite(drawX, drawY,sprite,5);
		
		
	}
}


void handleScreenRefresh(){
	float deltaTime;
    clock_t timePassed;
	timePassed = clock() - startTimeScreen;
	deltaTime =  (float) timePassed / CLOCKS_PER_SEC;
	if(deltaTime >= FREQUENTY_SCREEN ){
		printf("Value deltatime for screen= %f\n", deltaTime);
		
		// handle screen logic
		//Clear screen
		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
		SDL_RenderClear( gRenderer );

		drawPixel(1 , 1);
		uint8_t sprite0[5]= {0xF0,0x90,0x90,0x90,0xF0};
	//	drawSprite(10,10,sprite0,5);
		drawCharacterSet();	
		SDL_RenderPresent( gRenderer );
		deltaTime = 0.0f;
		startTimeScreen = clock();
	}
        
}

struct DecodedInstruction decode(uint16_t instruction){
	struct DecodedInstruction decodedInstruction = {
		(instruction & 0x0F00) >> 8,
		(instruction & 0x00F0) >> 4 ,
		instruction & 0x000F,
		instruction & 0x00FF,
		instruction & 0x0FFF,
	};
	return decodedInstruction;
}

void addPcToStack(){
	registersInstance.SP++;
	registersInstance.SS[registersInstance.SP] = registersInstance.PC;
}

void setPcFromStack(){
	registersInstance.PC = registersInstance.SS[registersInstance.SP];
	registersInstance.SP--;
}

int8_t getKeyboardInput(){
	//first row
	if ( keystate[SDL_SCANCODE_1] ) return 0;
	if ( keystate[SDL_SCANCODE_2] ) return 1;
	if ( keystate[SDL_SCANCODE_3] ) return 2;
	if ( keystate[SDL_SCANCODE_4] ) return 3;
	//second row
	if ( keystate[SDL_SCANCODE_Q] ) return 4;
	if ( keystate[SDL_SCANCODE_W] ) return 5;
	if ( keystate[SDL_SCANCODE_E] ) return 6;
	if ( keystate[SDL_SCANCODE_R] ) return 7;
	//third row
	if ( keystate[SDL_SCANCODE_A] ) return 8;
	if ( keystate[SDL_SCANCODE_S] ) return 9;
	if ( keystate[SDL_SCANCODE_D] ) return 10;
	if ( keystate[SDL_SCANCODE_F] ) return 11;
	//fourth row
	if ( keystate[SDL_SCANCODE_Z] ) return 12;
	if ( keystate[SDL_SCANCODE_X] ) return 13;
	if ( keystate[SDL_SCANCODE_C] ) return 14;
	if ( keystate[SDL_SCANCODE_V] ) return 15;
	return -1;
}

void handle0x8XY0(struct DecodedInstruction decodedInstruction){
	switch(decodedInstruction.N){
		case 0x0:
			registersInstance.V[decodedInstruction.X]  = registersInstance.V[decodedInstruction.Y];
			break;
		case 0x1:
			registersInstance.V[decodedInstruction.X]  = (registersInstance.V[decodedInstruction.X] | registersInstance.V[decodedInstruction.Y]);
			break;
		case 0x2:
			registersInstance.V[decodedInstruction.X]  = (registersInstance.V[decodedInstruction.X] & registersInstance.V[decodedInstruction.Y]);
			break;
		case 0x3:
			registersInstance.V[decodedInstruction.X]  = (registersInstance.V[decodedInstruction.X] ^ registersInstance.V[decodedInstruction.Y]);
			break;
		case 0x4:
			;uint8_t result = registersInstance.V[decodedInstruction.X] + registersInstance.V[decodedInstruction.Y];
			registersInstance.V[decodedInstruction.X]  = result;
			if(result < registersInstance.V[decodedInstruction.X] && result < registersInstance.V[decodedInstruction.Y]){
				registersInstance.V[0xF] = 1;
			}
			else{
				registersInstance.V[0xF] = 0;
			}
			break;
		case 0x5:
			if(registersInstance.V[decodedInstruction.X] > registersInstance.V[decodedInstruction.Y]){
				registersInstance.V[0xF] = 1;
			}
			else{
				registersInstance.V[0xF] = 0;
			}
			registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.X] - registersInstance.V[decodedInstruction.Y];
			break;
		case 0x6:
			//this is optional, depending on implementation
			//registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.Y];

			registersInstance.V[0xF] = registersInstance.V[decodedInstruction.X] & 0x01;
			registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.X] >> 1; 
			break;
		case 0x7:
			if(registersInstance.V[decodedInstruction.Y] > registersInstance.V[decodedInstruction.X]){
				registersInstance.V[0xF] = 1;
			}
			else{
				registersInstance.V[0xF] = 0;
			}
			registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.Y] - registersInstance.V[decodedInstruction.X];
			break;
		case 0xE: 
			//this is optional, depending on implementation
			//registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.Y];
			registersInstance.V[0xF] = (registersInstance.V[decodedInstruction.X] & 0x80) >> 7;

			registersInstance.V[decodedInstruction.X] = registersInstance.V[decodedInstruction.X] << 1;

			break;
	}
}

void handleInstruction(){
	float deltaTime;
    clock_t timePassed;
	timePassed = clock() - startTimeInstructions;
	deltaTime =  (float) timePassed / CLOCKS_PER_SEC;
	if(deltaTime >= FREQUENTY_INSTRUCTIONS ){
		//printf("Value deltatime for instruction= %f\n", deltaTime);
		// handle instruction logic
		//fetch
		uint8_t part1Instruction = memory[registersInstance.PC];
		registersInstance.PC++; // increase PC by one
		uint8_t part2Instruction = memory[registersInstance.PC];
		registersInstance.PC++; // increase PC again, 2 times in total

		uint16_t fullInstruction = part1Instruction << 8 | part2Instruction;

		//decode
		struct DecodedInstruction decodedInstruction = decode(fullInstruction);

		//execute
		//printf("executing instruction: %04x \n",fullInstruction);
		// printf("instruction after mask: %04x \n",fullInstruction & 0xF000);
		// printf("X: %04x \n",decodedInstruction.X);
		// printf("Y: %04x \n",decodedInstruction.Y);
		// printf("N: %04x \n",decodedInstruction.N);
		// printf("NN: %04x \n",decodedInstruction.NN);
		// printf("NNN: %04x \n",decodedInstruction.NNN);

		switch(fullInstruction >> 12){
			case 0x0:
				if(fullInstruction == 0x00E0)	{
					printf("clear screen 0x1\n");
					memset(screenBuffer, 0, sizeof(screenBuffer[0][0]) * CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH);
					drawScreen();
				}
				else if(fullInstruction == 0x00EE){
					setPcFromStack();
				}
				break;
			case 0x1:
				///printf("0x1\n");	
				registersInstance.PC = decodedInstruction.NNN;
				break;
			case 0x2:
				addPcToStack();
				registersInstance.PC = decodedInstruction.NNN;
				break;
			case 0x3:
				if (registersInstance.V[decodedInstruction.X]  == decodedInstruction.NN){
					registersInstance.PC+=2;
				};
				break;
			case 0x4:
				if (registersInstance.V[decodedInstruction.X]  != decodedInstruction.NN){
					registersInstance.PC+=2;
				};
				break;
			case 0x5:
				if(decodedInstruction.N == 0){
					if (registersInstance.V[decodedInstruction.X]  == registersInstance.V[decodedInstruction.Y]){
						registersInstance.PC+=2;
					}
				}
				break;
			case 0x6:
				//printf("0x6\n");	
				registersInstance.V[decodedInstruction.X] = decodedInstruction.NN;
				break;
			case 0x7:
				//printf("0x7\n");	
				registersInstance.V[decodedInstruction.X] += decodedInstruction.NN;
				break;
			case 0x8:
				handle0x8XY0(decodedInstruction);
				break;
			case 0x9:
				if(decodedInstruction.N == 0){
					if (registersInstance.V[decodedInstruction.X]  != registersInstance.V[decodedInstruction.Y]){
						registersInstance.PC+=2;
					}
				}
				break;
			case 0xA:
				//printf("0xA\n");	
				//printf("executing instruction: %04x with 3 last nibles: %04x\n",fullInstruction,decodedInstruction.NNN);

				registersInstance.I = decodedInstruction.NNN;
				break;
			case 0xB:
				//Old implementation: should take presedence 
				registersInstance.PC = decodedInstruction.NNN + registersInstance.V[0];
				// chip-48/super-chip implementation
				//registersInstance.PC = decodedInstruction.NNN + registersInstance[decodedInstruction.X];
				break;
			case 0xC:
				;uint8_t random = rand();
				registersInstance.V[decodedInstruction.X] = random & decodedInstruction.NN;
			case 0xD:
				;
				int x = registersInstance.V[decodedInstruction.X];
				int y = registersInstance.V[decodedInstruction.Y];
				printf("draw sprite at location %d,%d at memory %04x with height %d \n" , x,y,registersInstance.I,decodedInstruction.N);
				printf("executing instruction: %04x \n",fullInstruction);
				printf("instruction after mask: %04x \n",fullInstruction & 0xF000);
				printf("X: %04x \n",decodedInstruction.X);
				printf("Y: %04x \n",decodedInstruction.Y);
				printf("N: %04x \n",decodedInstruction.N);
				printf("NN: %04x \n",decodedInstruction.NN);
				printf("NNN: %04x \n",decodedInstruction.NNN);
				updateScreenBuffer(x,y,registersInstance.I,decodedInstruction.N);
				drawScreen();
				break;
			case 0xE:
				;int8_t pressedKey = getKeyboardInput();
				switch(decodedInstruction.NN){
					case 0x9E:
						if(pressedKey == registersInstance.V[decodedInstruction.X]) {
							registersInstance.PC += 2;
						}
						break;
					case 0xA1:
						if(pressedKey != registersInstance.V[decodedInstruction.X]) {
							registersInstance.PC += 2;
						}
						break;
				}
				break;
			case 0xF:
				switch(decodedInstruction.NN){
					case 0x07:
						registersInstance.V[decodedInstruction.X] = registersInstance.DT;
						break;
					case 0x15:
					 	registersInstance.DT = registersInstance.V[decodedInstruction.X];
						break;
					case 0x18:
						registersInstance.ST = registersInstance.V[decodedInstruction.X];
						break;
					case 0x1E:
						registersInstance.I += registersInstance.V[decodedInstruction.X];
						break;
					case 0x0A:
						; int8_t keyPressed = getKeyboardInput();
						if(keyPressed == -1){
							registersInstance.PC -= 2;
						}
						else{
							registersInstance.V[decodedInstruction.X] = keyPressed;	
						}
						break;
					case 0x29:
						;uint8_t characterToFetch = registersInstance.V[decodedInstruction.X];
						registersInstance.I = characterToFetch + CHARACTER_SET_START_POSITION;
						break;
					case 0x33:
						;uint8_t number = registersInstance.V[decodedInstruction.X];
						uint8_t digit1 = number % 10;
						memory[registersInstance.I+2] = digit1;
						uint8_t digit2 = (number / 10) % 10;
						memory[registersInstance.I + 1] = digit2;
						uint8_t digit3 = (number / 100) % 10;
						memory[registersInstance.I] = digit3;
						break;
					case 0x55:
						for(int i = 0; i<= decodedInstruction.X ; i++){
							memory[registersInstance.I + i] = registersInstance.V[i];
						}
						break;
					case 0x65:
						for(int i = 0; i<= decodedInstruction.X ; i++){
							registersInstance.V[i] = memory[registersInstance.I + i];
						}
						break;
				}
				break;
		}


		deltaTime = 0.0f;
		startTimeInstructions = clock();
	}
}

void handleTimers(){
	float deltaTime;
    clock_t timePassed;
	timePassed = clock() - startTimeInstructions;
	deltaTime =  (float) timePassed / CLOCKS_PER_SEC;
	if(deltaTime >= FREQUENTY_TIMERS ){

		if(registersInstance.DT > 0) registersInstance.DT --;
		if(registersInstance.ST > 0) registersInstance.ST --;

		deltaTime = 0.0f;
		startTimeInstructions = clock();
	}
}

void readRom(){
	FILE *fp;
	unsigned char c;

	fp = fopen("roms/test_opcode.ch8","rb");
	int counter = 0;
	while(fread(&c, sizeof(char), 1, fp) > 0){
		memory[0x200 + counter] = c;
		counter++; 
	}

	fclose(fp);
}


int main(void){
	keystate =SDL_GetKeyboardState(NULL);
	initScreen();
	writeCharactersetToMemory();
	readRom();
	srand(time(NULL));
	startTimeInstructions = clock();
	startTimeTimers = clock();
	while(quitProgram == 0){
		//handleScreenRefresh();
		handleKeyInput();
		handleInstruction();
		handleTimers();
	}
	
	removeScreenResources();
}
