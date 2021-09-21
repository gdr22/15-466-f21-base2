#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <time.h>

GLuint catblob_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > catblob_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("CatMesh.pnct"));
	catblob_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > catblob_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("Cat.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = catblob_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = catblob_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::Block PlayMode::new_block(float angle, float depth) {
	// Look for any dead blocks we can reuse
	for(Block &block : blocks) {
		if(!block.alive) {
			block.angle = angle;
			block.depth = depth;
			block.alive = true;
			block.update_pos(rotation);
			return block;
		}
	}
	
	Block block;

	block.angle = angle;
	block.depth = depth;
	block.tile = new Scene::Transform;

	block.tile->position = grass.position;
	block.tile->rotation = grass.rotation;
	block.tile->scale    = grass.scale;

	block.tile->name = "Grass Copy";
	//scene.transforms.push_back(tile);

	Scene::Drawable drawable(block.tile);
	drawable.pipeline = lit_color_texture_program_pipeline;
	drawable.pipeline.vao = catblob_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = grass_vertex_type;
	drawable.pipeline.start = grass_vertex_start;
	drawable.pipeline.count = grass_vertex_count;
	PlayMode::scene.drawables.push_back(drawable);

	
	block.update_pos(rotation);
	blocks.emplace_back(block);
	return block;
}

bool PlayMode::spawn_tile() {
	float max_depth = 0.f;
	for(Block &block : blocks) {
		if(max_depth < -block.depth) {
			max_depth = -block.depth;
		}
	}

	float angle_variance = spawn_angle_variance;
	
	if(max_depth < MAX_DEPTH) {
		// Generate a new strip of tiles
		uint32_t strip_size = (((unsigned)rand()) % 5);
		strip_size += 3;

		float randnum = (float)rand() / RAND_MAX;
		float skew = (randnum * 2.f * spawn_skew_variance) - spawn_skew_variance;

		max_depth += spawn_dist;

		for(uint32_t i = 0; i < strip_size; i++) {
			randnum = (float)rand() / RAND_MAX;
			spawn_angle += (2.f * angle_variance * randnum) - angle_variance + skew;
			
			new_block(spawn_angle, -max_depth);
			max_depth += 5.0f;

			// Fix the angle variance to be small within tiles in a strip
			angle_variance = 5.0f;
		}
		return true;
	}
	
	return false;
}

PlayMode::PlayMode() : scene(*catblob_scene) {
	//init game states
	score = 0;

	//get pointers:
	for (auto& drawable : scene.drawables) {
		//sadness that switch doesnt work on string
		if (drawable.transform->name == "Grass") {
			grass = *drawable.transform;
			grass_vertex_type = drawable.pipeline.type;
			grass_vertex_start = drawable.pipeline.start;
			grass_vertex_count = drawable.pipeline.count;
			drawable.transform->scale = glm::vec3(0.0f, 0.0f, 0.0f);
		} 
		else if (drawable.transform->name == "Left Back Foot") {
			lbpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Left Ear") {
			lear = drawable.transform;
		}
		else if (drawable.transform->name == "Left Front Foot") {
			lfpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Right Back Foot") {
			rbpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Right Ear") {
			rear = drawable.transform;
		}
		else if (drawable.transform->name == "Right Front Foot") {
			rfpeet = drawable.transform;
		}
		else if (drawable.transform->name == "Sphere") {
			cat = drawable.transform;
		}
		else if (drawable.transform->name == "Tail") {
			cattail = drawable.transform;
		}
		else {
			throw std::runtime_error("Found unexpected object: " + drawable.transform->name);
		}
	}
	catTransforms = { cat, lbpeet, lfpeet, rbpeet, rfpeet, lear, rear, cattail };
	lfpeet_base = lfpeet->position;
	lbpeet_base = lbpeet->position;
	rfpeet_base = rfpeet->position;
	rbpeet_base = rbpeet->position;

	// Make all cat parts children of the cat body
	for(Scene::Transform* transform : catTransforms) {
		if(transform != cat) {
			transform->position = (cat->make_world_to_local()) * glm::vec4(transform->position.x, transform->position.y, transform->position.z, 1.0f);
			// We should be inverting the parent rotation here. Not sure how to though
			transform->rotation = glm::inverse(cat->rotation) * transform->rotation;
			transform->parent = cat;
		}
	}

	cat->position.y = 0;
	cat->position.z = 3;

	//if (grass.name != "Grass") throw std::runtime_error("Grass object not found.");
	
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//spawn the tunnel
	srand((unsigned)time(NULL) + 15466);

	// Spawn tiles until we hit max depth
	while(spawn_tile()) { spawn_angle_variance = 60.f; }
}

PlayMode::~PlayMode() {
}

float norm_angle(float angle) {
	float a = angle;

	while (a < -180.f || a > 180.f)
	{
		a += a < -180.f ? 360.f : 0.f;
		a -= a >  180.f ? 360.f : 0.f;
	}

	return a;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a || evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d || evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a || evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d || evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			up.pressed = false;
			return true;
		}
	}
	return false;
}


void PlayMode::update(float elapsed) {
	if (game_over) return;

	score += elapsed;

	//rotate tunnel
	if (left.pressed) {
		//iterate through tiles and rotate transforms by rotation_speed * elapsed
		rotation += rotation_speed * elapsed;
	}
	else if (right.pressed) {
		//same thing but other direction
		rotation -= rotation_speed * elapsed;
	}

	//check if grounded aka collision with tile
	grounded = false;
//
	
	for(Block &block : blocks) {
		if(!block.alive) continue;

		if(abs(block.depth) < 5.f) {
			if(abs(norm_angle(block.angle + rotation)) < 15.f) {
				if(cat->position.z <= 2.85 && cat->position.z >= 2) {
					grounded = true;
				}
			}
		}
	}

	if (up.pressed && grounded) {
		cat_speed = 15.0f; //sorry this is just a magic number rn
		grounded = false; //perhaps we don't need this
	}
	if (grounded) {
		cat_speed = 0.0f;
	}
	else {
		cat_speed += gravity * elapsed;
	}
	//update cat position
	//for (Scene::Transform* t : catTransforms) {
	cat->position.z += (cat_speed * elapsed);
	//}
	
	if (cat->position.z < -15) {
		game_over = true;
	}
	//keeping this code for reference on animation
	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	lbpeet->position = lbpeet_base + glm::vec3(
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.0f
	);
	lfpeet->position = lfpeet_base + glm::vec3(
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.0f
	);
	rbpeet->position = rbpeet_base + glm::vec3(
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.0f
	);
	rfpeet->position = rfpeet_base + glm::vec3(
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.5f * std::sin(wobble * 20.0f * 2.0f * float(M_PI)),
		0.0f
	);

	for(Block &block : blocks) {
		if(!block.alive) continue;

		block.depth += elapsed * block_speed;
		block.update_pos(rotation);
	}

	block_speed += elapsed * 0.1f;

	if(spawn_dist < 30.f) {
		spawn_dist += elapsed;
	}

	if(spawn_angle_variance < 90.f) {
		spawn_angle_variance += elapsed * 5;
	}

	// Recycle blocks
	for(Block &block : blocks) {
		if(block.depth > 20.0f) {
			block.alive = false;
			//blocks.erase(blocks.begin() + i);
			//i--;

			//new_block(block.angle, -500.f);
		}
	}
	
	// Spawn tiles until we hit max depth
	while(spawn_tile()) { }
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.71f, 0.95f, 1.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		constexpr float bigH = 0.6f;
		lines.draw_text("Arrows/A+D to rotate tunnel, Space to jump",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Arrows/A+D to rotate tunnel, Space to jump",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		if (!game_over) {
			lines.draw_text("Score: " + std::to_string((int)score),
				glm::vec3(-aspect + 0.1f * H, 1.0f - 1.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("Score: " + std::to_string((int)score),
				glm::vec3(-aspect + 0.1f * H + ofs, 1.0f - 1.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		else {
			lines.draw_text("Score: " + std::to_string((int)score),
				glm::vec3(-aspect + 0.1f * bigH, 1.0f - 1.1f * bigH, 0.0),
				glm::vec3(bigH, 0.0f, 0.0f), glm::vec3(0.0f, bigH, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("Score: " + std::to_string((int)score),
				glm::vec3(-aspect + 0.1f * bigH + ofs, 1.0f - 1.1f * bigH + ofs, 0.0),
				glm::vec3(bigH, 0.0f, 0.0f), glm::vec3(0.0f, bigH, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}
