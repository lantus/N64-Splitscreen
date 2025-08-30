#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <rspq_profile.h>
#include <malloc.h>
#include <math.h>

// Rendering
typedef struct {
    float distance;
    float rotation;
	float x;        // Cube X position
    float y;        // Cube Y position
} camera_t;

typedef struct {
    float position[3];
    float texcoord[2];
    float normal[3];
    uint32_t color;
} vertex_t;

static uint8_t controllers_connected[4];
static surface_t buffer;
static camera_t cameras[4]; // Array for four player cameras
static float box_rot = 0.3f;
int player_count = 0;
 
// Fonts
static rdpq_font_t *font = NULL;
 
// Boolean for Flashing screen
static bool flash_screen = false;

// Timer stuff

void timer_reset_flash(int ovfl)
{
	flash_screen = false;
}

void timer_pulse_rumble(int ovfl)
{

}

typedef struct Vector3 
{
    float x, y, z;
} Vector3;

typedef struct Vector4 
{
    float x, y, z, w;
} Vector4;

// Viewport settings for 4 players
typedef struct Viewport 
{
	int x, y, width, height;
	struct Vector3 cameraPos;
	struct Vector4 color;
} Viewport;

/*

+--------------------+-------------------------+
| Top-Left           | Top-Right               |
| (0, 120, 160, 120) | (160, 120, 160, 120)    |
| Player 1           | Player 2                |
+--------------------+-------------------------+
| Bottom-Left        | Bottom-Right            |
| (0, 0, 160, 120)   | (160, 0, 160, 120)      |
| Player 3           | Player 4                |
+--------------------+-------------------------+

*/

Viewport viewports[4] = 
{
	// Player 1: Red (Top-left)
    {0, 120, 160, 120, 
    {2.0f, -2.0f, 2.0f}, // Camera position
    {0.1f, 0.0f, 0.5f, 1.0f}  
    }, 
    // Player 2: Yellow (Top-right)
    {160, 120, 160, 120, 
    {-2.0f, -2.0f, 2.0f}, 
    {0.1f, 0.1f, 0.7f, 1.0f}  
    }, 
    // Player 3: Blue (Bottom-left)
    {0, 0, 160, 120, 
    {2.0f, 2.0f, 2.0f}, 
	{0.0f, 0.0f, 0.4f, 1.0f} 
    }, 
    // Player 4: Green (Bottom-right)
    {160, 0, 160, 120, 
	{-2.0f, 2.0f, 2.0f}, 
	{0.0f, 0.2f, 0.8f, 1.0f}  
    } 
};

// Camera

void update_camera(const camera_t *c)
{
	glLoadIdentity();
	gluLookAt(
		0, -c->distance, -c->distance,
		0, 0, 0,
		0, 1, 0);
	glRotatef(c->rotation, 0, 1, 0);
}

static const float cube_size = 3.0f;

// Cube Vertices - taken from glDemo example

static const vertex_t cube_vertices[] = {
    // +X
    { .position = { cube_size, -cube_size, -cube_size}, .texcoord = {0.f, 0.f}, .normal = { 1.f,  0.f,  0.f}, .color = 0x87CEEBFF },
    { .position = { cube_size,  cube_size, -cube_size}, .texcoord = {1.f, 0.f}, .normal = { 1.f,  0.f,  0.f}, .color = 0x87CEEBFF },
    { .position = { cube_size,  cube_size,  cube_size}, .texcoord = {1.f, 1.f}, .normal = { 1.f,  0.f,  0.f}, .color = 0xDDA0DDFF },
    { .position = { cube_size, -cube_size,  cube_size}, .texcoord = {0.f, 1.f}, .normal = { 1.f,  0.f,  0.f}, .color = 0xDDA0DDFF },

    // -X
    { .position = {-cube_size, -cube_size, -cube_size}, .texcoord = {0.f, 0.f}, .normal = {-1.f,  0.f,  0.f}, .color = 0x98FB98FF },
    { .position = {-cube_size, -cube_size,  cube_size}, .texcoord = {0.f, 1.f}, .normal = {-1.f,  0.f,  0.f}, .color = 0x98FB98FF },
    { .position = {-cube_size,  cube_size,  cube_size}, .texcoord = {1.f, 1.f}, .normal = {-1.f,  0.f,  0.f}, .color = 0xFFA07AFF },
    { .position = {-cube_size,  cube_size, -cube_size}, .texcoord = {1.f, 0.f}, .normal = {-1.f,  0.f,  0.f}, .color = 0xFFA07AFF },

    // +Y
    { .position = {-cube_size,  cube_size, -cube_size}, .texcoord = {0.f, 0.f}, .normal = { 0.f,  1.f,  0.f}, .color = 0xE6E6FAFF },
    { .position = {-cube_size,  cube_size,  cube_size}, .texcoord = {0.f, 1.f}, .normal = { 0.f,  1.f,  0.f}, .color = 0xE6E6FAFF },
    { .position = { cube_size,  cube_size,  cube_size}, .texcoord = {1.f, 1.f}, .normal = { 0.f,  1.f,  0.f}, .color = 0xD8BFD8FF },
    { .position = { cube_size,  cube_size, -cube_size}, .texcoord = {1.f, 0.f}, .normal = { 0.f,  1.f,  0.f}, .color = 0xD8BFD8FF },

    // -Y
    { .position = {-cube_size, -cube_size, -cube_size}, .texcoord = {0.f, 0.f}, .normal = { 0.f, -1.f,  0.f}, .color = 0xF0E68CFF },
    { .position = { cube_size, -cube_size, -cube_size}, .texcoord = {1.f, 0.f}, .normal = { 0.f, -1.f,  0.f}, .color = 0xF0E68CFF },
    { .position = { cube_size, -cube_size,  cube_size}, .texcoord = {1.f, 1.f}, .normal = { 0.f, -1.f,  0.f}, .color = 0xB0C4DEFF },
    { .position = {-cube_size, -cube_size,  cube_size}, .texcoord = {0.f, 1.f}, .normal = { 0.f, -1.f,  0.f}, .color = 0xB0C4DEFF },

    // +Z
    { .position = {-cube_size, -cube_size,  cube_size}, .texcoord = {0.f, 0.f}, .normal = { 0.f,  0.f,  1.f}, .color = 0xFFB6C1FF },
    { .position = { cube_size, -cube_size,  cube_size}, .texcoord = {1.f, 0.f}, .normal = { 0.f,  0.f,  1.f}, .color = 0xFFB6C1FF },
    { .position = { cube_size,  cube_size,  cube_size}, .texcoord = {1.f, 1.f}, .normal = { 0.f,  0.f,  1.f}, .color = 0xF0FFF0FF },
    { .position = {-cube_size,  cube_size,  cube_size}, .texcoord = {0.f, 1.f}, .normal = { 0.f,  0.f,  1.f}, .color = 0xF0FFF0FF },

    // -Z
    { .position = {-cube_size, -cube_size, -cube_size}, .texcoord = {0.f, 0.f}, .normal = { 0.f,  0.f, -1.f}, .color = 0xADD8E6FF },
    { .position = {-cube_size,  cube_size, -cube_size}, .texcoord = {0.f, 1.f}, .normal = { 0.f,  0.f, -1.f}, .color = 0xADD8E6FF },
    { .position = { cube_size,  cube_size, -cube_size}, .texcoord = {1.f, 1.f}, .normal = { 0.f,  0.f, -1.f}, .color = 0xB0E0E6FF },
    { .position = { cube_size, -cube_size, -cube_size}, .texcoord = {1.f, 0.f}, .normal = { 0.f,  0.f, -1.f}, .color = 0xB0E0E6FF },
};

static const uint16_t cube_indices[] = {
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};

void render_hud()
{
	disable_interrupts();
 
    for (int i = 0; i < 4; i++)
    {
		if (viewports[i].width == 0 && viewports[i].height == 0) continue; 
        char player_text[16];
        int player_num;
        if (viewports[i].y == 120) 
		{ 	// Top row (Player 1 and 2)
            player_num = (viewports[i].x == 0) ? ((player_count == 2) ? 2 : 3) : 4;     // 1, 2
        } 
		else 
		{   // Bottom row (Player 3 and 4)
            player_num = (viewports[i].x == 0) ? 1 : 2;		// 3, 4
        }
        sprintf(player_text, "Player %d", player_num);
        rdpq_text_print(&(rdpq_textparms_t){
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = viewports[i].width,
            .height = viewports[i].height,
            .wrap = WRAP_WORD,
        }, 1, viewports[i].x + 5, viewports[i].y + 5, player_text);
    }
	
    enable_interrupts();
}

void render_cube()
{
	glPushMatrix();
    glRotatef(box_rot, 0, 1, 0); // Rotate around Y-axis
    glRotatef(box_rot, 1, 0, 0); // Rotate around X-axis
    box_rot += 0.1f;

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), (void*)(0*sizeof(float) + (void*)cube_vertices));
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_t), (void*)(3*sizeof(float) + (void*)cube_vertices));
	glNormalPointer(GL_FLOAT, sizeof(vertex_t), (void*)(5*sizeof(float) + (void*)cube_vertices));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), (void*)(8*sizeof(float) + (void*)cube_vertices));

	glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(uint16_t), GL_UNSIGNED_SHORT, cube_indices);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable(GL_COLOR_MATERIAL);
	
	glPopMatrix();

}
 
int main()
{

	dfs_init(DFS_DEFAULT_LOCATION);

	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);

	buffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());

	joypad_init();
	rdpq_init();
	gl_init();

	// Load Font

	font = rdpq_font_load("rom:/BakbakOne-Regular.font64");
	rdpq_font_style(font, 0, &(rdpq_fontstyle_t)
	{
    	.color = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
		//.outline_color = RGBA32(0x00, 0x00, 0x, 0xFF),
	});
	rdpq_text_register_font(1, font);

	// Initialize four cameras with default distance and rotation
    for (int i = 0; i < 4; i++) {
        cameras[i].distance = -10.0f; // Adjusted for better visibility
        cameras[i].rotation = 0.0f;
		cameras[i].x = 0.0f; // Initial X position
        cameras[i].y = 0.0f; // Initial Y position
    }

	float aspect_ratio = (float)320 / (float)240; // Default aspect ratio
	float near_plane = 1.0f;
	float far_plane = 50.0f;
 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-near_plane*aspect_ratio, near_plane*aspect_ratio, -near_plane, near_plane, near_plane, far_plane);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	static const GLfloat env_color[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, env_color);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    static const GLfloat light_position[] = {0.0f, 0.0f, -5.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    static const GLfloat light_ambient[] = {0.2f, 0.2f, 0.3f, 1.0f};
    static const GLfloat light_diffuse[] = {0.8f, 0.8f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    // Enable OpenGL features
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST); // Enable scissor test for viewport isolation
	
	while (1)
	{
		joypad_poll();
		
		// Count connected players
       
		player_count = 0;
        
		for (int i = 0; i < 4; i++) 
		{
			//controllers_connected[i] = 0;
            if (joypad_is_connected(i))
			{
				//controllers_connected[i] = 1;
				player_count++;
			}
        }
 
		// Update viewports based on player count
        if (player_count == 1) 
		{
            viewports[0] = (Viewport){0, 0, 320, 240, {2.0f, 2.0f, 2.0f}, {0.4f, 0.0f, 0.0f, 1.0f}}; // Full screen for P1
            viewports[1] = (Viewport){0, 0, 0, 0, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}}; // Disable others
            viewports[2] = (Viewport){0, 0, 0, 0, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
            viewports[3] = (Viewport){0, 0, 0, 0, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
        } 
		else if (player_count == 2) 
		{
            viewports[0] = (Viewport){0, 120, 320, 120, {2.0f, 2.0f, 2.0f}, {0.4f, 0.0f, 0.0f, 1.0f}}; // P1: Top half
            viewports[1] = (Viewport){0, 0, 320, 120, {-2.0f, -2.0f, 2.0f}, {0.0f, 0.4f, 0.0f, 1.0f}}; // P2: Bottom half
            viewports[2] = (Viewport){0, 0, 0, 0, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}}; // Disable others
            viewports[3] = (Viewport){0, 0, 0, 0, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
        } 
		else 
		{ // 3 or 4 players
            viewports[0] = (Viewport){0, 120, 160, 120, {2.0f, 2.0f, 2.0f}, {0.4f, 0.0f, 0.0f, 1.0f}}; // P1: Top-left
            viewports[1] = (Viewport){160, 120, 160, 120, {-2.0f, 2.0f, 2.0f}, {0.4f, 0.4f, 0.0f, 1.0f}}; // P2: Top-right
            viewports[2] = (Viewport){0, 0, 160, 120, {2.0f, -2.0f, 2.0f}, {0.0f, 0.0f, 0.4f, 1.0f}}; // P3: Bottom-left
            viewports[3] = (Viewport){160, 0, 160, 120, {-2.0f, -2.0f, 2.0f}, {0.0f, 0.4f, 0.0f, 1.0f}}; // P4: Bottom-right
        }
		
        // Poll all four controllers for camera distance and cube movement
		
        // Update input and camera for connected players only
        for (int i = 0; i < player_count && i < 4; i++) 
		{
            joypad_inputs_t inputs = joypad_get_inputs(i);
            float y = inputs.stick_y / 128.0f;
            float x = inputs.stick_x / 128.0f;
            if (fabsf(y) > 0.1f) 
			{
                cameras[i].distance += y * 0.2f;
                if (cameras[i].distance > -2.0f) cameras[i].distance = -2.0f;
                if (cameras[i].distance < -20.0f) cameras[i].distance = -20.0f;
            }
            if (fabsf(x) > 0.1f) 
			{
                cameras[i].x += x * 0.4f;
                if (cameras[i].x < -5.0f) cameras[i].x = -5.0f;
                if (cameras[i].x > 5.0f) cameras[i].x = 5.0f;
            }
            float y_move = inputs.stick_y / 128.0f;
            if (fabsf(y_move) > 0.1f) 
			{
                cameras[i].y += -y_move * 0.1f;
                if (cameras[i].y < -5.0f) cameras[i].y = -5.0f;
                if (cameras[i].y > 5.0f) cameras[i].y = 5.0f;
            }
        }

		surface_t *disp = display_get();
		rdpq_attach(disp, &buffer);

		gl_context_begin();
		glEnable(GL_SCISSOR_TEST); // Enable scissor test for viewport isolation
		for (int i = 0; i < 4; i++)
		{
			// Skip disabled viewports
			if (viewports[i].width == 0 && viewports[i].height == 0) continue; 
			 
			// Set viewport
			glViewport(viewports[i].x, viewports[i].y, viewports[i].width, viewports[i].height);
			glScissor(viewports[i].x, viewports[i].y, viewports[i].width, viewports[i].height);
			
			// Set projection matrix based on current viewport aspect ratio
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            float aspect_ratio = (float)viewports[i].width / (float)viewports[i].height;
            glFrustum(-near_plane * aspect_ratio, near_plane * aspect_ratio, -near_plane, near_plane, near_plane, far_plane);


			// Use per-viewport background color
			glClearColor(viewports[i].color.x, viewports[i].color.y, viewports[i].color.z, viewports[i].color.w);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			 // Set up camera to look at the center of the viewport
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // Adjust camera to look at (0, 0, 0) with distance and rotation
            gluLookAt(
                0.0f, 0.0f, cameras[i].distance, // Camera position at origin with adjusted distance
                0.0f, 0.0f, 0.0f,          		 // Look at the center
                0.0f, 1.0f, 0.0f           		 // Up vector
            );
			
			glRotatef(cameras[i].rotation, 0, 1, 0);

            // Apply per-player cube translation
            glPushMatrix();
            glTranslatef(cameras[i].x, cameras[i].y, 0.0f); // Move cube based on player input
            render_cube();
            glPopMatrix();
		}

		gl_context_end();
		
		glDisable(GL_SCISSOR_TEST); 
		render_hud();
		
		rdpq_detach_show();

	}
	
	return 0;
}
