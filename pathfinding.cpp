#include <SDL2/SDL.h>
#include <cassert>
#include <cstdio>
#include <cmath>

#define TILE_SIZE 20

const int map1[] = {
	8, 11,
	1,1,1,1,1,1,1,1,
	1,0,1,1,1,0,0,1,
	1,0,0,1,0,0,0,1,
	1,1,0,1,1,0,1,1,
	1,1,0,0,0,0,1,1,
	1,1,1,1,0,1,1,1,
	1,0,0,0,0,0,0,1,
	1,0,0,1,1,1,0,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,
};

class SDLInstance {
	SDL_Window *win;
public:
	SDL_Renderer *renderer;

	SDLInstance(const char *title, int w, int h) {
		renderer = NULL;
		win = NULL;
		assert(SDL_Init(SDL_INIT_EVERYTHING) >= 0);
		assert(win = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, w, h,
			SDL_WINDOW_SHOWN));
		assert(renderer = SDL_CreateRenderer(win, -1,
			SDL_RENDERER_ACCELERATED));
	}

	~SDLInstance() {
		printf("bye-bye sdl\n");
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(win);
		SDL_Quit();
	}
};

class Vec2 {
public:
	float x, y;
	Vec2(float x, float y) {
		Vec2::x = x;
		Vec2::y = y;
	}

	Vec2() {
		x = 0;
		y = 0;
	}

	Vec2 operator+(const Vec2 &av) {
		Vec2 v;
		v.x = this->x + av.x;
		v.y = this->y + av.y;
		return v;
	}

	Vec2 operator*(const float &m) {
		Vec2 v;
		v.x = this->x * m;
		v.y = this->y * m;
		return v;
	}

	void clamp(float c) {
		if(fabs(x) < c && fabs(y) < c)
			return;
		float a = atan2(y, x);
		x = cosf(a)*c;
		y = sinf(a)*c;
	}

	void moveTo(Vec2 t, float m) {
		float xd = t.x-x, yd = t.y-y;
		x += xd*m;
		y += yd*m;
	}
};

class Map {
protected:
	int *arr=0;
public:
	int w, h;

	void load(const int *maparr) {
		if(arr) delete arr;
		w = maparr[0];
		h = maparr[1];
		arr = new int[w*h];
		for(int i = 0; i < w*h; i++)
			arr[i] = maparr[i+2];
	}

	~Map() {
		if(arr) delete arr;
	}

	void draw(SDLInstance &sdl) {
		SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0xff, 0xff);
		for(int i = 0; i < w*h; i++) {
			if(!arr[i])
				continue;
			SDL_Rect r = (SDL_Rect){
				(i%w)*TILE_SIZE, (i/w)*TILE_SIZE,
				TILE_SIZE, TILE_SIZE};
			SDL_RenderFillRect(sdl.renderer, &r);
		}
	}

	void print() {
		for(int y = 0; y < h; y++) {
			int *row = arr+y*w;
			for(int x = 0; x < w; x++)
				printf("%3d", row[x]);
			printf("\n");
		}
	}

	int getTile(int x, int y) {
		if(x < 0 || y < 0 || x >= w || y >= h)
			return -1;
		return arr[y*w+x];
	}

	void setTile(int x, int y, int t) {
		if(x >= 0 && y >= 0 && x < w && y < h)
			arr[y*w+x] = t;
	}

	void tsetTile(int x, int y, int t) {
		if(!getTile(x, y))
			arr[y*w+x] = t;
	}

	Map generateHeatmap(int tx, int ty) {
		Map hmap;
		hmap.w = w;
		hmap.h = h;
		hmap.arr = new int[w*h];

		for(int i = 0; i < w*h; i++)
			hmap.arr[i] = ((bool)arr[i])*-1;

		hmap.setTile(tx, ty, 1);

		for(int i = 1; ; i++) {
			bool stuck = true;

			for(int j = 0; j < w*h; j++) {
				if(hmap.arr[j] != i)
					continue;

				stuck = false;

				int x = j%w, y = j/w;
				hmap.tsetTile(x, y-1, i+1);
				hmap.tsetTile(x+1, y, i+1);
				hmap.tsetTile(x, y+1, i+1);
				hmap.tsetTile(x-1, y, i+1);
			}

			if(stuck)
				break;
		}

		for(int i = 0; i < w*h; i++) {
			hmap.arr[i]--;
			if(hmap.arr[i] < 0)
				hmap.arr[i] = -1;
		}

		return hmap;
	}

	Vec2 getVec(int x, int y) {
		int t = getTile(x, y);
		if(t <= 0)
			return Vec2(0, 0);

		int udlr[] = {
			getTile(x, y-1),
			getTile(x, y+1),
			getTile(x-1, y),
			getTile(x+1, y),
		};

		if(t == 1) {
			if(udlr[0] == 0)
				return Vec2(0, -1);
			if(udlr[1] == 0)
				return Vec2(0, 1);
			if(udlr[2] == 0)
				return Vec2(-1, 0);
			if(udlr[3] == 0)
				return Vec2(1, 0);
		}

		for(int i = 0; i < 4; i++)
			if(udlr[i] == -1)
				udlr[i] = t;

		float xd = udlr[2]-udlr[3];
		float yd = udlr[0]-udlr[1];

		if(xd && !yd) {
			for(int i = -1; i <= 1; i+=2) {
				if(i == -1 && xd > 0)
					continue;
				if(i == 1 && xd < 0)
					continue;

				if(getTile(x+i, y) == -1) {
					if(udlr[0] != -1)
						yd = 1;
					else
						yd = -1;
				}
			}
		}

		if(yd && !xd) {
			for(int i = -1; i <= 1; i+=2) {
				if(i == -1 && yd > 0)
					continue;
				if(i == 1 && yd < 0)
					continue;

				if(getTile(x, y+i) == -1) {
					if(udlr[2] != -1)
						xd = 1;
					else
						xd = -1;
				}
			}
		}

		if(!xd && !yd) {
			if(getTile(x, y+1) != -1)
				yd = 1;
			else
				xd = -1;
		}

		return Vec2(xd, yd);
	}
};

class VecMap {
	Vec2 *arr=0;
public:
	int w, h;

	void get(Map map) {
		if(arr) delete arr;
		w = map.w;
		h = map.h;
		arr = new Vec2[w*h];

		for(int i = 0; i < w*h; i++)
			arr[i] = map.getVec(i%w, i/w);
	}

	~VecMap() {
		if(arr) delete arr;
	}

	void draw(SDLInstance &sdl) {
		SDL_SetRenderDrawColor(sdl.renderer, 0xff, 0, 0, 0xff);
		for(int i = 0; i < w*h; i++) {
			int x = (i%w)*TILE_SIZE+TILE_SIZE/2;
			int y = (i/w)*TILE_SIZE+TILE_SIZE/2;
			SDL_RenderDrawLine(sdl.renderer, x, y,
				x+arr[i].x*TILE_SIZE/8,
				y+arr[i].y*TILE_SIZE/8);
		}
	}

	Vec2 vec(int x, int y) {
		if(x >= 0 && y >= 0 && x < w && y < h)
			return arr[y*w+x];
		return Vec2(0, 0);
	}
};

class Actor {
	Vec2 vel;
	Map *map;
	VecMap *vmap;
	float speed;

	void move(float xm, float ym) {
		Vec2 dst = pos + Vec2(xm, ym);

		for(float x = -0.2; x <= 0.2; x += 0.2)
			for(float y = -0.2; y <= 0.2; y += 0.2)
				if(map->getTile((int)(dst.x+x),
						(int)(dst.y+y)))
					return;

		pos = dst;
	}

public:
	Vec2 pos;

	Actor(Map *map, VecMap *vmap, int x, int y, float speed) {
		pos = Vec2((float)x+0.5, (float)y+0.5);
		vel = Vec2();
		Actor::map = map;
		Actor::vmap = vmap;
		Actor::speed = speed;
	}

	void draw(SDLInstance &sdl) {
		SDL_SetRenderDrawColor(sdl.renderer, 0xff, 0xff, 0xff, 0xff);
		int s = TILE_SIZE/4;
		SDL_Rect r = (SDL_Rect) {
			(int)(pos.x*TILE_SIZE)-s/2, (int)(pos.y*TILE_SIZE)-s/2,
			s, s
		};
		SDL_RenderFillRect(sdl.renderer, &r);
	}

	void update(int diff) {
		Vec2 acc = vmap->vec((int)pos.x, (int)pos.y);
		/*vel.x += acc.x * 0.02 * diff;
		vel.y += acc.y * 0.02 * diff;*/
		vel.moveTo(acc, speed*diff);
		//vel = vel + acc * 0.05;

		vel.clamp(speed);

		if(acc.x == 0 && acc.y == 0)
			vel = Vec2();

		int tx = floor(pos.x);
		int ty = floor(pos.y);
		if(!map->getTile(tx, ty))
			pos.moveTo(Vec2(
				(float)tx+0.5, (float)ty+0.5),
				speed*diff);

		Vec2 avel = vel * diff;
		for(int i = 0; i < 10; i++) {
			move(0, avel.y*0.1);
			move(avel.x*0.1, 0);
		}
	}
};

int main() {
	Map map;
	map.load(map1);
	VecMap vmap;
	vmap.get(map.generateHeatmap(1, 1));
	Actor actor1(&map, &vmap, 1, 9, 0.001);
	Actor actor2(&map, &vmap, 1, 9, 0.0008);

	SDLInstance sdl("Pathfinding Test", map.w*TILE_SIZE, map.h*TILE_SIZE);

	bool quit = false;
	int mx, my;
	int lastUpdate = SDL_GetTicks();
	bool showvec = false;

	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev))
			switch(ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				SDL_GetMouseState(&mx, &my);
				vmap.get(map.generateHeatmap(mx/TILE_SIZE,
					my/TILE_SIZE));
				break;
			case SDL_KEYDOWN:
				switch(ev.key.keysym.sym) {
				case SDLK_SPACE:
					showvec = !showvec;
					break;
				}
				break;
			}

		int currentTime = SDL_GetTicks();
		int diff = currentTime-lastUpdate;
		lastUpdate = currentTime;

		actor1.update(diff);
		actor2.update(diff);

		SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 0xff);
		SDL_RenderClear(sdl.renderer);

		map.draw(sdl);
		if(showvec)
			vmap.draw(sdl);
		actor1.draw(sdl);
		actor2.draw(sdl);

		SDL_RenderPresent(sdl.renderer);
	}

	return 0;
}
