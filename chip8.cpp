#include <cstdint>
#include <string>
#include <string_view> //using for loadRom function
#include <fstream> // using for loadRom function
#include <iostream>
#include <chrono> 
#include <random> //for std::mt19937
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>


const unsigned int START_ADDRESS = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;

// seeding for our prng and seeding it only once ;
std::mt19937 mt { static_cast<std::mt19937::result_type>(
		std::chrono::steady_clock::now().time_since_epoch().count()
		) 
	};

// Class Declarations for Chip8, Platform(SDL)

class Chip8
{
public:
	uint8_t registers[16]{};
	uint8_t memory[4096]{};
	uint16_t index{}; // register to store memoery address of current instruction (addr of curr opcode); 
	uint16_t pc{}; // program counter , which would tell the next memory address of instuction 
	uint16_t stack[16]{}; //stack calls ,limited to 16
	uint8_t sp{}; // pointer cum index to the top of the stack 
	uint8_t delayTimer{};
	uint8_t soundTimer{};
	uint8_t keypad[16]{};
	uint32_t video[64 * 32]{};
	uint16_t opcode; 

	// defining prerequiste function 
	void loadROM(const std::string filename);
	unsigned int randomNum(std::mt19937& mt);

	// defining all the 34 instructions
	void OP_00E0 ();
	void OP_00EE();
	void OP_1nnn();
	void OP_2nnn();
	void OP_3xkk();
	void OP_4xkk();
	void OP_5xy0();
	void OP_6xkk();
	void OP_7xkk();
	void OP_8xy0();
	void OP_8xy1();
	void OP_8xy2();
	void OP_8xy3();
	void OP_8xy4(); 
	void OP_8xy5(); 
	void OP_8xy6(); 
	void OP_8xy7();
	void OP_8xyE();
	void OP_9xy0();
	void OP_Annn();
	void OP_Bnnn();
	void OP_Cxkk();
	void OP_Dxyn();
	void OP_Ex9E();
	void OP_ExA1();
	void OP_Fx07();
	void OP_Fx0A();
	void OP_Fx15();
	void OP_Fx18();
	void OP_Fx1E();
	void OP_Fx29();
	void OP_Fx33();
	void OP_Fx55();
	void OP_Fx65();
	
	// defining func pointer
	using Chip8Func = void(Chip8::*)(); 


	Chip8Func table[0xF+1]; // primary table
	Chip8Func table0[0xE +1]; // 0 table 
	Chip8Func table8[0xE +1]; // 8 table 
	Chip8Func tableE[0xE +1]; // E table (A1,9E);
	Chip8Func tableF[0x65+1]; // F table

	void Table0();
	void Table8();
	void TableE();
	void TableF();
	void OP_NULL();


	// defining cycle 
	void Cycle();
	
	
	 
	
	// definig ctor 
	Chip8();

};

class Platform{
	public:
		Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
		~Platform();

		void Update(void const* buffer, int pitch);
		bool ProcessInput(uint8_t* keys);

	
	private:
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* texture{};
};


// Chip8 fucntions
// implementing loadROM function 
void Chip8::loadROM(const std::string filename){
	
// openfile in binary mode, get all the number of characters  
	std::ifstream file {filename,std::ios::binary|std::ios::ate};
	if (file.is_open()){
 
		
		std::streampos size = file.tellg();

		if (size==-1){
			std::cerr<<"Tellg function failed\n";
			file.close();
			exit(-1);
		}
		
		char * buffer = new char[size];

		// seek to beginning 
		file.seekg(0); 

		file.read(buffer,size);
		file.close();

		for(int i=0;i<size;++i){
			memory[START_ADDRESS+i]=buffer[i];
		}
		delete [] buffer;
	}
	else{
		std::cerr<<"Couldnt open file\n"<<"Exiting Program..\n";
	}




	
}


// implementing randomNum function
unsigned int Chip8::randomNum(std::mt19937& mt = mt){
	std::uniform_int_distribution random{ 1, 255 };
	return random(mt);
}


// implementing 00E0 : CLS 
void Chip8::OP_00E0 (){
	std::cout<<"CLS was called\n";
	// clearing video buffer;
	std::fill( std::begin(this->video), std::end(this->video), 0);
}

// implementing 00EE: RET
void Chip8::OP_00EE(){

	this->pc = this->stack[--(this->sp)];
	

}

// implementing  1nnn: JP addr
void Chip8::OP_1nnn(){

	this->pc=this->opcode & 0x0FFFu;

}

	
// implementing 2nnn : CALL addr
void Chip8::OP_2nnn(){
	
	stack[sp]=pc;
	sp++;
	pc=this->opcode & 0x0FFFu;


}

// implementing 3xkk : Skipping instruction when Vx=kk;
void Chip8::OP_3xkk(){
	
	uint8_t x= (this->opcode & 0x0F00u) >> 8 ; 
	uint8_t kk = this->opcode & 0x00FFu; 
	
	if (this->registers[x]==kk){
		this->pc+=2;
	}


}

// implementing 4xkk : Skipping instruction when Vx!=kk; 
void Chip8::OP_4xkk(){
	uint8_t x = (this->opcode & 0x0F00) >> 8;
	uint8_t kk = (this->opcode & 0x00FF);

	if (this->registers[x]!= kk){
		this->pc+=2;
	}
}

// implementing 5xy0 :  skipping instruction when Vx=Vy 
void Chip8::OP_5xy0(){
	uint8_t x= (this->opcode & 0x0F00)>>8; //1111 0000 0000 
	uint8_t y=(this->opcode & 0x00F0)>>4;

	if (this->registers[x]==this->registers[y]){
		this->pc+=2;
	}
}

// implementing 6xkk : Vx=kk;
void Chip8::OP_6xkk(){
	uint8_t x = (this->opcode & 0x0F00)>>8;
	uint8_t kk = (this->opcode & 0x00FF);

	this->registers[x]=static_cast <int> (kk);
}

// implementing 7xkk: Vx+=kk;
void Chip8::OP_7xkk(){
	uint8_t x = (this->opcode & 0x0F00) >> 8;
	uint8_t kk = (this->opcode & 0x00FF);
	this->registers[x]+=kk;
}

// implemnting 8xy0: Vx=Vy
void Chip8::OP_8xy0(){
	uint8_t x= (this->opcode & 0x0F00)>>8; 
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[x]=this->registers[y];
		
	
}

// implemnting 8xy1: Vx=Vy|Vx
void Chip8::OP_8xy1(){
	uint8_t x= (this->opcode & 0x0F00)>>8; 
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[x]|=this->registers[y] ;
	
}

// implemnting 8xy2: Vx=Vy&Vx
void Chip8::OP_8xy2(){
	uint8_t x= (this->opcode & 0x0F00)>>8; 
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[x]&=this->registers[y];
	
}

// implemnting 8xy3: Vx=Vy ^ Vx
void Chip8::OP_8xy3(){
	uint8_t x= (this->opcode & 0x0F00)>>8; 
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[x]^=this->registers[y] ;
}

// implementing 8xy4 : Vx=Vx+Vy + carry in VF;
void Chip8::OP_8xy4(){
	uint8_t x= (this->opcode & 0x0F00)>>8;  
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[x]^=this->registers[y];
	this->registers[0xF]=this->registers[x]&registers[y];

	uint16_t sum = registers[x] + this->registers[y];

	uint8_t sum_8 = sum & 0x00FF;
	
	registers[0xF] = (sum>255)?1:0;
	this->registers[x] = sum_8; 

	


}

// implementing 8xy5 : Vx=Vx-vy + borrowing in VF;
void Chip8::OP_8xy5(){

	uint8_t x= (this->opcode & 0x0F00)>>8;  
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[0xF]=(x>y)?1:0;
	this->registers[x]-=this->registers[y];


}

// implementing 8xy6 : Vx .set(last)? VF + Vx>>1;
void Chip8::OP_8xy6(){
	uint8_t x= (this->opcode & 0x0F00)>>8;  
	
	uint8_t val = this->registers[x];
	this->registers[0xF]= val & 0x1;
	this->registers[x]=val>>1;

}

// implementing 8xy7 : Vx=Vy-Vx  and storing borrow or not in vf ;	
void Chip8::OP_8xy7(){
	uint8_t x= (this->opcode & 0x0F00)>>8;  
	uint8_t y=(this->opcode & 0x00F0)>>4;

	this->registers[0xF]=(y>x)?1:0;
	this->registers[x]=this->registers[y] - this->registers[x] ;

}

// implementing 8xyE : vx.set(first)?VF + Vx<<1
void Chip8::OP_8xyE(){
	uint8_t x = (this->opcode & 0x0F00) >> 8;

	this->registers[0xF]=((x & 0x80)>>7);
	this->registers[x]<<=1;
}

// implementing 9xy0 : Skip Instruction Vx!=Vy ;
void Chip8::OP_9xy0(){

	uint8_t x = (this->opcode & 0xF00)>>8;
	uint8_t y = (this->opcode & 0xF0)>>4;
	
	if (this->registers[x]!=this->registers[y]){
		this->pc+=2;
	}
	
}

// implementing Annn : set I =nnn ;
void Chip8::OP_Annn(){
	uint16_t nnn = (this->opcode & 0x0FFF);
	this->index=nnn;
}

// implementing Bnnn : jump call to *location* vo +nnn
void Chip8::OP_Bnnn(){
	uint16_t nnn = (this->opcode & 0x0FFF);

	this->pc=this->registers[0x0]+nnn;
} 

// implementing Cxkk : Vx= random byte AND it with kk
void Chip8::OP_Cxkk(){

	uint8_t x = (this->opcode & 0x0F00 )>>8;
	uint8_t kk = (this->opcode & 0xFF);

	this->registers[x]= this->randomNum() & kk;
}

// implementing Dxyn : I being the starting location I, drawing Vx,Vy set Vf= collision; 
void Chip8::OP_Dxyn(){

	uint8_t x = ( this->opcode & 0x0F00 )>>8;
	uint8_t y = ( this->opcode & 0x00F0 )>>4;
	uint8_t n = ( this->opcode & 0xF ); 

	uint8_t xPos = this->registers[x];
	uint8_t yPos = this->registers[y];

	this->registers[0xF]=0;
	
	for (int i=0;i<n;i++){
		uint8_t spritedata = this->memory[this->index + i];  
		for (int j=0; j<8; j++ ){
			uint8_t spritebit = spritedata & (0x80 >>j);

			uint32_t & screenpixel = video [(xPos+j)%(VIDEO_WIDTH)+((yPos+i)%(VIDEO_HEIGHT))*VIDEO_WIDTH];

			if(spritebit){

				if (screenpixel == 0xFFFFFFFF){	
					this->registers[0xF]=1; 	
				}

				if (screenpixel == 0xFFFFFFFF){
			
					screenpixel = 0xFF000000;
				}

				else{

					screenpixel = 0xFFFFFFFF;
				}  
			}
		}

	}

	

}

// implementing Ex9E() : Skip next instruction if key with value of Vx is pressed; 
void Chip8::OP_Ex9E(){
	uint8_t x = (this->opcode & 0x0F00 ) >> 8;
	
	
	uint8_t value = this->registers[x]&0x0F;
	if ( this->keypad[value]){
		this->pc+=2;
	}

} 

// implementing ExA1(): Skip next instruction if key with value of Vx is not pressed; 
void Chip8::OP_ExA1(){

	uint8_t x = (this->opcode & 0x0F00)>>8;
	uint8_t value = this->registers[x] & 0x0F;


	if (!this->keypad[value] ){
		this->pc+=2;

	}
	
}

// implementing Fx07 (): Vx= delay timer;
void Chip8:: OP_Fx07(){
	uint8_t x = (this->opcode & 0x0F00)>>8;

	this->registers[x]=this->delayTimer;
}

// implementing Fx0A() :when a keypad is deteted set value = vx'
void Chip8 :: OP_Fx0A(){
	uint8_t x= (this->opcode & 0x0F00)>>8;
	
	if (this->keypad[0]){
		this->registers[x] = 0;
	}

	else if (this->keypad[1]){
		this->registers[x]=1;
	}


	else if (this->keypad[2]){
		this->registers[x]=2;
	}


	else if (this->keypad[3]){
		this->registers[x]=3;
	}


	else if (this->keypad[4]){
		this->registers[x]=4;
	}


	else if (this->keypad[5]){
		this->registers[x]=5;
	}


	else if (this->keypad[6]){
		this->registers[x]=6;
	}


	else if (this->keypad[7]){
		this->registers[x]=7;
	}


	else if (this->keypad[8]){
		this->registers[x]=8;
	}


	else if (this->keypad[9]){
		this->registers[x]=9;
	}


	else if (this->keypad[10]){
		this->registers[x]=10;
	}


	else if (this->keypad[11]){
		this->registers[x]=11;
	}


	else if (this->keypad[12]){
		this->registers[x]=12;
	}


	else if (this->keypad[13]){
		this->registers[x]=13;
	}


	else if (this->keypad[14]){
		this->registers[x]=14;
	}


	else if (this->keypad[15]){
		this->registers[x]=15;
	}

	else {
		pc-=2;
	}

}

// implementing Fx15(): set delay timer as Vx
void Chip8 :: OP_Fx15(){
	uint8_t x = (this->opcode & 0x0F00)>>8;

	this->delayTimer=this->registers[x]; 
}

// implementing Fx18(): set sound timer as Vx
void Chip8 :: OP_Fx18(){
	uint8_t x = (this->opcode & 0x0F00)>>8;

	this->soundTimer=this->registers[x];
}

// implementing Fx1E(): add I+ vx 
void Chip8 :: OP_Fx1E(){
	
	uint8_t x = (this->opcode & 0x0F00 ) >> 8;

	this->index += this->registers[x];
}

// implementing Fx29() : Set I = location of sprite for digit Vx.
void Chip8 :: OP_Fx29(){
	uint8_t x = (this->opcode & 0x0F00)>>8;
	uint8_t val = this->registers[x];

	this->index = FONTSET_START_ADDRESS + 5*val ;
	

}

// impelmenting Fx33(): BCD of VX in I,I+1,I+2;
void Chip8 :: OP_Fx33(){

	uint8_t x = ( this->opcode & 0x0F00) >> 8;
	uint8_t val = this->registers[x];
	
	for (int i=2;i>=0;i--){
		int curr_digit = val%10;
		memory[this->index+i] = curr_digit;
		val/=10; 
	}

}

// implementing Fx55(): Store registers V0 through Vx in memory starting at location I.
void Chip8 :: OP_Fx55(){

	uint8_t x = (this->opcode & 0x0F00 )>>8;

	for (uint8_t i=0;i<=x;i++){
		memory[index+i]= this->registers[i];
	}

}

// implementing Fx65() : Read registers V0 through Vx from memory starting at location I
void Chip8 :: OP_Fx65(){

	uint8_t x = ( this->opcode & 0x0F00 )>>8;

	for (uint8_t i=0 ; i<=x; i++){
		this->registers[i]= this->memory[this->index +i ];
	}
}

// implementing Table0 fucntion 
void Chip8 :: Table0(){
		(this->*(table0[opcode & 0x000F]))();
	}

//implementign Table8 fucntion k
void Chip8 :: Table8(){
	(this->*(table8[opcode&0x000F]))();
} 

// implememitng Table E funciton 
void Chip8 :: TableE(){
	(this->*(tableE[opcode&0x000F]))();

}

// implementing Table F function 
void Chip8 :: TableF(){
	(this->*(tableF[opcode&0x00FF]))();
}

// implementing OP_NULL
void Chip8:: OP_NULL(){
	std::cout<<"OP_NUll was called\n";
}

// impelmenting cycle 
void Chip8::Cycle(){
	opcode= memory[pc]<<8 | memory[pc+1];
	// get index 
	uint16_t op_index = (opcode & 0xF000u)>>12;
	
	pc+=2;
	(this->*(table[op_index]))();
	
	if (delayTimer>0){
		delayTimer--;
	}
	if (soundTimer>0){
		soundTimer--;
	}
	
}


// implementing ctor 
Chip8::Chip8(){

	this->pc=START_ADDRESS;
	uint8_t fontset[FONTSET_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0xA0, 0x20, 0xF0, // 1
	0xE0, 0x10, 0x20, 0x80, 0xF0, // 2
	0xF0, 0x10, 0x70, 0x10, 0xF0, // 3
	0x20, 0x60, 0xF0, 0x20, 0x20, // 4
	0xF0, 0x80, 0xD0, 0x20, 0xA0, // 5
	0xF0, 0x80, 0xE0, 0xA0, 0xE0, // 6
	0xF0, 0x10, 0xF0, 0x40, 0x80, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xF0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xE0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F

};

	for(int i=0;i<FONTSET_SIZE;++i){
		memory[FONTSET_START_ADDRESS+i]=fontset[i];
	}


		table[0x0] = &Table0;
		table[0x1] = &OP_1nnn;
		table[0x2] = &OP_2nnn;
		table[0x3] = &OP_3xkk;
		table[0x4] = &OP_4xkk;
		table[0x5] = &OP_5xy0;
		table[0x6] = &OP_6xkk;
		table[0x7] = &OP_7xkk;
		table[0x8] = &Table8;
		table[0x9] = &OP_9xy0;
		table[0xA] = &OP_Annn;
		table[0xB] = &OP_Bnnn;
		table[0xC] = &OP_Cxkk;
		table[0xD] = &OP_Dxyn;
		table[0xE] = &TableE;
		table[0xF] = &TableF;

		// initilaising table 0,8,E to all null ptr 

		for (uint16_t i =0;i<=0xE;i++){
			table0[i]=&OP_NULL;
			table8[i]=&OP_NULL;
			tableE[i]=&OP_NULL;
		}
		table0[0x0]=&OP_00E0;
		table0[0xE]=&OP_00EE;

		table8[0x0] = &OP_8xy0;
		table8[0x1] = &OP_8xy1;
		table8[0x2] = &OP_8xy2;
		table8[0x3] = &OP_8xy3;
		table8[0x4] = &OP_8xy4;
		table8[0x5] = &OP_8xy5;
		table8[0x6] = &OP_8xy6;
		table8[0x7] = &OP_8xy7;
		table8[0xE] = &OP_8xyE;

		tableE[0x1] = &OP_ExA1;
		tableE[0xE] = &OP_Ex9E;

		for (int i =0; i <=0x65;i++){
			tableF[i]=&OP_NULL;
		}

		tableF[0x07] = &OP_Fx07;
		tableF[0x0A] = &OP_Fx0A;
		tableF[0x15] = &OP_Fx15;
		tableF[0x18] = &OP_Fx18;
		tableF[0x1E] = &OP_Fx1E;
		tableF[0x29] = &OP_Fx29;
		tableF[0x33] = &OP_Fx33;
		tableF[0x55] = &OP_Fx55;
		tableF[0x65] = &OP_Fx65;
		


}


// implementing ctor
Platform::Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight){

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(title,windowWidth,windowHeight,0);
	renderer=SDL_CreateRenderer(window,nullptr);
	texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,textureWidth,textureHeight);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	
}

// calling dtor
Platform::~Platform(){
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
}

// implementing Update function
void Platform::Update(void const* buffer, int pitch){
	SDL_UpdateTexture(texture, nullptr, buffer, pitch); //nullptr to select the whole texture together
	SDL_RenderClear(renderer); 
	SDL_RenderTexture(renderer,texture,nullptr,nullptr); // rendercopy -> rendertexture
	SDL_RenderPresent(renderer);


}

// implementing Input processing funciton (understood and migrated using documentation, mosly copied though )
bool Platform::ProcessInput(uint8_t* keys)
	{
		bool quit = false;

		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
				{
					quit = true;
				} break;

				case SDL_EVENT_KEY_DOWN:
				{
					switch (event.key.key)
					{
						case SDLK_ESCAPE:
						{
							quit = true;
						} break;

						case SDLK_X:
						{
							keys[0] = 1;
						} break;

						case SDLK_1:
						{
							keys[1] = 1;
						} break;

						case SDLK_2:
						{
							keys[2] = 1;
						} break;

						case SDLK_3:
						{
							keys[3] = 1;
						} break;

						case SDLK_Q:
						{
							keys[4] = 1;
						} break;

						case SDLK_W:
						{
							keys[5] = 1;
						} break;

						case SDLK_E:
						{
							keys[6] = 1;
						} break;

						case SDLK_A:
						{
							keys[7] = 1;
						} break;

						case SDLK_S:
						{
							keys[8] = 1;
						} break;

						case SDLK_D:
						{
							keys[9] = 1;
						} break;

						case SDLK_Z:
						{
							keys[0xA] = 1;
						} break;

						case SDLK_C:
						{
							keys[0xB] = 1;
						} break;

						case SDLK_4:
						{
							keys[0xC] = 1;
						} break;

						case SDLK_R:
						{
							keys[0xD] = 1;
						} break;

						case SDLK_F:
						{
							keys[0xE] = 1;
						} break;

						case SDLK_V:
						{
							keys[0xF] = 1;
						} break;
					}
				} break;

				case SDL_EVENT_KEY_UP:
				{
					switch (event.key.key)
					{
						case SDLK_X:
						{
							keys[0] = 0;
						} break;

						case SDLK_1:
						{
							keys[1] = 0;
						} break;

						case SDLK_2:
						{
							keys[2] = 0;
						} break;

						case SDLK_3:
						{
							keys[3] = 0;
						} break;

						case SDLK_Q:
						{
							keys[4] = 0;
						} break;

						case SDLK_W:
						{
							keys[5] = 0;
						} break;

						case SDLK_E:
						{
							keys[6] = 0;
						} break;

						case SDLK_A:
						{
							keys[7] = 0;
						} break;

						case SDLK_S:
						{
							keys[8] = 0;
						} break;

						case SDLK_D:
						{
							keys[9] = 0;
						} break;

						case SDLK_Z:
						{
							keys[0xA] = 0;
						} break;

						case SDLK_C:
						{
							keys[0xB] = 0;
						} break;

						case SDLK_4:
						{
							keys[0xC] = 0;
						} break;

						case SDLK_R:
						{
							keys[0xD] = 0;
						} break;

						case SDLK_F:
						{
							keys[0xE] = 0;
						} break;

						case SDLK_V:
						{
							keys[0xF] = 0;
						} break;
					}
				} break;
			}
		}

		return quit;
	}

// main function 

int main(int argc, char* argv[]){

	if (argc!=4){
		std::cerr<<"Invalid Usage\nCorrect Usage:"<<argv[0]<<" <VideoScaling> <CycleDelay> <ROMFile>\n";
		std::exit(EXIT_FAILURE);
	}

	int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFilename = argv[3];

	// creating platform 
	Platform platform("CHIP-8 Emulator", (VIDEO_WIDTH * videoScale), (VIDEO_HEIGHT * videoScale), VIDEO_WIDTH, VIDEO_HEIGHT);

	// creating chip8 interface
	Chip8 chip8;
	chip8.loadROM(romFilename);
	
	// getting videopitch (just so as to comply with sdl library) not required by us in this simple programm
	int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

	// getting the clocks out
	auto lastCycleTime = std::chrono::high_resolution_clock::now();

	bool quit=false;


	while(!quit){
		quit = platform.ProcessInput(chip8.keypad);
		auto currentTime = std::chrono::high_resolution_clock::now();
		
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();// getting time differene to check cycle spped
		
		
		if (dt > cycleDelay){
			lastCycleTime = currentTime;
			chip8.Cycle();
			platform.Update(chip8.video, videoPitch);
			
		}
	

	}

	return 0; 




}