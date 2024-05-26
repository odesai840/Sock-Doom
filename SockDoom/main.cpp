#include <cmath>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// defines for window settings
#define res                1                       // window resolution: 1=200x150 2=400x300 4=800x600
#define SW                 200*res                 // screen width
#define SH                 150*res                 // screen height
#define SW2                (SW/2)                  // half of screen width
#define SH2                (SH/2)                  // half of screen height
#define pixelScale         4/res                   // OpenGL pixel scale
#define GLSW               (SW*pixelScale)         // OpenGL window width
#define GLSH               (SH*pixelScale)         // OpenGL window height

// defines for math constants
#define PI                 (3.1415926535897932f)   // pi constant

// defines for game variables
#define numSect            4                       // number of sectors
#define numWall            16                      // number of walls

struct Time {
	int frame1, frame2;
};

struct Keys {
	// move up, down, left, right
	int w, a, s, d;
	// strafe left, right
	int strafeL, strafeR;
	// move up, down, look up, down
	int mlook;
};

struct Rotation {
	// save sin and cos as values 0-360 degrees
	float cos[360];
	float sin[360];
};

struct Player {
	// player position
	int x, y, z;
	// player angle of rotation
	int angle;
	// variable to look up and down
	int look;
};

struct Wall {
	// bottom line point 1
	int x1, y1;
	// bottom line point 2
	int x2, y2;
	// wall color
	int color;
};

struct Sector {
	// wall number start and end
	int wallStart, wallEnd;
	// height of bottom and top
	int z1, z2;
	// center position of sector
	int x, y;
	// add y distances to sort drawing order
	int dist;
	// bottom and top colors
	int colorBot, colorTop;
	// array to hold points for surface
	int surfaces[SW];
	// variable to determine which surface to draw (top, bottom, none)
	int surface;
};

Time frameTime;
Keys keys;
Rotation rot;
Player player;
Wall walls[30];
Sector sectors[30];

// draw a pixel at x/y with rgb
void pixel(int x, int y, int color) {
	int rgb[3];
	switch (color) {
		case 0:  // yellow
			rgb[0] = 255;
			rgb[1] = 255;
			rgb[2] = 0;
			break;
		case 1:  // dark yellow
			rgb[0] = 160;
			rgb[1] = 160;
			rgb[2] = 0;
			break;
		case 2:  // green
			rgb[0] = 0;
			rgb[1] = 255;
			rgb[2] = 0;
			break;
		case 3:  // dark green
			rgb[0] = 0;
			rgb[1] = 160;
			rgb[2] = 0;
			break;
		case 4:  // cyan
			rgb[0] = 0;
			rgb[1] = 255;
			rgb[2] = 255;
			break;
		case 5:  // dark cyan
			rgb[0] = 0;
			rgb[1] = 160;
			rgb[2] = 160;
			break;
		case 6:  // brown
			rgb[0] = 160;
			rgb[1] = 100;
			rgb[2] = 0;
			break;
		case 7:  // dark brown
			rgb[0] = 110;
			rgb[1] = 50;
			rgb[2] = 0;
			break;
		case 8:  // background
			rgb[0] = 0;
			rgb[1] = 60;
			rgb[2] = 130;
			break;
	}
	glad_glColor3ub(rgb[0], rgb[1], rgb[2]);
	glad_glBegin(GL_POINTS);
	glad_glVertex2i(x * pixelScale + 2, y * pixelScale + 2);
	glad_glEnd();
}

void movePlayer() {
	// move up, down, left, right
	if (keys.a == 1 && keys.mlook == 0) {
		player.angle -= 4;

		if (player.angle < 0) {
			player.angle += 360;
		}
	}
	if (keys.d == 1 && keys.mlook == 0) {
		player.angle += 4;

		if (player.angle > 359) {
			player.angle -= 360;
		}
	}

	int deltaX = rot.sin[player.angle] * 10.0;
	int deltaY = rot.cos[player.angle] * 10.0;

	if (keys.w == 1 && keys.mlook == 0) {
		player.x += deltaX;
		player.y += deltaY;
	}
	if (keys.s == 1 && keys.mlook == 0) {
		player.x -= deltaX;
		player.y -= deltaY;
	}

	// strafe left, right
	if (keys.strafeL == 1) {
		player.x -= deltaY;
		player.y += deltaX;
	}
	if (keys.strafeR == 1) {
		player.x += deltaY;
		player.y -= deltaX;
	}

	// move up, down, look up, look down
	if (keys.a == 1 && keys.mlook == 1) {
		player.look -= 1;
	}
	if (keys.d == 1 && keys.mlook == 1) {
		player.look += 1;
	}
	if (keys.w == 1 && keys.mlook == 1) {
		player.z -= 4;
	}
	if (keys.s == 1 && keys.mlook == 1) {
		player.z += 4;
	}
}

void clearBackground() {
	int x, y;

	for (y = 0; y < SH; y++) {
		// clear background color
		for (x = 0; x < SW; x++) { 
			pixel(x, y, 8);
		}
	}
}

void cullBehindPlayer(int* x1, int* y1, int* z1, int x2, int y2, int z2) {
	// distance plane to point a (first point)
	float distA = *y1;
	// distance plane to point b (second point)
	float distB = y2;

	float dist = distA - distB;
	if (dist == 0) {
		dist = 1;
	}

	// intersection factor (normalize between 0 and 1)
	float norm = distA / (distA - distB);

	*x1 = *x1 + norm * (x2 - (*x1));
	*y1 = *y1 + norm * (y2 - (*y1));
	// prevent divide by 0
	if (*y1 == 0) {
		*y1 = 1;
	}
	*z1 = *z1 + norm * (z2 - (*z1));
}

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int color, int surfaceNum) {
	int x, y;

	// hold the difference in distnce between the bottom two points (b1 and b2)
	// y distance of the bottom line
	int distYBottom = b2 - b1;
	// y distance of top line
	int distYTop = t2 - t1;
	// x distance
	int distX = x2 - x1;

	// hold initial value of x1 starting position
	int xStart = x1;

	// cull x
	if (x1 < 1) {
		x1 = 1;  // cull left
	}
	if (x2 < 1) {
		x2 = 1;  // cull left
	}
	if (x1 > SW - 1) {
		x1 = SW - 1;  // cull right
	}
	if (x2 > SW - 1) {
		x2 = SW - 1;  // cull right
	}

	// draw vertical lines between x1 and x2
	for (x = x1; x < x2; x++) {
		// find y start and end point
		// y bottom point
		int y1 = distYBottom * (x - xStart + 0.5) / distX + b1;
		int y2 = distYTop * (x - xStart + 0.5) / distX + t1;

		// cull y
		if (y1 < 1) {
			y1 = 1;
		}
		if (y2 < 1) {
			y2 = 1;
		}
		if (y1 > SH - 1) {
			y1 = SH - 1;
		}
		if (y2 > SH - 1) {
			y2 = SH - 1;
		}

		// draw surface
		if (sectors[surfaceNum].surface == 1) {
			// save bottom points
			sectors[surfaceNum].surfaces[x] = y1;
			continue;
		}
		if (sectors[surfaceNum].surface == 2) {
			// save top points
			sectors[surfaceNum].surfaces[x] = y2;
			continue;
		}
		if (sectors[surfaceNum].surface == -1) {
			// bottom
			for (y = sectors[surfaceNum].surfaces[x]; y < y1; y++) {
				pixel(x, y, sectors[surfaceNum].colorBot);
			}
		}
		if (sectors[surfaceNum].surface == -2) {
			// top
			for (y = y1; y < sectors[surfaceNum].surfaces[x]; y++) {
				pixel(x, y, sectors[surfaceNum].colorTop);
			}
		}

		// draw wall points
		for (y = y1; y < y2; y++) {
			pixel(x, y, color);
		}
	}
}

int distance(int x1, int y1, int x2, int y2) {
	int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return distance;
}

void draw3D() {
	int s, w, i;
	int wallX[4], wallY[4], wallZ[4];
	float wallCos = rot.cos[player.angle];
	float wallSin = rot.sin[player.angle];

	// order sectors by distance using bubble sort
	for (s = 0; s < numSect - 1; s++) {
		for (w = 0; w < numSect - s - 1; w++) {
			if (sectors[w].dist < sectors[w + 1].dist) {
				Sector temp = sectors[w];
				sectors[w] = sectors[w + 1];
				sectors[w + 1] = temp;
			}
		}
	}

	// draw sectors
	for (s = 0; s < numSect; s++) {
		//clear distance
		sectors[s].dist = 0;

		// bottom surface
		if (player.z < sectors[s].z1) {
			sectors[s].surface = 1;
		}
		// top surface
		else if (player.z > sectors[s].z2) {
			sectors[s].surface = 2;
		}
		// no surface
		else {
			sectors[s].surface = 0;
		}

		// loop to draw back faces of walls
		for (i = 0; i < 2; i++) {
			for (w = sectors[s].wallStart; w < sectors[s].wallEnd; w++) {
				// offset the bottom 2 points by player position
				int x1 = walls[w].x1 - player.x;
				int y1 = walls[w].y1 - player.y;
				int x2 = walls[w].x2 - player.x;
				int y2 = walls[w].y2 - player.y;

				// swap for surface
				if (i == 0) {
					int swap = x1;
					x1 = x2;
					x2 = swap;
					swap = y1;
					y1 = y2;
					y2 = swap;
				}

				// rotate points around player for wall x position
				wallX[0] = x1 * wallCos - y1 * wallSin;
				wallX[1] = x2 * wallCos - y2 * wallSin;
				// top line has same x
				wallX[2] = wallX[0];
				wallX[3] = wallX[1];

				// rotate points around player for wall y position
				wallY[0] = y1 * wallCos + x1 * wallSin;
				wallY[1] = y2 * wallCos + x2 * wallSin;
				// top line has same y
				wallY[2] = wallY[0];
				wallY[3] = wallY[1];

				// store this wall's distance
				sectors[s].dist += distance(0, 0, (wallX[0] + wallX[1]) / 2, (wallY[0] + wallY[1]) / 2);

				// rotate points around player for wall z position
				wallZ[0] = sectors[s].z1 - player.z + ((player.look * wallY[0]) / 32.0);
				wallZ[1] = sectors[s].z1 - player.z + ((player.look * wallY[1]) / 32.0);
				// top line has higher z
				wallZ[2] = wallZ[0] + sectors[s].z2;
				wallZ[3] = wallZ[1] + sectors[s].z2;

				// dont draw if behind player
				if (wallY[0] < 1 && wallY[1] < 1) {
					continue;
				}
				// cull if one side is behind player
				if (wallY[0] < 1) {
					// bottom line
					cullBehindPlayer(&wallX[0], &wallY[0], &wallZ[0], wallX[1], wallY[1], wallZ[1]);
					// top line
					cullBehindPlayer(&wallX[2], &wallY[2], &wallZ[2], wallX[3], wallY[3], wallZ[3]);
				}
				// cull if other side is behing player
				if (wallY[1] < 1) {
					// bottom line
					cullBehindPlayer(&wallX[1], &wallY[1], &wallZ[1], wallX[0], wallY[0], wallZ[0]);
					// top line
					cullBehindPlayer(&wallX[3], &wallY[3], &wallZ[3], wallX[2], wallY[2], wallZ[2]);
				}

				// convert wall world position into screen position
				wallX[0] = wallX[0] * 200 / wallY[0] + SW2;
				wallY[0] = wallZ[0] * 200 / wallY[0] + SH2;
				wallX[1] = wallX[1] * 200 / wallY[1] + SW2;
				wallY[1] = wallZ[1] * 200 / wallY[1] + SH2;
				wallX[2] = wallX[2] * 200 / wallY[2] + SW2;
				wallY[2] = wallZ[2] * 200 / wallY[2] + SH2;
				wallX[3] = wallX[3] * 200 / wallY[3] + SW2;
				wallY[3] = wallZ[3] * 200 / wallY[3] + SH2;

				// draw points
				drawWall(wallX[0], wallX[1], wallY[0], wallY[1], wallY[2], wallY[3], walls[w].color, s);
			}

			// find average sector distance
			sectors[s].dist /= (sectors[s].wallEnd - sectors[s].wallStart);
			// flip surface number to negative to draw surface
			sectors[s].surface *= -1;
		}
	}
}

void display(GLFWwindow* window) {
	int x, y;

	// only draw 20 frames/second
	if (frameTime.frame1 - frameTime.frame2 >= 50) {
		clearBackground();
		movePlayer();
		draw3D();

		frameTime.frame2 = frameTime.frame1;
		// swap buffers
		glfwSwapBuffers(window);
	}

	// 1000 Milliseconds per second
	frameTime.frame1 = static_cast<int>(glfwGetTime() * 1000);
}

void processInput(GLFWwindow* window, int keyPressed, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		int state = (action == GLFW_PRESS) ? 1 : 0;

		switch (keyPressed) {
			case GLFW_KEY_W:
				keys.w = state;
				break;
			case GLFW_KEY_S:
				keys.s = state;
				break;
			case GLFW_KEY_A:
				keys.a = state;
				break;
			case GLFW_KEY_D:
				keys.d = state;
				break;
			case GLFW_KEY_M:
				keys.mlook = state;
				break;
			case GLFW_KEY_E:
				keys.strafeR = state;
				break;
			case GLFW_KEY_Q:
				keys.strafeL = state;
				break;
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, true);
				break;
		}
	}
}

int loadSectors[] = {
	// wall start, wall end, z1 (bottom of wall) height, z2 (top of wall) height, bottom color, top color
	 0,  4,  0, 40,  2,  3,  // sector 1
	 4,  8,  0, 40,  4,  5,  // sector 2
	 8, 12,  0, 40,  6,  7,  // sector 3
    12, 16,  0, 40,  0,  1,  // sector 4
};

int loadWalls[] = {
	// x1, y1, x2, y2, color
	 0,  0, 32,  0,  0,
	32,  0, 32, 32,  1,
	32, 32,  0, 32,  0,
	 0, 32,  0,  0,  1,

	64,  0, 96,  0,  2,
	96,  0, 96, 32,  3,
	96, 32, 64, 32,  2,
	64, 32, 64,  0,  3,

	64, 64, 96, 64,  4,
	96, 64, 96, 96,  5,
	96, 96, 64, 96,  4,
	64, 96, 64, 64,  5,

	 0, 64, 32, 64,  6,
	32, 64, 32, 96,  7,
	32, 96,  0, 96,  6,
	 0, 96,  0, 64,  7,
};

void init() {
	int x;

	// store sin/cos in degrees
	for (x = 0; x < 360; x++) {
		rot.cos[x] = cos(x / 180.0 * PI);
		rot.sin[x] = sin(x / 180.0 * PI);
	}

	// initialize player
	player.x = 70;
	player.y = -110;
	player.z = 20;
	player.angle = 0;
	player.look = 0;

	// load sectors
	int s, w;
	int v1 = 0;
	int v2 = 0;
	for (s = 0; s < numSect; s++) {
		// wall start number
		sectors[s].wallStart = loadSectors[v1 + 0];
		// wall end number
		sectors[s].wallEnd = loadSectors[v1 + 1];
		// sector bottom height
		sectors[s].z1 = loadSectors[v1 + 2];
		// sector top height
		sectors[s].z2 = loadSectors[v1 + 3] - loadSectors[v1 + 2];
		// sector bottom color
		sectors[s].colorBot = loadSectors[v1 + 4];
		// sector top color
		sectors[s].colorTop = loadSectors[v1 + 5];
		v1 += 6;
		
		// load walls
		for (w = sectors[s].wallStart; w < sectors[s].wallEnd; w++) {
			// bottom x1
			walls[w].x1 = loadWalls[v2 + 0];
			// bottom y1
			walls[w].y1 = loadWalls[v2 + 1];
			// top x2
			walls[w].x2 = loadWalls[v2 + 2];
			// top y2
			walls[w].y2 = loadWalls[v2 + 3];
			// wall color
			walls[w].color = loadWalls[v2 + 4];
			v2 += 5;
		}
	}
}

int main() {
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(GLSW, GLSH, "Sock Doom", nullptr, nullptr);
	if (window == NULL) {
		std::cout << "Failed to open GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//prevent window scaling
	glfwSetWindowSizeLimits(window, GLSW, GLSH, GLSW, GLSH);
	glfwSetKeyCallback(window, processInput);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glad_glPointSize(pixelScale);
	// set origin to bottom left
	glad_glOrtho(0, GLSW, 0, GLSH, -1, 1);

	init();

	while (!glfwWindowShouldClose(window)) {
		// display window content
		display(window);

		// poll IO events
		glfwPollEvents();
	}

	// terminate all glfw resources
	glfwTerminate();
	return 0;
}