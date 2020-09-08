#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * PongMode is a game mode that implements a single-player game of Pong.
 */

struct PongMode : Mode {
	PongMode();
	virtual ~PongMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void setup();
	void damaged(int damage);

	// game constants
	static constexpr glm::vec2 court_size = glm::vec2(9.6f, 6.0f);
	static constexpr glm::vec2 paddle_size = glm::vec2(0.2f, 1.0f);

	static constexpr float snake_radius = 0.2f;
	static constexpr glm::vec2 snake_size = glm::vec2(snake_radius,
			snake_radius);

	static constexpr float fruit_radius = 0.3f;
	static constexpr glm::vec2 fruit_size = glm::vec2(fruit_radius,
			fruit_radius);
	static constexpr float green_fruit_length_increase = 0.5f;

	static constexpr int initial_health = 5;
	static constexpr int collision_damage = 2;
	static constexpr int paddle_miss_damage = 1;

    static constexpr float red_fruit_chance = 0.101;
	static constexpr int red_fruit_heal = 1;

    static constexpr float initial_snake_length = 10.5f;

	//----- game state -----
    bool running;

	glm::vec2 left_paddle;
	glm::vec2 right_paddle;

	glm::vec2 snake_velocity = glm::vec2(-1.0f, 0.0f);

	float snake_length = initial_snake_length;
	std::deque<glm::vec2> snake_vertices;

	glm::vec2 green_fruit;
    bool red_fruit_exists;
	glm::vec2 red_fruit;

	float length_update_buffer = 0.0f;

	// flag to prevent multiple collisions along the same segment
	bool last_collided;

	int health;

    // keyboard flags
    bool w_pressed = false;
    bool s_pressed = false;

	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0xb4bfb0ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x2c1320ff);

	const glm::u8vec4 snake_color = HEX_TO_U8VEC4(0x6f9283ff);

    //TODO
	const glm::u8vec4 green_fruit_color = HEX_TO_U8VEC4(0x188b2dff);
	const glm::u8vec4 red_fruit_color = HEX_TO_U8VEC4(0xc54630ff);

	const glm::u8vec4 life_color = HEX_TO_U8VEC4(0xc54630ff);

	const glm::u8vec4 paddle_color = HEX_TO_U8VEC4(0x2c1320ff);

	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0x604d29ff);
	const std::vector< glm::u8vec4 > rainbow_colors = {
		HEX_TO_U8VEC4(0x604d29ff), HEX_TO_U8VEC4(0x624f29fc), HEX_TO_U8VEC4(0x69542df2),
		HEX_TO_U8VEC4(0x6a552df1), HEX_TO_U8VEC4(0x6b562ef0), HEX_TO_U8VEC4(0x6b562ef0),
		HEX_TO_U8VEC4(0x6d572eed), HEX_TO_U8VEC4(0x6f592feb), HEX_TO_U8VEC4(0x725b31e7),
		HEX_TO_U8VEC4(0x745d31e3), HEX_TO_U8VEC4(0x755e32e0), HEX_TO_U8VEC4(0x765f33de),
		HEX_TO_U8VEC4(0x7a6234d8), HEX_TO_U8VEC4(0x826838ca), HEX_TO_U8VEC4(0x977840a4),
		HEX_TO_U8VEC4(0x96773fa5), HEX_TO_U8VEC4(0xa07f4493), HEX_TO_U8VEC4(0xa1814590),
		HEX_TO_U8VEC4(0x9e7e4496), HEX_TO_U8VEC4(0xa6844887), HEX_TO_U8VEC4(0xa9864884),
		HEX_TO_U8VEC4(0xad8a4a7c),
	};
	#undef HEX_TO_U8VEC4

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "PongMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
