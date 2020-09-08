#include "PongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <deque>
	   
PongMode::PongMode() {
	// initial setup of game
	setup();
	
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]
	   

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

	   
PongMode::~PongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

void PongMode::setup() {
	// Initialize snake position
	snake_vertices.clear();
	snake_vertices.emplace_back(glm::vec2(0.0f, 0.0f));
	snake_vertices.emplace_back(glm::vec2(snake_length, 0.0f));

	snake_velocity = glm::vec2(-1.0f, 0.0f);
	snake_length = initial_snake_length;
	length_update_buffer = 0.0f;

	left_paddle = glm::vec2(-court_size.x + 0.5f, 0.0f);
	right_paddle = glm::vec2(court_size.x - 0.5f, 0.0f);

	// Generate initial green_fruit position
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	green_fruit.x = (mt() / float(mt.max()) * court_size.x - court_size.x) * 0.8;
	green_fruit.y = (mt() / float(mt.max()) * court_size.y - court_size.y) * 0.8;

    red_fruit_exists = false;

	last_collided = false;
	
	health = initial_health;

    running = true;
}

void PongMode::damaged(int damage) {
	health -= damage;
	if(health < 0) {
        running = false;
	}
}

bool PongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

    if(!running) {
        if(evt.type == SDL_KEYDOWN) {
            setup();
        }
    } else {
        if (evt.type == SDL_MOUSEMOTION) {
            //convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
            glm::vec2 clip_mouse = glm::vec2(
                    (evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
                    (evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
                    );
            right_paddle.y = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).y;
        } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_w) {
            w_pressed = true;
            s_pressed = false;
        } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_w) {
            w_pressed = false;
        } else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_s) {
            s_pressed = true;
            w_pressed = false;
        } else if (evt.type == SDL_KEYUP && evt.key.keysym.sym == SDLK_s) {
            s_pressed = false;
        }
    }

	return false;
}
	   

// Inline helper function
// not sure why vec.length() always returns 2
static float inline veclength(glm::vec2 vector) {
	return pow(vector.x * vector.x + vector.y * vector.y, 0.5);
}

void PongMode::update(float elapsed) {
    if(!running) return;

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

    if(w_pressed) {
        left_paddle.y += 0.2f;
    } else if(s_pressed) {
        left_paddle.y -= 0.2f;
    }

	left_paddle.y = std::min(left_paddle.y, court_size.y - paddle_size.y);
	left_paddle.y = std::max(left_paddle.y, -court_size.y + paddle_size.y);
	right_paddle.y = std::min(right_paddle.y, court_size.y - paddle_size.y);
	right_paddle.y = std::max(right_paddle.y, -court_size.y + paddle_size.y);

	//----- snake head update -----

	// speed of snake scales proportionally with snake length
	float speed_multiplier = 2.0f + 0.5f * snake_length;

	//velocity cap, this time for balance reasons
	speed_multiplier = std::min(speed_multiplier, 7.5f);

	glm::vec2 movement = elapsed * speed_multiplier * snake_velocity;
	snake_vertices[0] += movement;
	float move_length = veclength(movement);

	// Check if the length has increased first
	if(move_length > length_update_buffer) {
		move_length -= length_update_buffer;
		length_update_buffer = 0.0f;

		// trim end of snake
		glm::vec2 end = snake_vertices.back();
		snake_vertices.pop_back();
		glm::vec2 penult = snake_vertices.back();

		while(true) {
			float length = veclength(penult - end);

			// if movement is larger than snake segment, remove it completely
			if(move_length > length) {
				move_length -= length;
				end = snake_vertices.back();
				snake_vertices.pop_back();
				penult = snake_vertices.back();;
			} else {
				end += (penult - end) * move_length / length;
				snake_vertices.push_back(end);
				break;
			}
		}
	} else {
		// can skip trimming snake
		length_update_buffer -= move_length;
		move_length = 0.0f;
	}

	//---- collision handling ----

	//paddles:
	auto paddle_vs_head = [this](glm::vec2 const &paddle) {
		//compute area of overlap:
		glm::vec2 min = glm::max(paddle - paddle_size,
				snake_vertices[0] - snake_size);
		glm::vec2 max = glm::min(paddle + paddle_size,
				snake_vertices[0] + snake_size);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return;

		if (max.x - min.x > max.y - min.y) {
			//wider overlap in x => bounce in y direction:
			if (snake_vertices[0].y > paddle.y) {
				snake_vertices[0].y = paddle.y + paddle_size.y + snake_size.y;
				snake_vertices.push_front(glm::vec2(snake_vertices[0]));
				snake_velocity.y = std::abs(snake_velocity.y);
			} else {
				snake_vertices[0].y = paddle.y - paddle_size.y - snake_size.y;
				snake_vertices.push_front(glm::vec2(snake_vertices[0]));
				snake_velocity.y = -std::abs(snake_velocity.y);
			}
		} else {
			//wider overlap in y => bounce in x direction:
			if (snake_vertices[0].x > paddle.x) {
				snake_vertices[0].x = paddle.x + paddle_size.x + snake_size.x;
				snake_vertices.push_front(glm::vec2(snake_vertices[0]));
				snake_velocity.x = std::abs(snake_velocity.x);
			} else {
				snake_vertices[0].x = paddle.x - paddle_size.x - snake_size.x;
				snake_vertices.push_front(glm::vec2(snake_vertices[0]));
				snake_velocity.x = -std::abs(snake_velocity.x);
			}
			//warp y velocity based on offset from paddle center:
			float vel = (snake_vertices[0].y - paddle.y) / (paddle_size.y + snake_size.y);
			snake_velocity.y = glm::mix(snake_velocity.y, vel, 0.75f);
		}
	};
	paddle_vs_head(left_paddle);
	paddle_vs_head(right_paddle);

	//court walls:
	if (snake_vertices[0].y > court_size.y - snake_size.y) {
		snake_vertices[0].y = 2 * (court_size.y - snake_size.y) -
			snake_vertices[0].y;
		snake_vertices.push_front(glm::vec2(snake_vertices[0].x,
					court_size.y - snake_size.y));
		if (snake_velocity.y > 0.0f) {
			snake_velocity.y = -snake_velocity.y;
		}
	} else if (snake_vertices[0].y < -court_size.y + snake_size.y) {
		snake_vertices[0].y = 2 * (-court_size.y + snake_size.y) -
			snake_vertices[0].y;
		snake_vertices.push_front(glm::vec2(snake_vertices[0].x,
					-court_size.y + snake_size.y));
		if (snake_velocity.y < 0.0f) {
			snake_velocity.y = -snake_velocity.y;
		}
	}

	if (snake_vertices[0].x > court_size.x - snake_size.x) {
		damaged(paddle_miss_damage);

		snake_vertices[0].x = 2 * (court_size.x - snake_size.x) -
			snake_vertices[0].x;
		snake_vertices.push_front(glm::vec2(court_size.x - snake_size.x,
					snake_vertices[0].y));
		if (snake_velocity.x > 0.0f) {
			snake_velocity.x = -snake_velocity.x;
		}
	} else if (snake_vertices[0].x < -court_size.x + snake_size.x) {
		damaged(paddle_miss_damage);

		snake_vertices[0].x = 2 * (-court_size.x + snake_size.x) -
			snake_vertices[0].x;
		snake_vertices.push_front(glm::vec2(-court_size.x + snake_size.x,
					snake_vertices[0].y));
		if (snake_velocity.x < 0.0f) {
			snake_velocity.x = -snake_velocity.x;
		}
	}

	// green fruit
	if(abs(snake_vertices[0].x - green_fruit.x) < snake_radius + fruit_radius &&
			abs(snake_vertices[0].y - green_fruit.y) < snake_radius + fruit_radius) {
		// regenerate green_fruit someplace else
		green_fruit.x = (mt() / float(mt.max()) * court_size.x - court_size.x) * 0.8;
		green_fruit.y = (mt() / float(mt.max()) * court_size.y - court_size.y) * 0.8;
		
		// increase snake length
		length_update_buffer += green_fruit_length_increase;
		snake_length += green_fruit_length_increase;
	}

    // red fruit
    if(red_fruit_exists) {
        if(abs(snake_vertices[0].x - red_fruit.x) < snake_radius + fruit_radius &&
                abs(snake_vertices[0].y - red_fruit.y) < snake_radius + fruit_radius) {
            // heal
            damaged(-red_fruit_heal);

            red_fruit_exists = false;
        }
    } else {
        // otherwise spawn a red fruit with a chance
        if(mt() / float(mt.max()) < red_fruit_chance) {
            red_fruit.x = (mt() / float(mt.max()) * court_size.x - court_size.x) * 0.8;
            red_fruit.y = (mt() / float(mt.max()) * court_size.y - court_size.y) * 0.8;
            red_fruit_exists = true;
        }
    }

	// snake head with body
	// we ignore the second snake segment for balance, so we can skip if there
	// are less than 4 vertices
	if(snake_vertices.size() > 3) {
        bool tick_collided = false;

		auto it = snake_vertices.begin();
		glm::vec2 head = *it;
		it += 2; // skip second segment

		glm::vec2 cur_vertex = *it++;
		glm::vec2 prev_vertex;
		while(it != snake_vertices.end()) {
				prev_vertex = cur_vertex;
				cur_vertex = *it++;

			// collision if distance from point to line is less than diameter
			float d = abs((cur_vertex.y - prev_vertex.y) * head.x -
					(cur_vertex.x - prev_vertex.x) * head.y +
					cur_vertex.x * prev_vertex.y -
					cur_vertex.y * prev_vertex.x) /
				veclength(cur_vertex - prev_vertex);

			if(d < 2 * snake_radius) {
                if(!last_collided) {
                    damaged(collision_damage);
                }
                tick_collided = true;
				break;
			}
		}

        last_collided = tick_collided;
	} else {
        last_collided = false;
    }
}

void PongMode::draw(glm::uvec2 const &drawable_size) {

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	// List of circles to be drawn
	std::vector<std::vector<Vertex>> circles;

	// inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center,
			glm::vec2 const &size, glm::u8vec4 const &color) {
		// draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-size.x, center.y-size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-size.x, center.y+size.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	// inline helper function for rectangles not aligned with axis
	// requires vertex1 to be on top-left or bottom-right of vertex 2
	auto draw_unaligned_rectangle = [&vertices](glm::vec2 const &vertex1,
			glm::vec2 const &vertex2, glm::vec2 const &size,
			glm::u8vec4 const &color) {
		float length = distance(vertex1, vertex2);
		float x_ratio = abs(vertex1.x - vertex2.x) / length;
		float y_ratio = abs(vertex1.y - vertex2.y) / length;

		if(vertex1.y == vertex2.y ||
			(vertex1.x - vertex2.x) / (vertex1.y - vertex2.y) > 0) {
			const glm::vec2 &botleft = vertex1.x - vertex2.x < 0 ? vertex1 :
				vertex2;
			const glm::vec2 &topright = vertex1.x - vertex2.x < 0 ? vertex2 :
				vertex1;

			vertices.emplace_back(glm::vec3(botleft.x - size.x * x_ratio + size.y * y_ratio, botleft.y - size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(topright.x + size.x * x_ratio + size.y * y_ratio, topright.y + size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(topright.x + size.x * x_ratio - size.y * y_ratio, topright.y + size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 

			vertices.emplace_back(glm::vec3(botleft.x - size.x * x_ratio + size.y * y_ratio, botleft.y - size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(topright.x + size.x * x_ratio - size.y * y_ratio, topright.y + size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(botleft.x - size.x * x_ratio - size.y * y_ratio, botleft.y - size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
		} else {
			const glm::vec2 &topleft = vertex1.x - vertex2.x < 0 ? vertex1 :
				vertex2;
			const glm::vec2 &botright = vertex1.x - vertex2.x < 0 ? vertex2 :
				vertex1;

			vertices.emplace_back(glm::vec3(topleft.x - size.x * x_ratio + size.y * y_ratio, topleft.y + size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(botright.x + size.x * x_ratio + size.y * y_ratio, botright.y - size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(botright.x + size.x * x_ratio - size.y * y_ratio, botright.y - size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 

			vertices.emplace_back(glm::vec3(topleft.x - size.x * x_ratio + size.y * y_ratio, topleft.y + size.x * y_ratio + size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(botright.x + size.x * x_ratio - size.y * y_ratio, botright.y - size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
			vertices.emplace_back(glm::vec3(topleft.x - size.x * x_ratio - size.y * y_ratio, topleft.y + size.x * y_ratio - size.y * x_ratio, 0.0f), color, glm::vec2(0.5f, 0.5f)); 
		}
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

	draw_rectangle(glm::vec2(-court_size.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_size.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( court_size.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_size.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f,-court_size.y-wall_radius)+s, glm::vec2(court_size.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2( 0.0f, court_size.y+wall_radius)+s, glm::vec2(court_size.x, wall_radius), shadow_color);
	draw_rectangle(left_paddle+s, paddle_size, shadow_color);
	draw_rectangle(right_paddle+s, paddle_size, shadow_color);
	//TODO shadow for snake?
	//draw_rectangle(ball+s, snake_size, shadow_color);

	//solid objects:

	// snake body
	auto it = snake_vertices.begin();
	glm::vec2 prev_vertex = *it++;
	while(it != snake_vertices.end()) {
		glm::vec2 cur_vertex = *it++;
		draw_unaligned_rectangle(cur_vertex, prev_vertex, snake_size,
                snake_color);
		prev_vertex = cur_vertex;
	}

	//walls:
	draw_rectangle(glm::vec2(-court_size.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_size.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_size.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_size.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_size.y-wall_radius), glm::vec2(court_size.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_size.y+wall_radius), glm::vec2(court_size.x, wall_radius), fg_color);

	//paddles:
	draw_rectangle(left_paddle, paddle_size, paddle_color);
	draw_rectangle(right_paddle, paddle_size, paddle_color);

	// green fruit
	draw_rectangle(green_fruit, fruit_size, green_fruit_color);

    // red fruit
    if(red_fruit_exists) {
        draw_rectangle(red_fruit, fruit_size, red_fruit_color);
    }

    // hearts at top of screen
    for(int i = 0; i < health; i++) {
        glm::vec2 pos = glm::vec2(-court_size.x + 0.5f + 1.0f * i,
                court_size.y + 0.3f + 2.0f * wall_radius);
        glm::vec2 offset1 = glm::vec2(-0.2f, 0.2f);
        glm::vec2 offset2 = glm::vec2(0.2f, 0.2f);
        draw_unaligned_rectangle(pos + offset1, pos, glm::vec2(0.2f, 0.2f), life_color);
        draw_unaligned_rectangle(pos + offset2, pos, glm::vec2(0.2f, 0.2f), life_color);
    }

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_size.x - 2.0f * wall_radius - padding,
		-court_size.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_size.x + 2.0f * wall_radius + padding,
		court_size.y + 2.0f * wall_radius + 3.0f + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);


	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
